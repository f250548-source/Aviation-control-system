#include "game_engine.h"
#include "menu.h"
#include "utils.h"
#include <cstdlib>
#include <ctime>
using namespace std;

// Construct engine and wire graph-dependent helpers
game_engine::game_engine() {
    player_name = "";
    current_tick = 0;
    max_ticks = 30;
    credibility = 100;
    actions_remaining = 2;
    flights_landed = 0;
    emergencies_resolved = 0;
    game_over = false;
    mission_won = false;
    failure_cause = "";
    tower_disabled = false;
    cyber_corridor_disabled = false;
    last_spawn_tick = 0;
    next_spawn_interval = 3;
    emergency_head = 0;
    missing_head = 0;
    sweep = new radar_sweep(&airspace);
    router = new route_finder(&airspace);
    mst_tool = new radar_mst(&airspace);

    int i = 0;
    while (i < 32) {
        wrong_penalty_by_type[i] = 5;
        i = i + 1;
    }
}

// Free graph helpers and linked lists
game_engine::~game_engine() {
    free_emergency_list();
    free_missing_list();
    if (sweep != 0) {
        delete sweep;
        sweep = 0;
    }
    if (router != 0) {
        delete router;
        router = 0;
    }
    if (mst_tool != 0) {
        delete mst_tool;
        mst_tool = 0;
    }
}

void game_engine::free_emergency_list() {
    active_emergency* current = emergency_head;
    while (current != 0) {
        active_emergency* temp = current;
        current = current->next;
        delete temp;
    }
    emergency_head = 0;
}

void game_engine::free_missing_list() {
    missing_flight_record* current = missing_head;
    while (current != 0) {
        missing_flight_record* temp = current;
        current = current->next;
        delete temp;
    }
    missing_head = 0;
}

// Soft reset of counters (structures rebuilt by start_new_game flow)
void game_engine::reset_state() {
    current_tick = 0;
    credibility = 100;
    actions_remaining = 2;
    flights_landed = 0;
    emergencies_resolved = 0;
    game_over = false;
    mission_won = false;
    failure_cause = "";
    tower_disabled = false;
    cyber_corridor_disabled = false;
    last_spawn_tick = 0;
    next_spawn_interval = random_range(3, 5);
    free_emergency_list();
    free_missing_list();

    int i = 0;
    while (i < 32) {
        wrong_penalty_by_type[i] = 5;
        i = i + 1;
    }
}

void game_engine::clamp_credibility() {
    if (credibility < 0) {
        credibility = 0;
    }
    if (credibility > 200) {
        credibility = 200;
    }
}

void game_engine::apply_credibility_delta(int delta) {
    credibility = credibility + delta;
    clamp_credibility();
    if (delta >= 0) {
        cout << "[Credibility +" << delta << "] now " << credibility << endl;
    } else {
        cout << "[Credibility " << delta << "] now " << credibility << endl;
    }
}

// Wrong command: -5 first time for that slot, then -10, -15, ...
void game_engine::apply_wrong_command(int penalty_slot) {
    if (penalty_slot < 0 || penalty_slot >= 32) {
        penalty_slot = 0;
    }
    int penalty = wrong_penalty_by_type[penalty_slot];
    cout << "[ATC Error] Invalid or escalating action. Penalty -"
         << penalty << endl;
    apply_credibility_delta(-penalty);
    wrong_penalty_by_type[penalty_slot] = penalty + 5;
}

bool game_engine::is_free_action(int option_code) {
    return option_code == 13 || option_code == 14 || option_code == 15
        || option_code == 16 || option_code == 22;
}

void game_engine::consume_action_if_needed(int option_code) {
    if (option_code == 23 || option_code == 24) {
        return;
    }
    if (is_free_action(option_code)) {
        return;
    }
    if (actions_remaining > 0) {
        actions_remaining = actions_remaining - 1;
        cout << "Actions remaining this turn: " << actions_remaining << endl;
    }
}

void game_engine::push_undo_snapshot(string action_label) {
    history_log.log_command(action_label, current_tick);
    history_log.push_snapshot(current_tick, credibility, flights_landed,
                              emergencies_resolved, action_label);
}

int game_engine::random_range(int low, int high) {
    if (high < low) {
        int temp = low;
        low = high;
        high = temp;
    }
    return low + (rand() % (high - low + 1));
}

string game_engine::pick_random_sector() {
    int n = airspace.get_sector_count();
    if (n <= 0) {
        return "Control_Tower";
    }
    int attempts = 0;
    while (attempts < 20) {
        int index = random_range(0, n - 1);
        if (!airspace.is_runway(index)) {
            return airspace.get_sector_name(index);
        }
        attempts = attempts + 1;
    }
    return airspace.get_sector_name(0);
}

