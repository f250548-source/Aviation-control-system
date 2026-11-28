#include "game_engine.h"
#include "menu.h"
#include "utils.h"
#include <cstdlib>
#include <ctime>
#include <fstream>
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
    crash_count = 0;
    hijack_count = 0;
    breach_count = 0;
    game_over = false;
    mission_won = false;
    failure_cause = "";
    tower_disabled = false;
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

// Soft reset of counters for a new mission
void game_engine::reset_state() {
    current_tick = 0;
    credibility = 100;
    actions_remaining = 2;
    flights_landed = 0;
    emergencies_resolved = 0;
    crash_count = 0;
    hijack_count = 0;
    breach_count = 0;
    game_over = false;
    mission_won = false;
    failure_cause = "";
    tower_disabled = false;
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

// Free actions cost no action points (23 still ends the turn)
bool game_engine::is_free_action(int option_code) {
    return option_code == 13 || option_code == 14 || option_code == 15
        || option_code == 16 || option_code == 22 || option_code == 23;
}

void game_engine::consume_action_if_needed(int option_code) {
    if (option_code == 24) {
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

// Log + push snapshot (stores runway/priority/sector/status when f is set)
void game_engine::push_undo_snapshot(string action_label, flight* f) {
    history_log.log_command(action_label, current_tick);
    history_log.push_snapshot(current_tick, credibility, flights_landed,
                              emergencies_resolved, action_label, f);
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

void game_engine::print_flight_record(flight* f) {
    if (f == 0) {
        cout << "[ATC Error] No flight record." << endl;
        return;
    }
    cout << "Callsign: " << f->callsign << endl;
    cout << "Type: " << f->aircraft_type << " | Airline: " << f->airline << endl;
    cout << "From: " << f->origin << " -> " << f->destination << endl;
    cout << "Passengers: " << f->passenger_count
         << " | Fuel: " << f->fuel_remaining << endl;
    cout << "Sector: " << f->current_sector
         << " | Runway: " << f->assigned_runway << endl;
    cout << "Priority: " << f->priority_level
         << " | Status: " << f->status
         << " | Arrival tick: " << f->arrival_tick << endl;
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
    flight_registry.add_flight(f);
    callsign_table.insert(f);
    flight_bst.insert(f);
    flight_avl.insert(f);
}

// Drop indexes FIRST, then delete the owned registry node.
// Deleting first left dangling pointers in the hash table and crashed
// on the next tick when find/insert probed an OCCUPIED dead slot.
void game_engine::unregister_flight_everywhere(string callsign) {
    callsign_table.remove(callsign);
    flight_bst.remove(callsign);
    flight_avl.remove(callsign);
    flight_registry.remove_flight(callsign);
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
    node->corridor_disabled = false;
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
    string crash_list[64];
    int crash_list_count = 0;

    while (current != 0) {
        if (current->status == "ACTIVE" || current->status == "EMERGENCY") {
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
                crash_count = crash_count + 1;
                history_log.log_command(
                    "CRASH " + current->callsign, current_tick);
                if (crash_list_count < 64) {
                    crash_list[crash_list_count] = current->callsign;
                    crash_list_count = crash_list_count + 1;
                }
            }
        }
        current = current->next;
    }

    int i = 0;
    while (i < crash_list_count) {
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
                end_game(false, "Hijack not intercepted in time ("
                         + current->threat_actor + ")");
                return;
            }
            if (type == CYBER) {
                // Corridor disabled when this group's own window ends
                if (elapsed > current->deadline_ticks
                    && !current->corridor_disabled) {
                    cout << "Cyber escalation (" << current->threat_actor
                         << "): corridor Syntax_Sector--Alpha_Tree disabled."
                         << endl;
                    airspace.remove_corridor("Syntax_Sector", "Alpha_Tree");
                    current->corridor_disabled = true;
                }
                // Tower disabled one tick after the corridor cut
                if (elapsed > current->deadline_ticks + 1) {
                    tower_disabled = true;
                    apply_credibility_delta(-15);
                    end_game(false, "Cyber attack disabled the tower");
                    return;
                }
            }
            if (type == FUEL && elapsed > 5) {
                apply_credibility_delta(-50);
                crash_count = crash_count + 1;
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
                crash_count = crash_count + 1;
                unregister_flight_everywhere(current->callsign);
                current->resolved = true;
                cout << "Weather forced crash of " << current->callsign << endl;
            }
            if (type == MECHANICAL && elapsed > 5) {
                apply_credibility_delta(-50);
                crash_count = crash_count + 1;
                unregister_flight_everywhere(current->callsign);
                current->resolved = true;
                cout << "Mechanical failure crash: " << current->callsign << endl;
            }
        }
        current = current->next;
    }

    missing_flight_record* miss = missing_head;
    while (miss != 0) {
        if (!miss->located && miss->spawn_tick >= 0) {
            int elapsed = current_tick - miss->spawn_tick;
            if (elapsed > miss->tick_window) {
                breach_count = breach_count + 1;
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

    int event_roll = random_range(1, 9);
    string sector = pick_random_sector();
    string runway = pick_random_runway(false);
    string callsign = "MD" + string(1, (char)('0' + random_range(1, 9)))
                    + string(1, (char)('0' + random_range(0, 9)))
                    + string(1, (char)('0' + random_range(0, 9)));

    if (event_roll == MISSING) {
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
        hijack_count = hijack_count + 1;
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
        end_game(true, "");
        return;
    }
}

void game_engine::advance_tick() {
    current_tick = current_tick + 1;
    actions_remaining = 2;
    cout << endl;
    cout << "======== Tick: " << current_tick
         << "/" << max_ticks << " ========" << endl;
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
    board.save_result(player_name, outcome, credibility, current_tick,
                      crash_count, hijack_count, breach_count,
                      "leaderboard.txt");
}

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

    flight* current = flight_registry.get_head();
    while (current != 0) {
        callsign_table.insert(current);
        flight_bst.insert(current);
        flight_avl.insert(current);
        queue_by_priority(current);
        current = current->next;
    }

    cout << "Welcome, CRO " << player_name << "." << endl;
    cout << "Tick: 0/" << max_ticks << " - game start." << endl;
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
        case 1: option_register_aircraft(); break;
        case 2: option_register_crew(); break;
        case 3: option_search_flight(); break;
        case 4: option_add_normal_arrival(); break;
        case 5: option_add_emergency_arrival(); break;
        case 6: option_process_landing(); break;
        case 7: option_clear_flight_for_runway(); break;
        case 8: option_reroute_flight(); break;
        case 9: option_intercept_aircraft(); break;
        case 10: option_escalate_priority(); break;
        case 11: option_counter_cyber(); break;
        case 12: option_undo_redo(); break;
        case 13: option_display_pending(); break;
        case 14: option_display_crew(); break;
        case 15: option_display_flights(); break;
        case 16: option_display_airspace(); break;
        case 17: option_bfs_radar_sweep(); break;
        case 18: option_dfs_sector_traversal(); break;
        case 19: option_search_missing(); break;
        case 20: option_find_fastest_route(); break;
        case 21: option_generate_mst(); break;
        case 22: option_compare_heights(); break;
        case 23: option_end_turn(); break;
        case 24: option_exit_to_menu(); break;
        default:
            cout << "[ATC Error] Unknown option." << endl;
            apply_wrong_command(0);
            break;
    }

    if (!game_over && option_code != 24) {
        consume_action_if_needed(option_code);
    }
}

// ---- Option handlers (manual numbering) ----

void game_engine::option_register_aircraft() {
    cout << "01 Register Aircraft" << endl;
    string callsign;
    string aircraft_type;
    string airline;
    string origin;
    string destination;
    string sector;
    string runway;
    int passengers = 0;
    int fuel = 0;
    int priority_level = NORMAL;

    cout << "Callsign: ";
    cin >> callsign;
    clear_input_buffer();
    if (callsign_table.find_flight(callsign) != 0) {
        cout << "[ATC Error] Callsign already registered." << endl;
        apply_wrong_command(1);
        return;
    }
    cout << "Aircraft type: ";
    cin >> aircraft_type;
    clear_input_buffer();
    cout << "Airline: ";
    cin >> airline;
    clear_input_buffer();
    cout << "Origin: ";
    cin >> origin;
    clear_input_buffer();
    cout << "Destination: ";
    cin >> destination;
    clear_input_buffer();
    passengers = read_int("Passenger count: ", 0, 500);
    fuel = read_int("Fuel remaining: ", 1, 50);
    cout << "Current sector: ";
    cin >> sector;
    clear_input_buffer();
    cout << "Assigned runway: ";
    cin >> runway;
    clear_input_buffer();
    priority_level = read_int("Priority (1-9, 9=NORMAL): ", 1, 9);

    if (airspace.get_sector_index(sector) < 0) {
        cout << "[ATC Error] Unknown sector." << endl;
        apply_wrong_command(1);
        return;
    }
    int runway_index = airspace.get_sector_index(runway);
    if (runway_index < 0 || !airspace.is_runway(runway_index)) {
        cout << "[ATC Error] Unknown runway." << endl;
        apply_wrong_command(1);
        return;
    }

    flight* f = new flight;
    f->callsign = callsign;
    f->aircraft_type = aircraft_type;
    f->airline = airline;
    f->origin = origin;
    f->destination = destination;
    f->passenger_count = passengers;
    f->fuel_remaining = fuel;
    f->current_sector = sector;
    f->assigned_runway = runway;
    f->arrival_tick = current_tick;
    f->priority_level = priority_level;
    f->status = (priority_level == NORMAL) ? "ACTIVE" : "EMERGENCY";
    f->next = 0;

    register_flight_everywhere(f);
    queue_by_priority(f);
    print_aviation_call("Roger");
    cout << "Aircraft " << callsign << " registered." << endl;
}

void game_engine::option_register_crew() {
    cout << "02 Register Crew Member" << endl;
    string employee_id;
    string name;
    string role;
    string assigned_callsign;
    cout << "Employee ID: ";
    cin >> employee_id;
    clear_input_buffer();
    cout << "Name (use _ for spaces): ";
    cin >> name;
    clear_input_buffer();
    cout << "Role: ";
    cin >> role;
    clear_input_buffer();
    cout << "Assigned callsign (or Unassigned): ";
    cin >> assigned_callsign;
    clear_input_buffer();

    crew* c = new crew;
    c->employee_id = employee_id;
    c->name = name;
    c->role = role;
    c->assigned_callsign = assigned_callsign;
    c->next = 0;
    flight_registry.add_crew(c);
    print_aviation_call("Wilco");
    cout << "Crew member " << name << " registered." << endl;
}

void game_engine::option_search_flight() {
    cout << "03 Search Flight by Call Sign" << endl;
    string callsign;
    cout << "Enter callsign: ";
    cin >> callsign;
    clear_input_buffer();
    flight* f = callsign_table.find_flight(callsign);
    if (f == 0) {
        cout << "[ATC Error] Flight not found in hash table." << endl;
        apply_wrong_command(3);
        return;
    }
    print_flight_record(f);
    callsign_table.display_stats();
}

void game_engine::option_add_normal_arrival() {
    cout << "04 Add Normal Arrival (test spawn)" << endl;
    string sector = pick_random_sector();
    string runway = pick_random_runway(false);
    string callsign = "TN" + string(1, (char)('0' + random_range(1, 9)))
                    + string(1, (char)('0' + random_range(0, 9)));
    flight* f = create_flight_template(callsign, NORMAL, sector, runway);
    register_flight_everywhere(f);
    queue_by_priority(f);
    print_aviation_call("Roger");
    cout << "Spawned normal arrival " << callsign << endl;
}

void game_engine::option_add_emergency_arrival() {
    cout << "05 Add Emergency Arrival (test spawn)" << endl;
    cout << "Priority: 1=WAR 2=HIJACK 3=CYBER 5=FUEL 6=MEDICAL 7=WEATHER 8=MECHANICAL"
         << endl;
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
        hijack_count = hijack_count + 1;
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

void game_engine::process_landing_action() {
    flight* f = landing_system.process_next_landing();
    if (f == 0) {
        cout << "[ATC Error] No flights pending landing." << endl;
        apply_wrong_command(6);
        return;
    }
    string runway = f->assigned_runway;
    bool emergency = f->priority_level != NORMAL;
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
    cout << "06 Process Next Landing" << endl;
    process_landing_action();
}

// 07 Clear for runway (also resolves War with option 09)
void game_engine::option_clear_flight_for_runway() {
    cout << "07 Clear Flight for Runway" << endl;
    string callsign;
    cout << "Enter callsign: ";
    cin >> callsign;
    clear_input_buffer();
    flight* f = callsign_table.find_flight(callsign);
    if (f == 0) {
        apply_wrong_command(7);
        return;
    }
    string runway;
    cout << "Enter runway (RWY09/RWY12/RWY04/RWY17): ";
    cin >> runway;
    clear_input_buffer();
    int runway_index = airspace.get_sector_index(runway);
    if (runway_index < 0 || !airspace.is_runway(runway_index)) {
        cout << "[ATC Error] Runway does not exist." << endl;
        apply_wrong_command(7);
        return;
    }
    if (!sweep->is_reachable(f->current_sector, runway)) {
        cout << "[ATC Error] Runway unreachable from current sector." << endl;
        apply_wrong_command(7);
        return;
    }

    push_undo_snapshot("CLEAR_RUNWAY " + callsign, f);
    f->assigned_runway = runway;

    // Emergency on non-capable runway costs credibility
    if (f->priority_level != NORMAL) {
        if (runway == "RWY04" || runway == "RWY17") {
            cout << "Emergency clear on " << runway << " (-10 credibility)." << endl;
            apply_credibility_delta(-10);
        }
    }

    // War resolve path (options 07 and 09)
    active_emergency* war = find_emergency(callsign, WAR);
    if (war != 0) {
        print_aviation_call("Cleared for Landing");
        resolve_emergency(callsign, WAR, 30);
        unregister_flight_everywhere(callsign);
        flights_landed = flights_landed + 1;
        apply_credibility_delta(2);
        cout << callsign << " war aircraft cleared via " << runway << endl;
        return;
    }

    // Medical / weather / other: queue for landing on the cleared runway
    if (f->priority_level == MEDICAL || f->priority_level == WEATHER
        || f->priority_level == FUEL || f->priority_level == MECHANICAL) {
        landing_system.insert_emergency(f);
    }
    print_aviation_call("Cleared for Landing");
    cout << callsign << " cleared for " << runway << endl;
}

void game_engine::option_reroute_flight() {
    cout << "08 Reroute Flight" << endl;
    string callsign;
    cout << "Enter callsign: ";
    cin >> callsign;
    clear_input_buffer();
    flight* f = callsign_table.find_flight(callsign);
    if (f == 0) {
        apply_wrong_command(8);
        return;
    }
    string new_runway;
    cout << "Enter new runway target: ";
    cin >> new_runway;
    clear_input_buffer();
    int runway_index = airspace.get_sector_index(new_runway);
    if (runway_index < 0 || !airspace.is_runway(runway_index)) {
        apply_wrong_command(8);
        return;
    }
    if (!sweep->is_reachable(f->current_sector, new_runway)) {
        apply_wrong_command(8);
        return;
    }

    push_undo_snapshot("REROUTE " + callsign, f);
    f->assigned_runway = new_runway;

    active_emergency* weather = find_emergency(callsign, WEATHER);
    if (weather != 0) {
        weather->resolved = true;
        emergencies_resolved = emergencies_resolved + 1;
    }

    print_aviation_call("Vector");
    route r = router->find_fastest_route(f->current_sector, new_runway);
    router->print_route(r);
    if (r.sectors != 0) {
        delete[] r.sectors;
    }
    cout << callsign << " rerouted to " << new_runway << endl;
}

void game_engine::option_intercept_aircraft() {
    cout << "09 Intercept Aircraft" << endl;
    string callsign;
    cout << "Enter callsign (or - to auto): ";
    cin >> callsign;
    clear_input_buffer();
    if (callsign == "-") {
        callsign = "";
    }

    active_emergency* em = find_emergency(callsign, WAR);
    if (em != 0) {
        flight* f = callsign_table.find_flight(em->callsign);
        push_undo_snapshot("INTERCEPT_WAR " + em->callsign, f);
        print_aviation_call("Wilco");
        resolve_emergency(em->callsign, WAR, 30);
        unregister_flight_everywhere(em->callsign);
        return;
    }
    em = find_emergency(callsign, HIJACK);
    if (em != 0) {
        flight* f = callsign_table.find_flight(em->callsign);
        push_undo_snapshot("INTERCEPT_HIJACK " + em->callsign, f);
        print_aviation_call("Wilco");
        resolve_emergency(em->callsign, HIJACK, 20);
        return;
    }
    apply_wrong_command(9);
}

void game_engine::option_escalate_priority() {
    cout << "10 Escalate Priority" << endl;
    string callsign;
    cout << "Enter callsign: ";
    cin >> callsign;
    clear_input_buffer();
    flight* f = callsign_table.find_flight(callsign);
    if (f == 0) {
        apply_wrong_command(10);
        return;
    }
    int p = read_int("New priority (1-8): ", 1, 8);
    push_undo_snapshot("ESCALATE " + callsign, f);
    f->priority_level = p;
    f->status = "EMERGENCY";
    landing_system.insert_emergency(f);
    print_aviation_call("Pan-Pan");
    cout << callsign << " priority escalated to " << p << endl;
}

void game_engine::option_counter_cyber() {
    cout << "11 Counter Cyber Attack" << endl;
    string callsign;
    cout << "Enter related callsign (or - to auto): ";
    cin >> callsign;
    clear_input_buffer();
    if (callsign == "-") {
        callsign = "";
    }
    active_emergency* em = find_emergency(callsign, CYBER);
    if (em == 0) {
        apply_wrong_command(11);
        return;
    }
    flight* f = callsign_table.find_flight(em->callsign);
    push_undo_snapshot("COUNTER_CYBER " + em->callsign, f);
    print_aviation_call("Roger");
    resolve_emergency(em->callsign, CYBER, 15);
}

void game_engine::option_undo_redo() {
    cout << "12 Undo / Redo" << endl;
    int choice = read_int("1=Undo  2=Redo: ", 1, 2);
    if (choice == 1) {
        game_snapshot* snap = history_log.undo();
        if (snap == 0) {
            return;
        }
        if (snap->callsign.length() > 0) {
            flight* f = callsign_table.find_flight(snap->callsign);
            if (f == 0) {
                history_log.redo();
                cout << "[ATC Error] Flight no longer exists for undo." << endl;
                return;
            }
            swap_strings(f->assigned_runway, snap->prev_runway);
            swap_strings(f->current_sector, snap->prev_sector);
            swap_strings(f->status, snap->prev_status);
            int p = f->priority_level;
            f->priority_level = snap->prev_priority;
            snap->prev_priority = p;
        }
        int temp = credibility;
        credibility = snap->credibility;
        snap->credibility = temp;
        temp = flights_landed;
        flights_landed = snap->flights_landed;
        snap->flights_landed = temp;
        temp = emergencies_resolved;
        emergencies_resolved = snap->emergencies_resolved;
        snap->emergencies_resolved = temp;
        clamp_credibility();
        cout << "Command undone." << endl;
    } else {
        game_snapshot* snap = history_log.redo();
        if (snap == 0) {
            return;
        }
        if (snap->callsign.length() > 0) {
            flight* f = callsign_table.find_flight(snap->callsign);
            if (f == 0) {
                history_log.undo();
                cout << "[ATC Error] Flight no longer exists for redo." << endl;
                return;
            }
            swap_strings(f->assigned_runway, snap->prev_runway);
            swap_strings(f->current_sector, snap->prev_sector);
            swap_strings(f->status, snap->prev_status);
            int p = f->priority_level;
            f->priority_level = snap->prev_priority;
            snap->prev_priority = p;
        }
        int temp = credibility;
        credibility = snap->credibility;
        snap->credibility = temp;
        temp = flights_landed;
        flights_landed = snap->flights_landed;
        snap->flights_landed = temp;
        temp = emergencies_resolved;
        emergencies_resolved = snap->emergencies_resolved;
        snap->emergencies_resolved = temp;
        clamp_credibility();
        cout << "Command redone." << endl;
    }
}

void game_engine::option_display_pending() {
    cout << "13 Display Pending Arrivals (free)" << endl;
    landing_system.display_normal_queue();
    landing_system.display_heap();
}

void game_engine::option_display_crew() {
    cout << "14 Display Crew Members (free)" << endl;
    flight_registry.display_all_crew();
}

void game_engine::option_display_flights() {
    cout << "15 Display Flight Records (free)" << endl;
    flight_registry.display_all_flights();
    callsign_table.display_stats();
}

void game_engine::option_display_airspace() {
    cout << "16 Display Airspace Network (free)" << endl;
    airspace.display_adjacency_list();
    airspace.display_adjacency_matrix();
}

void game_engine::option_bfs_radar_sweep() {
    cout << "17 BFS Radar Sweep" << endl;
    sweep->bfs_from("Control_Tower");
    cout << "Sectors within 2 hops of Control_Tower:" << endl;
    sweep->bfs_within_2_hops("Control_Tower");
}

void game_engine::option_dfs_sector_traversal() {
    cout << "18 DFS Sector Traversal" << endl;
    string start;
    cout << "Start sector: ";
    cin >> start;
    clear_input_buffer();
    if (airspace.get_sector_index(start) < 0) {
        cout << "[ATC Error] Unknown sector." << endl;
        apply_wrong_command(18);
        return;
    }
    sweep->dfs_from(start);

    cout << "Reachability checks:" << endl;
    bool rwy09 = sweep->is_reachable("Alpha_Tree", "RWY09");
    bool rwy12 = sweep->is_reachable("Alpha_Tree", "RWY12");
    cout << "Alpha_Tree -> RWY09: " << (rwy09 ? "reachable" : "NOT reachable")
         << endl;
    cout << "Alpha_Tree -> RWY12: " << (rwy12 ? "reachable" : "NOT reachable")
         << endl;

    missing_flight_record* miss = missing_head;
    while (miss != 0) {
        bool ok = sweep->is_reachable(miss->last_known_sector, "Restricted_Zone");
        cout << "Restricted_Zone from last-known of " << miss->flight_name
             << " (" << miss->last_known_sector << "): "
             << (ok ? "reachable" : "NOT reachable") << endl;
        miss = miss->next;
    }
}

void game_engine::option_search_missing() {
    cout << "19 Search for Missing Flight" << endl;
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

void game_engine::option_find_fastest_route() {
    cout << "20 Find Fastest Route to Runway" << endl;
    string source;
    string runway;
    cout << "Source sector: ";
    cin >> source;
    clear_input_buffer();
    cout << "Destination runway: ";
    cin >> runway;
    clear_input_buffer();

    if (airspace.get_sector_index(source) < 0) {
        cout << "[ATC Error] Unknown source sector." << endl;
        apply_wrong_command(20);
        return;
    }
    int runway_index = airspace.get_sector_index(runway);
    if (runway_index < 0 || !airspace.is_runway(runway_index)) {
        cout << "[ATC Error] Destination is not a valid runway." << endl;
        apply_wrong_command(20);
        return;
    }

    route r = router->find_fastest_route(source, runway);
    if (r.sector_count == 0 || r.sectors == 0) {
        cout << "[ATC Error] No route found." << endl;
        apply_wrong_command(20);
        return;
    }
    router->print_route(r);
    cout << "Total cost: " << r.total_weight << " seconds." << endl;
    delete[] r.sectors;
}

void game_engine::option_generate_mst() {
    cout << "21 Generate Minimum Radar Coverage (Kruskal MST)" << endl;
    mst_tool->compute_mst();
    mst_tool->display_mst();
    mst_tool->verify_connectivity();
}

void game_engine::option_compare_heights() {
    cout << "22 Compare BST and AVL Heights (free)" << endl;
    string demo_path = resolve_data_file("avl_demo.txt");
    ifstream input(demo_path.c_str());
    if (!input.is_open()) {
        cout << "[ATC Error] Could not open avl_demo.txt - using built-in demo."
             << endl;
        compare_bst_avl_heights_demo();
        return;
    }

    string callsigns[15];
    int loaded = 0;
    string token;
    while (loaded < 15 && (input >> token)) {
        callsigns[loaded] = token;
        loaded = loaded + 1;
    }
    input.close();
    if (loaded < 15) {
        cout << "[ATC Error] avl_demo.txt needs 15 callsigns." << endl;
        apply_wrong_command(22);
        return;
    }

    flight demo_flights[15];
    bst_flight demo_bst;
    avl_flight demo_avl;
    cout << "#  Callsign  BST height  AVL height" << endl;
    int i = 0;
    while (i < 15) {
        demo_flights[i].callsign = callsigns[i];
        demo_flights[i].aircraft_type = "A320";
        demo_flights[i].airline = "DemoAir";
        demo_flights[i].origin = "Demo";
        demo_flights[i].destination = "RWY09";
        demo_flights[i].passenger_count = 0;
        demo_flights[i].fuel_remaining = 0;
        demo_flights[i].current_sector = "Control_Tower";
        demo_flights[i].assigned_runway = "RWY09";
        demo_flights[i].arrival_tick = 0;
        demo_flights[i].priority_level = NORMAL;
        demo_flights[i].status = "DEMO";
        demo_flights[i].next = 0;
        demo_bst.insert(&demo_flights[i]);
        demo_avl.insert(&demo_flights[i]);
        cout << (i + 1) << "  " << callsigns[i]
             << "  " << demo_bst.height()
             << "  " << demo_avl.height() << endl;
        i = i + 1;
    }
    cout << "Final BST height: " << demo_bst.height()
         << "  |  Final AVL height: " << demo_avl.height() << endl;
}

void game_engine::option_end_turn() {
    cout << "23 End Turn / Advance Tick" << endl;
    print_aviation_call("Standby");
    advance_tick();
}

void game_engine::option_exit_to_menu() {
    cout << "24 Exit to Main Menu (counts as LOSE)." << endl;
    end_game(false, "Player exited to main menu");
}
