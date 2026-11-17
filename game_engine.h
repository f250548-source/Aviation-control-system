#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "avl_flight.h"
#include "bst_flight.h"
#include "command_history.h"
#include "emergency_landing.h"
#include "file_loader.h"
#include "flight.h"
#include "hash_table.h"
#include "leader_board.h"
#include "radar_mst.h"
#include "radar_sweep.h"
#include "registration.h"
#include "route_finder.h"
#include "space_graph.h"
#include <iostream>
#include <string>
using namespace std;

// Active emergency / threat tracked for escalation timers
struct active_emergency {
    string callsign;
    int event_type;
    int spawn_tick;
    int deadline_ticks;
    string threat_actor;
    bool resolved;
    active_emergency* next;
};

// Missing flight kept outside registry / hash / trees (no fuel burn)
struct missing_flight_record {
    string flight_name;
    string last_known_sector;
    int tick_window;
    int credibility_threshold;
    int spawn_tick;
    bool located;
    missing_flight_record* next;
};

// Module 10: tick-based ATC simulation engine
class game_engine {
private:
    string player_name;
    int current_tick;
    int max_ticks;
    int credibility;
    int actions_remaining;
    int flights_landed;
    int emergencies_resolved;
    bool game_over;
    bool mission_won;
    string failure_cause;
    bool tower_disabled;
    bool cyber_corridor_disabled;
    int last_spawn_tick;
    int next_spawn_interval;

    // Wrong-command penalty grows by type (base -5, then -10, ...)
    int wrong_penalty_by_type[32];

    // Teammate / shared modules
    registration flight_registry;
    hash_table callsign_table;
    emergency_landing landing_system;
    command_history history_log;
    file_loader loader;
    leader_board board;
    bst_flight flight_bst;
    avl_flight flight_avl;
    space_graph airspace;
    radar_sweep* sweep;
    route_finder* router;
    radar_mst* mst_tool;

    active_emergency* emergency_head;
    missing_flight_record* missing_head;

    // ---- Internal helpers ----
    void reset_state();
    void free_emergency_list();
    void free_missing_list();
    void clamp_credibility();
    void apply_credibility_delta(int delta);
    void apply_wrong_command(int penalty_slot);
    bool is_free_action(int option_code);
    void consume_action_if_needed(int option_code);
    void push_undo_snapshot(string action_label);
    void register_flight_everywhere(flight* f);
    void unregister_flight_everywhere(string callsign);
    void queue_by_priority(flight* f);
    flight* create_flight_template(string callsign, int priority_level,
                                   string sector, string runway);
    void add_active_emergency(string callsign, int event_type,
                              int deadline_ticks, string threat_actor);
    active_emergency* find_emergency(string callsign, int event_type);
    void resolve_emergency(string callsign, int event_type, int reward);
    void seed_missing_catalog();
    void announce_threat(string event_label, string actor, int ticks);
    string pick_hijack_actor(int& deadline_out);
    string pick_cyber_actor(int& deadline_out);
    string pick_random_sector();
    string pick_random_runway(bool emergency_capable_only);
    int random_range(int low, int high);

    // Per-tick pipeline
    void auto_move_flights();
    void apply_escalation();
    void auto_spawn();
    void check_win_lose();
    void advance_tick();

    // Landing helper
    void process_landing_action();

    // Option handlers 01-24
    void handle_option(int option_code);
    void option_assign_runway();
    void option_reroute_flight();
    void option_process_landing();
    void option_manual_spawn_normal();
    void option_manual_spawn_emergency();
    void option_declare_emergency();
    void option_clear_war();
    void option_counter_cyber();
    void option_intercept_hostile();
    void option_counter_hijack();
    void option_medical_landing();
    void option_weather_reroute();
    void option_view_pending();
    void option_view_crew();
    void option_view_flights();
    void option_view_airspace();
    void option_radar_sweep();
    void option_compute_mst();
    void option_search_missing();
    void option_undo();
    void option_redo();
    void option_compare_heights();
    void option_end_turn();
    void option_exit_to_menu();

    void end_game(bool won, string cause);

public:
    game_engine();
    ~game_engine();

    // Prompt for name, load data, reset state, enter loop
    void start_new_game();

    // Main in-game loop until win/lose/exit
    void run_game_loop();
};

#endif