string game_engine::pick_random_runway(bool emergency_capable_only) {
    if (emergency_capable_only) {
        if (random_range(0, 1) == 0) {
            return "RWY09";
        }
        return "RWY12";
    }
    int pick = random_range(0, 3);
    if (pick == 0) {
        return "RWY09";
    }
    if (pick == 1) {
        return "RWY12";
    }
    if (pick == 2) {
        return "RWY04";
    }
    return "RWY17";
}

string game_engine::pick_hijack_actor(int& deadline_out) {
    int pick = random_range(0, 2);
    if (pick == 0) {
        deadline_out = 2;
        return "Dart Jet";
    }
    if (pick == 1) {
        deadline_out = 3;
        return "Overflow Jet";
    }
    deadline_out = 2;
    return "Dangling Jet";
}

string game_engine::pick_cyber_actor(int& deadline_out) {
    int pick = random_range(0, 2);
    if (pick == 0) {
        deadline_out = 2;
        return "Phantom Root";
    }
    if (pick == 1) {
        deadline_out = 3;
        return "Leak Air";
    }
    deadline_out = 1;
    return "Crack Heap";
}

void game_engine::announce_threat(string event_label, string actor, int ticks) {
    cout << "ALERT: " << event_label << " threat actor [" << actor
         << "] - respond within " << ticks << " tick(s)!" << endl;
}

flight* game_engine::create_flight_template(string callsign, int priority_level,
                                           string sector, string runway) {
    flight* f = new flight;
    f->callsign = callsign;
    f->aircraft_type = "A320";
    f->airline = "MayDayAir";
    f->origin = "Origin";
    f->destination = runway;
    f->passenger_count = random_range(50, 180);
    f->fuel_remaining = random_range(6, 15);
    f->current_sector = sector;
    f->assigned_runway = runway;
    f->arrival_tick = current_tick;
    f->priority_level = priority_level;
    f->status = "ACTIVE";
    f->next = 0;
    return f;
}

void game_engine::register_flight_everywhere(flight* f) {
    if (f == 0) {
        return;
    }
    // registration takes ownership of this node
    flight_registry.add_flight(f);
    callsign_table.insert(f);
    flight_bst.insert(f);
    flight_avl.insert(f);
}

void game_engine::unregister_flight_everywhere(string callsign) {
    flight_registry.remove_flight(callsign);
    callsign_table.remove(callsign);
    flight_bst.remove(callsign);
    flight_avl.remove(callsign);
}

void game_engine::queue_by_priority(flight* f) {
    if (f == 0) {
        return;
    }
    if (f->priority_level == NORMAL) {
        landing_system.enqueue_normal(f);
    } else {
        landing_system.insert_emergency(f);
    }
}

void game_engine::add_active_emergency(string callsign, int event_type,
                                      int deadline_ticks, string threat_actor) {
    active_emergency* node = new active_emergency;
    node->callsign = callsign;
    node->event_type = event_type;
    node->spawn_tick = current_tick;
    node->deadline_ticks = deadline_ticks;
    node->threat_actor = threat_actor;
    node->resolved = false;
    node->next = emergency_head;
    emergency_head = node;
}

active_emergency* game_engine::find_emergency(string callsign, int event_type) {
    active_emergency* current = emergency_head;
    while (current != 0) {
        if (!current->resolved && current->callsign == callsign
            && current->event_type == event_type) {
            return current;
        }
        current = current->next;
    }
    // Also allow match by type only when callsign empty
    if (callsign.length() == 0) {
        current = emergency_head;
        while (current != 0) {
            if (!current->resolved && current->event_type == event_type) {
                return current;
            }
            current = current->next;
        }
    }
    return 0;
}

void game_engine::resolve_emergency(string callsign, int event_type, int reward) {
    active_emergency* node = find_emergency(callsign, event_type);
    if (node == 0) {
        apply_wrong_command(event_type);
        return;
    }
    node->resolved = true;
    emergencies_resolved = emergencies_resolved + 1;
    apply_credibility_delta(reward);
    cout << "Emergency resolved for " << node->callsign
         << " (" << node->threat_actor << ")." << endl;
}

void game_engine::seed_missing_catalog() {
    // Table from the brief: name / sector / window / credibility gate
    missing_flight_record* a = new missing_flight_record;
    a->flight_name = "MH370";
    a->last_known_sector = "Restricted_Zone";
    a->tick_window = 6;
    a->credibility_threshold = 63;
    a->spawn_tick = -1;
    a->located = false;
    a->next = 0;

    missing_flight_record* b = new missing_flight_record;
    b->flight_name = "PK404";
    b->last_known_sector = "Syntax_Sector";
    b->tick_window = 9;
    b->credibility_threshold = 54;
    b->spawn_tick = -1;
    b->located = false;
    b->next = a;

    missing_flight_record* c = new missing_flight_record;
    c->flight_name = "FT739";
    c->last_known_sector = "Cache_Fly";
    c->tick_window = 13;
    c->credibility_threshold = 40;
    c->spawn_tick = -1;
    c->located = false;
    c->next = b;

    missing_flight_record* d = new missing_flight_record;
    d->flight_name = "GA542";
    d->last_known_sector = "Air_Segment";
    d->tick_window = 15;
    d->credibility_threshold = 38;
    d->spawn_tick = -1;
    d->located = false;
    d->next = c;

    missing_head = d;
}

// ---- Per-tick pipeline ----

void game_engine::auto_move_flights() {
    cout << "-- Auto-move flights --" << endl;
    flight* current = flight_registry.get_head();
    // Collect callsigns that crash this tick to avoid iterator invalidation
    string crash_list[64];
    int crash_count = 0;

    while (current != 0) {
        if (current->status == "ACTIVE" || current->status == "EMERGENCY") {
            // Missing-style flights should not appear here; still guard fuel
            string hop = router->next_hop(current->current_sector,
                                          current->assigned_runway);
            if (hop.length() > 0) {
                current->current_sector = hop;
                cout << current->callsign << " vectors to " << hop << endl;
                print_aviation_call("Vector");
            }
            current->fuel_remaining = current->fuel_remaining - 1;
            if (current->fuel_remaining <= 0) {
                cout << "MAYDAY! " << current->callsign
                     << " fuel critical - CRASH." << endl;
                print_aviation_call("Mayday");
                apply_credibility_delta(-50);
                history_log.log_command(
                    "CRASH " + current->callsign, current_tick);
                if (crash_count < 64) {
                    crash_list[crash_count] = current->callsign;
                    crash_count = crash_count + 1;
                }
            }
        }
        current = current->next;
    }

    int i = 0;
    while (i < crash_count) {
        unregister_flight_everywhere(crash_list[i]);
        i = i + 1;
    }
}

void game_engine::apply_escalation() {
    cout << "-- Apply escalation timers --" << endl;
    active_emergency* current = emergency_head;
    while (current != 0) {
        if (!current->resolved) {
            int elapsed = current_tick - current->spawn_tick;
            int type = current->event_type;

            if (type == WAR && elapsed > 2) {
                end_game(false, "War aircraft not countered within 2 ticks");
                return;
            }
            if (type == HIJACK && elapsed > current->deadline_ticks) {
                apply_credibility_delta(-20);
                end_game(false, "Hijack not intercepted in time");
                return;
            }
            if (type == CYBER) {
                // Not countered within 3 ticks: disable one corridor once
                if (elapsed > 3 && !cyber_corridor_disabled) {
                    cout << "Cyber escalation: corridor disabled." << endl;
                    airspace.remove_corridor("Control_Tower", "Cache_Fly");
                    cyber_corridor_disabled = true;
                }
                // Within 4 ticks still open: tower disabled / mission fail
                if (elapsed > 4) {
                    tower_disabled = true;
                    apply_credibility_delta(-15);
                    end_game(false, "Cyber attack disabled the tower");
                    return;
                }
            }
            if (type == FUEL && elapsed > 5) {
                apply_credibility_delta(-50);
                unregister_flight_everywhere(current->callsign);
                current->resolved = true;
                cout << current->callsign << " crashed (fuel emergency timeout)."
                     << endl;
            }
            if (type == MEDICAL && elapsed > 4) {
                apply_credibility_delta(-30);
                end_game(false, "Medical emergency - passenger casualty");
                return;
            }
            if (type == WEATHER && elapsed > 4) {
                apply_credibility_delta(-50);
                unregister_flight_everywhere(current->callsign);
                current->resolved = true;
                cout << "Weather forced crash of " << current->callsign << endl;
            }
            if (type == MECHANICAL && elapsed > 5) {
                apply_credibility_delta(-50);
                unregister_flight_everywhere(current->callsign);
                current->resolved = true;
                cout << "Mechanical failure crash: " << current->callsign << endl;
            }
        }
        current = current->next;
    }

    // Missing flight windows
    missing_flight_record* miss = missing_head;
    while (miss != 0) {
        if (!miss->located && miss->spawn_tick >= 0) {
            int elapsed = current_tick - miss->spawn_tick;
            if (elapsed > miss->tick_window) {
                end_game(false,
                         "Missing flight " + miss->flight_name
                         + " - Restricted Zone breach / hijack-style alarm");
                return;
            }
        }
        miss = miss->next;
    }
}

void game_engine::auto_spawn() {
    if (current_tick == 0) {
        return;
    }
    if (current_tick - last_spawn_tick < next_spawn_interval) {
        return;
    }
    cout << "-- Auto-spawn scenario traffic --" << endl;
    last_spawn_tick = current_tick;
    next_spawn_interval = random_range(3, 5);

    // Randomized event mapping
    int event_roll = random_range(1, 9);
    string sector = pick_random_sector();
    string runway = pick_random_runway(false);
    string callsign = "MD" + string(1, (char)('0' + random_range(1, 9)))
                    + string(1, (char)('0' + random_range(0, 9)))
                    + string(1, (char)('0' + random_range(0, 9)));

    if (event_roll == MISSING) {
        // Activate one unused missing-flight record
        missing_flight_record* miss = missing_head;
        while (miss != 0) {
            if (!miss->located && miss->spawn_tick < 0) {
                miss->spawn_tick = current_tick;
                cout << "MISSING FLIGHT reported: " << miss->flight_name
                     << " last known " << miss->last_known_sector
                     << " (window " << miss->tick_window << " ticks)" << endl;
                print_aviation_call("Pan-Pan");
                return;
            }
            miss = miss->next;
        }
        event_roll = NORMAL;
    }

    int priority_level = event_roll;
    if (priority_level < WAR || priority_level > NORMAL) {
        priority_level = NORMAL;
    }

    flight* f = create_flight_template(callsign, priority_level, sector, runway);
    if (priority_level != NORMAL) {
        f->status = "EMERGENCY";
        print_aviation_call("Mayday");
    } else {
        print_aviation_call("Roger");
    }

    register_flight_everywhere(f);
    queue_by_priority(f);

    if (priority_level == HIJACK) {
        int deadline = 2;
        string actor = pick_hijack_actor(deadline);
        add_active_emergency(callsign, HIJACK, deadline, actor);
        announce_threat("HIJACK", actor, deadline);
    } else if (priority_level == CYBER) {
        int deadline = 2;
        string actor = pick_cyber_actor(deadline);
        add_active_emergency(callsign, CYBER, deadline, actor);
        announce_threat("CYBER", actor, deadline);
    } else if (priority_level == WAR) {
        f->airline = "NullAir";
        f->aircraft_type = "Military";
        add_active_emergency(callsign, WAR, 2, "NullAir Hostile");
        announce_threat("WAR", "NullAir Hostile", 2);
    } else if (priority_level == FUEL) {
        add_active_emergency(callsign, FUEL, 5, "Low Fuel");
    } else if (priority_level == MEDICAL) {
        add_active_emergency(callsign, MEDICAL, 4, "Medical");
    } else if (priority_level == WEATHER) {
        add_active_emergency(callsign, WEATHER, 4, "Weather");
    } else if (priority_level == MECHANICAL) {
        add_active_emergency(callsign, MECHANICAL, 5, "Mechanical");
    }

    cout << "Spawned " << callsign << " priority=" << priority_level
         << " at " << sector << " -> " << runway << endl;
}

void game_engine::check_win_lose() {
    if (game_over) {
        return;
    }
    if (credibility <= 0) {
        end_game(false, "Credibility depleted");
        return;
    }
    if (tower_disabled) {
        end_game(false, "Tower disabled");
        return;
    }
    if (current_tick >= max_ticks) {
        // Survive to max ticks with credibility > 0 => win
        end_game(true, "");
        return;
    }
}

void game_engine::advance_tick() {
    current_tick = current_tick + 1;
    actions_remaining = 2;
    cout << endl;
    cout << "======== TICK " << current_tick
         << " / " << max_ticks << " ========" << endl;
    cout << "Credibility: " << credibility << endl;

    auto_move_flights();
    if (game_over) {
        return;
    }
    apply_escalation();
    if (game_over) {
        return;
    }
    auto_spawn();
    if (game_over) {
        return;
    }
    check_win_lose();
}

void game_engine::end_game(bool won, string cause) {
    if (game_over) {
        return;
    }
    game_over = true;
    mission_won = won;
    failure_cause = cause;
    string outcome = "WIN";
    if (!won) {
        outcome = "LOSE";
    }
    print_result_box(player_name, credibility, current_tick,
                     flights_landed, emergencies_resolved,
                     outcome, failure_cause);
    board.save_result(player_name, credibility, current_tick,
                      flights_landed, emergencies_resolved,
                      outcome, "leaderboard.txt");
}

// ---- Public entry points ----

void game_engine::start_new_game() {
    srand((unsigned int)time(0));
    cout << endl;
    cout << "=== NEW GAME: MayDay-404 ===" << endl;
    player_name = read_name();

    reset_state();
    max_ticks = loader.load_rules("rules.txt");
    loader.load_airspace(airspace, "airspace.txt");
    loader.load_flights(flight_registry, "flights.txt");
    loader.load_crew(flight_registry, "crew.txt");
    loader.load_scenario("scenario.txt");
    seed_missing_catalog();

    // Sync any preloaded flights into hash/trees/queues
    flight* current = flight_registry.get_head();
    while (current != 0) {
        callsign_table.insert(current);
        flight_bst.insert(current);
        flight_avl.insert(current);
        queue_by_priority(current);
        current = current->next;
    }

    cout << "Welcome, CRO " << player_name << "." << endl;
    cout << "Tick 0 - game start. Max ticks: " << max_ticks << endl;
    cout << "Credibility: " << credibility << " | Actions per turn: 2" << endl;
    print_aviation_call("Standby");
    run_game_loop();
}

void game_engine::run_game_loop() {
    while (!game_over) {
        cout << endl;
        cout << "Player: " << player_name
             << " | Tick: " << current_tick << "/" << max_ticks
             << " | Cred: " << credibility
             << " | Actions left: " << actions_remaining << endl;
        display_game_menu();
        int choice = read_int("Select game option (1-24): ", 1, 24);
        handle_option(choice);

        if (game_over) {
            break;
        }
        // After 2 paid actions the turn ends automatically
        if (actions_remaining <= 0 && choice != 23 && choice != 24) {
            cout << "No actions left - advancing tick automatically." << endl;
            advance_tick();
        }
    }
    cout << "Returning to main menu..." << endl;
}

void game_engine::handle_option(int option_code) {
    if (tower_disabled && option_code != 24) {
        cout << "[ATC Error] Tower disabled. Mission failing." << endl;
        return;
    }

    switch (option_code) {
        case 1: option_assign_runway(); break;
        case 2: option_reroute_flight(); break;
        case 3: option_process_landing(); break;
        case 4: option_manual_spawn_normal(); break;
        case 5: option_manual_spawn_emergency(); break;
        case 6: option_declare_emergency(); break;
        case 7: option_clear_war(); break;
        case 8: option_counter_cyber(); break;
        case 9: option_intercept_hostile(); break;
        case 10: option_counter_hijack(); break;
        case 11: option_medical_landing(); break;
        case 12: option_weather_reroute(); break;
        case 13: option_view_pending(); break;
        case 14: option_view_crew(); break;
        case 15: option_view_flights(); break;
        case 16: option_view_airspace(); break;
        case 17: option_radar_sweep(); break;
        case 18: option_compute_mst(); break;
        case 19: option_search_missing(); break;
        case 20: option_undo(); break;
        case 21: option_redo(); break;
        case 22: option_compare_heights(); break;
        case 23: option_end_turn(); break;
        case 24: option_exit_to_menu(); break;
        default:
            cout << "[ATC Error] Unknown option." << endl;
            apply_wrong_command(0);
            break;
    }

    if (!game_over && option_code != 23 && option_code != 24) {
        consume_action_if_needed(option_code);
    }
}

// ---- Option handlers ----

void game_engine::option_assign_runway() {
    cout << "Assign Runway" << endl;
    string callsign;
    cout << "Enter callsign: ";
    cin >> callsign;
    clear_input_buffer();
    flight* f = callsign_table.find_flight(callsign);
    if (f == 0) {
        apply_wrong_command(1);
        return;
    }
    string runway;
    cout << "Enter runway (RWY09/RWY12/RWY04/RWY17): ";
    cin >> runway;
    clear_input_buffer();
    if (airspace.get_sector_index(runway) < 0 || !airspace.is_runway(
            airspace.get_sector_index(runway))) {
        apply_wrong_command(1);
        return;
    }
    if (!sweep->is_reachable(f->current_sector, runway)) {
        cout << "[ATC Error] Runway unreachable from current sector." << endl;
        apply_wrong_command(1);
        return;
    }
    f->assigned_runway = runway;
    // Mirror into registry head copy if present
    flight* reg = flight_registry.get_head();
    while (reg != 0) {
        if (reg->callsign == callsign) {
            reg->assigned_runway = runway;
            break;
        }
        reg = reg->next;
    }
    print_aviation_call("Wilco");
    cout << callsign << " assigned " << runway << endl;
}

void game_engine::option_reroute_flight() {
    cout << "Reroute Flight" << endl;
    string callsign;
    cout << "Enter callsign: ";
    cin >> callsign;
    clear_input_buffer();
    flight* f = callsign_table.find_flight(callsign);
    if (f == 0) {
        apply_wrong_command(2);
        return;
    }
    string new_runway = pick_random_runway(false);
    cout << "Enter new runway target: ";
    cin >> new_runway;
    clear_input_buffer();
    if (!sweep->is_reachable(f->current_sector, new_runway)) {
        apply_wrong_command(2);
        return;
    }
    f->assigned_runway = new_runway;
    flight* reg = flight_registry.get_head();
    while (reg != 0) {
        if (reg->callsign == callsign) {
            reg->assigned_runway = new_runway;
            break;
        }
        reg = reg->next;
    }
    print_aviation_call("Vector");
    route r = router->find_fastest_route(f->current_sector, new_runway);
    router->print_route(r);
    if (r.sectors != 0) {
        delete[] r.sectors;
    }
}

void game_engine::process_landing_action() {
    flight* f = landing_system.process_next_landing();
    if (f == 0) {
        cout << "[ATC Error] No flights pending landing." << endl;
        apply_wrong_command(3);
        return;
    }
    string runway = f->assigned_runway;
    bool emergency = f->priority_level != NORMAL;

    // Emergency capable runways: 09 and 12 only
    if (emergency) {
        if (runway != "RWY09" && runway != "RWY12") {
            cout << "Emergency landing on non-capable runway " << runway
                 << " (-10 credibility)." << endl;
            apply_credibility_delta(-10);
        }
    }

    print_aviation_call("Cleared for Landing");
    cout << f->callsign << " landed on " << runway << endl;
    unregister_flight_everywhere(f->callsign);
    flights_landed = flights_landed + 1;
    apply_credibility_delta(2);

    // Mark matching emergency resolved without extra reward here
    active_emergency* em = emergency_head;
    while (em != 0) {
        if (!em->resolved && em->callsign == f->callsign) {
            em->resolved = true;
            emergencies_resolved = emergencies_resolved + 1;
            break;
        }
        em = em->next;
    }
    delete f;
}

void game_engine::option_process_landing() {
    cout << "Process Landing" << endl;
    process_landing_action();
}

void game_engine::option_manual_spawn_normal() {
    cout << "Manual Spawn - Normal Flight (test)" << endl;
    string sector = pick_random_sector();
    string runway = pick_random_runway(false);
    string callsign = "TN" + string(1, (char)('0' + random_range(1, 9)))
                    + string(1, (char)('0' + random_range(0, 9)));
    flight* f = create_flight_template(callsign, NORMAL, sector, runway);
    register_flight_everywhere(f);
    queue_by_priority(f);
    print_aviation_call("Roger");
    cout << "Spawned normal flight " << callsign << endl;
}

void game_engine::option_manual_spawn_emergency() {
    cout << "Manual Spawn - Emergency (test)" << endl;
    cout << "Select priority: 1=WAR 2=HIJACK 3=CYBER 5=FUEL 6=MEDICAL 7=WEATHER 8=MECHANICAL" << endl;
    int p = read_int("Priority: ", 1, 8);
    if (p == MISSING) {
        p = FUEL;
    }
    string sector = pick_random_sector();
    string runway = pick_random_runway(true);
    string callsign = "TE" + string(1, (char)('0' + random_range(1, 9)))
                    + string(1, (char)('0' + random_range(0, 9)));
    flight* f = create_flight_template(callsign, p, sector, runway);
    f->status = "EMERGENCY";
    register_flight_everywhere(f);
    queue_by_priority(f);
    print_aviation_call("Mayday");

    if (p == HIJACK) {
        int deadline = 2;
        string actor = pick_hijack_actor(deadline);
        add_active_emergency(callsign, HIJACK, deadline, actor);
        announce_threat("HIJACK", actor, deadline);
    } else if (p == CYBER) {
        int deadline = 2;
        string actor = pick_cyber_actor(deadline);
        add_active_emergency(callsign, CYBER, deadline, actor);
        announce_threat("CYBER", actor, deadline);
    } else if (p == WAR) {
        add_active_emergency(callsign, WAR, 2, "NullAir Hostile");
        announce_threat("WAR", "NullAir Hostile", 2);
    } else {
        int deadline = 5;
        if (p == MEDICAL || p == WEATHER) {
            deadline = 4;
        }
        add_active_emergency(callsign, p, deadline, "Manual");
    }
    cout << "Spawned emergency " << callsign << endl;
}

void game_engine::option_declare_emergency() {
    cout << "Declare Emergency Priority" << endl;
    string callsign;
    cout << "Enter callsign: ";
    cin >> callsign;
    clear_input_buffer();
    flight* f = callsign_table.find_flight(callsign);
    if (f == 0) {
        apply_wrong_command(6);
        return;
    }
    int p = read_int("New priority (1-8): ", 1, 8);
    f->priority_level = p;
    f->status = "EMERGENCY";
    flight* reg = flight_registry.get_head();
    while (reg != 0) {
        if (reg->callsign == callsign) {
            reg->priority_level = p;
            reg->status = "EMERGENCY";
            break;
        }
        reg = reg->next;
    }
    landing_system.insert_emergency(f);
    print_aviation_call("Pan-Pan");
}

void game_engine::option_clear_war() {
    push_undo_snapshot("CLEAR_WAR");
    cout << "Clear War Aircraft" << endl;
    string callsign;
    cout << "Enter hostile callsign (or - to auto): ";
    cin >> callsign;
    clear_input_buffer();
    if (callsign == "-") {
        callsign = "";
    }
    active_emergency* em = find_emergency(callsign, WAR);
    if (em == 0) {
        apply_wrong_command(WAR);
        return;
    }
    print_aviation_call("Wilco");
    resolve_emergency(em->callsign, WAR, 30);
    unregister_flight_everywhere(em->callsign);
}

void game_engine::option_counter_cyber() {
    push_undo_snapshot("COUNTER_CYBER");
    cout << "Counter Cyber Attack" << endl;
    string callsign;
    cout << "Enter related callsign (or - to auto): ";
    cin >> callsign;
    clear_input_buffer();
    if (callsign == "-") {
        callsign = "";
    }
    active_emergency* em = find_emergency(callsign, CYBER);
    if (em == 0) {
        apply_wrong_command(CYBER);
        return;
    }
    print_aviation_call("Roger");
    resolve_emergency(em->callsign, CYBER, 15);
}

void game_engine::option_intercept_hostile() {
    push_undo_snapshot("INTERCEPT_HOSTILE");
    cout << "Intercept Hostile" << endl;
    string callsign;
    cout << "Enter hostile callsign (or - to auto): ";
    cin >> callsign;
    clear_input_buffer();
    if (callsign == "-") {
        callsign = "";
    }
    active_emergency* em = find_emergency(callsign, WAR);
    if (em == 0) {
        // Intercept can also finish a hijack in some scenarios
        em = find_emergency(callsign, HIJACK);
        if (em == 0) {
            apply_wrong_command(WAR);
            return;
        }
        print_aviation_call("Wilco");
        resolve_emergency(em->callsign, HIJACK, 20);
        return;
    }
    print_aviation_call("Wilco");
    resolve_emergency(em->callsign, WAR, 30);
    unregister_flight_everywhere(em->callsign);
}

void game_engine::option_counter_hijack() {
    push_undo_snapshot("COUNTER_HIJACK");
    cout << "Counter Hijack" << endl;
    string callsign;
    cout << "Enter callsign (or - to auto): ";
    cin >> callsign;
    clear_input_buffer();
    if (callsign == "-") {
        callsign = "";
    }
    active_emergency* em = find_emergency(callsign, HIJACK);
    if (em == 0) {
        apply_wrong_command(HIJACK);
        return;
    }
    print_aviation_call("Roger");
    resolve_emergency(em->callsign, HIJACK, 20);
}

void game_engine::option_medical_landing() {
    push_undo_snapshot("MEDICAL_LANDING");
    cout << "Medical Priority Landing" << endl;
    string callsign;
    cout << "Enter callsign: ";
    cin >> callsign;
    clear_input_buffer();
    active_emergency* em = find_emergency(callsign, MEDICAL);
    flight* f = callsign_table.find_flight(callsign);
    if (f == 0) {
        apply_wrong_command(MEDICAL);
        return;
    }
    // Force emergency-capable runway
    string landing_runway = pick_random_runway(true);
    f->assigned_runway = landing_runway;
    f->priority_level = MEDICAL;
    landing_system.insert_emergency(f);
    print_aviation_call("Cleared for Landing");
    if (em != 0) {
        em->resolved = true;
        emergencies_resolved = emergencies_resolved + 1;
    }
    unregister_flight_everywhere(callsign);
    flights_landed = flights_landed + 1;
    apply_credibility_delta(2);
    cout << callsign << " medical landing complete on "
         << landing_runway << endl;
}

void game_engine::option_weather_reroute() {
    cout << "Weather Reroute" << endl;
    string callsign;
    cout << "Enter callsign: ";
    cin >> callsign;
    clear_input_buffer();
    flight* f = callsign_table.find_flight(callsign);
    if (f == 0) {
        apply_wrong_command(WEATHER);
        return;
    }
    string new_runway = pick_random_runway(true);
    if (!sweep->is_reachable(f->current_sector, new_runway)) {
        apply_wrong_command(WEATHER);
        return;
    }
    f->assigned_runway = new_runway;
    active_emergency* em = find_emergency(callsign, WEATHER);
    if (em != 0) {
        em->resolved = true;
        emergencies_resolved = emergencies_resolved + 1;
    }
    print_aviation_call("Vector");
    cout << callsign << " weather-rerouted to " << new_runway << endl;
}

void game_engine::option_view_pending() {
    cout << "View Pending Arrivals (free)" << endl;
    landing_system.display_normal_queue();
    landing_system.display_heap();
}

void game_engine::option_view_crew() {
    cout << "View Crew (free)" << endl;
    flight_registry.display_all_crew();
}

void game_engine::option_view_flights() {
    cout << "View Flight Records (free)" << endl;
    flight_registry.display_all_flights();
    callsign_table.display_stats();
}

void game_engine::option_view_airspace() {
    cout << "View Airspace Network (free)" << endl;
    airspace.display_adjacency_list();
    airspace.display_adjacency_matrix();
}

void game_engine::option_radar_sweep() {
    cout << "Radar Sweep" << endl;
    string start;
    cout << "Start sector: ";
    cin >> start;
    clear_input_buffer();
    sweep->bfs_from(start);
    sweep->dfs_from(start);
}

void game_engine::option_compute_mst() {
    cout << "Compute Radar MST" << endl;
    mst_tool->compute_mst();
    mst_tool->display_mst();
    mst_tool->verify_connectivity();
}

void game_engine::option_search_missing() {
    cout << "Search Missing Flight" << endl;
    string name;
    cout << "Enter missing flight name (MH370/PK404/FT739/GA542): ";
    cin >> name;
    clear_input_buffer();

    missing_flight_record* miss = missing_head;
    while (miss != 0) {
        if (miss->flight_name == name) {
            break;
        }
        miss = miss->next;
    }
    if (miss == 0 || miss->located || miss->spawn_tick < 0) {
        cout << "[ATC Error] That missing flight is not active." << endl;
        apply_wrong_command(MISSING);
        return;
    }
    if (credibility < miss->credibility_threshold) {
        cout << "[ATC Error] Credibility " << credibility
             << " below threshold " << miss->credibility_threshold << endl;
        apply_wrong_command(MISSING);
        return;
    }

    bool found = sweep->search_missing_flight("Control_Tower",
                                              miss->last_known_sector);
    if (found) {
        miss->located = true;
        apply_credibility_delta(20);
        emergencies_resolved = emergencies_resolved + 1;
        print_aviation_call("Roger");
        cout << "Missing flight " << miss->flight_name << " located." << endl;
    } else {
        apply_wrong_command(MISSING);
    }
}

void game_engine::option_undo() {
    game_snapshot* snap = history_log.undo();
    if (snap == 0) {
        apply_wrong_command(20);
        return;
    }
    credibility = snap->credibility;
    flights_landed = snap->flights_landed;
    emergencies_resolved = snap->emergencies_resolved;
    clamp_credibility();
}

void game_engine::option_redo() {
    game_snapshot* snap = history_log.redo();
    if (snap == 0) {
        apply_wrong_command(21);
        return;
    }
    credibility = snap->credibility;
    flights_landed = snap->flights_landed;
    emergencies_resolved = snap->emergencies_resolved;
    clamp_credibility();
}

void game_engine::option_compare_heights() {
    cout << "BST vs AVL Heights (free)" << endl;
    cout << "BST height: " << flight_bst.height() << endl;
    cout << "AVL height: " << flight_avl.height() << endl;
    flight_bst.inorder_traversal();
    flight_avl.inorder_traversal();
}

void game_engine::option_end_turn() {
    cout << "End Turn / Advance Tick" << endl;
    print_aviation_call("Standby");
    advance_tick();
}

void game_engine::option_exit_to_menu() {
    cout << "Exit to Main Menu (counts as LOSE)." << endl;
    end_game(false, "Player exited to main menu");
}
