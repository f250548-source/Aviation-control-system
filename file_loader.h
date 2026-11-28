#ifndef FILE_LOADER_H
#define FILE_LOADER_H

#include "flight.h"
#include "registration.h"
#include "space_graph.h"
#include "utils.h"
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

// Loads text data files into the game structures (no <sstream>)
class file_loader {
public:
    file_loader() {
    }

    ~file_loader() {
    }

    // Load flights from flights.txt into registration
    bool load_flights(registration& reg, string filename) {
        string path = resolve_data_file(filename);
        ifstream input(path.c_str());
        if (!input.is_open()) {
            cout << "[ATC Error] Could not open " << filename << endl;
            return false;
        }
        string callsign;
        string aircraft_type;
        string airline;
        string origin;
        string destination;
        int passenger_count;
        int fuel_remaining;
        string current_sector;
        string assigned_runway;
        int priority_level;
        while (input >> callsign >> aircraft_type >> airline >> origin >> destination
                     >> passenger_count >> fuel_remaining >> current_sector
                     >> assigned_runway >> priority_level) {
            flight* f = new flight;
            f->callsign = callsign;
            f->aircraft_type = aircraft_type;
            f->airline = airline;
            f->origin = origin;
            f->destination = destination;
            f->passenger_count = passenger_count;
            f->fuel_remaining = fuel_remaining;
            f->current_sector = current_sector;
            f->assigned_runway = assigned_runway;
            f->arrival_tick = 0;
            f->priority_level = priority_level;
            f->status = "ACTIVE";
            f->next = 0;
            reg.add_flight(f);
        }
        input.close();
        return true;
    }

    // Load crew from crew.txt into registration
    bool load_crew(registration& reg, string filename) {
        string path = resolve_data_file(filename);
        ifstream input(path.c_str());
        if (!input.is_open()) {
            cout << "[ATC Error] Could not open " << filename << endl;
            return false;
        }
        string employee_id;
        string name;
        string role;
        string assigned_callsign;
        while (input >> employee_id >> name >> role >> assigned_callsign) {
            crew* c = new crew;
            c->employee_id = employee_id;
            c->name = name;
            c->role = role;
            c->assigned_callsign = assigned_callsign;
            c->next = 0;
            reg.add_crew(c);
        }
        input.close();
        return true;
    }

    // Load airspace, then run the manual topology checks
    bool load_airspace(space_graph& graph, string filename) {
        string path = resolve_data_file(filename);
        ifstream input(path.c_str());
        if (!input.is_open()) {
            cout << "[ATC Error] Could not open " << filename
                 << " - using default airspace." << endl;
            build_default_airspace(graph);
            graph.validate_manual_rules();
            return false;
        }
        string command;
        while (input >> command) {
            if (command == "SECTOR") {
                string name;
                int is_runway_flag;
                input >> name >> is_runway_flag;
                graph.add_sector(name, is_runway_flag != 0);
            } else if (command == "CORRIDOR") {
                string from_name;
                string to_name;
                int weight;
                input >> from_name >> to_name >> weight;
                graph.add_corridor(from_name, to_name, weight);
            }
        }
        input.close();
        if (graph.get_sector_count() == 0) {
            build_default_airspace(graph);
        }
        graph.validate_manual_rules();
        return true;
    }

    // Parse scenario lines with >> only (skips # comment lines manually)
    scenario_event* load_scenario(string filename, int& count) {
        count = 0;
        string path = resolve_data_file(filename);
        ifstream input(path.c_str());
        if (!input.is_open()) {
            cout << "[ATC Error] Could not open " << filename << endl;
            return 0;
        }

        // Count non-empty, non-comment lines first
        string line;
        int capacity = 0;
        while (getline(input, line)) {
            if (line.length() == 0) {
                continue;
            }
            if (line[0] == '#') {
                continue;
            }
            capacity = capacity + 1;
        }
        input.clear();
        input.seekg(0);

        if (capacity <= 0) {
            input.close();
            return 0;
        }

        scenario_event* events = new scenario_event[capacity];
        while (getline(input, line)) {
            if (line.length() == 0 || line[0] == '#') {
                continue;
            }
            // Manual parse: extract five whitespace-separated fields
            string tokens[5];
            int token_count = 0;
            string current = "";
            int i = 0;
            while (i < (int)line.length() && token_count < 5) {
                char ch = line[i];
                if (ch == ' ' || ch == '\t') {
                    if (current.length() > 0) {
                        tokens[token_count] = current;
                        token_count = token_count + 1;
                        current = "";
                    }
                } else {
                    current = current + ch;
                }
                i = i + 1;
            }
            if (current.length() > 0 && token_count < 5) {
                tokens[token_count] = current;
                token_count = token_count + 1;
            }
            if (token_count < 5) {
                continue;
            }

            // Parse tick digits manually
            int tick_value = 0;
            bool tick_ok = true;
            int t = 0;
            while (t < (int)tokens[0].length()) {
                if (tokens[0][t] < '0' || tokens[0][t] > '9') {
                    tick_ok = false;
                    break;
                }
                tick_value = tick_value * 10 + (tokens[0][t] - '0');
                t = t + 1;
            }
            if (!tick_ok) {
                continue;
            }

            events[count].tick = tick_value;
            events[count].event_type = tokens[1];
            events[count].callsign = tokens[2];
            events[count].sector = tokens[3];
            events[count].extra = tokens[4];
            count = count + 1;
        }
        input.close();
        return events;
    }

    bool load_scenario(string filename) {
        int count = 0;
        scenario_event* events = load_scenario(filename, count);
        if (events == 0) {
            return false;
        }
        cout << "Scenario events loaded: " << count << endl;
        delete[] events;
        return true;
    }

    // Load max ticks and other rules; returns max_ticks (default 30)
    int load_rules(string filename) {
        string path = resolve_data_file(filename);
        ifstream input(path.c_str());
        if (!input.is_open()) {
            cout << "[ATC Error] Could not open " << filename
                 << " - using default max ticks 30." << endl;
            return 30;
        }
        int max_ticks = 30;
        string key;
        while (input >> key) {
            if (key == "MAX_TICKS") {
                input >> max_ticks;
            }
        }
        input.close();
        if (max_ticks <= 0) {
            max_ticks = 30;
        }
        return max_ticks;
    }

    // Print about.txt contents
    void load_about(string filename) {
        string path = resolve_data_file(filename);
        ifstream input(path.c_str());
        if (!input.is_open()) {
            cout << "[ATC Error] Could not open " << filename << endl;
            cout << "MayDay-404: Air Traffic Control Simulation" << endl;
            return;
        }
        string line;
        while (getline(input, line)) {
            cout << line << endl;
        }
        input.close();
    }

    // Print gamemenu.txt contents
    void load_game_menu(string filename) {
        string path = resolve_data_file(filename);
        ifstream input(path.c_str());
        if (!input.is_open()) {
            cout << "[ATC Error] Could not open " << filename << endl;
            print_default_game_menu();
            return;
        }
        string line;
        while (getline(input, line)) {
            cout << line << endl;
        }
        input.close();
    }

private:
    // Fallback airspace matching the project-manual topology
    void build_default_airspace(space_graph& graph) {
        graph.add_sector("Control_Tower", false);
        graph.add_sector("Air_Segment", false);
        graph.add_sector("Cache_Fly", false);
        graph.add_sector("Syntax_Sector", false);
        graph.add_sector("Alpha_Tree", false);
        graph.add_sector("Restricted_Zone", false);
        graph.add_sector("Holding_Pattern_Delta", false);
        graph.add_sector("RWY12", true);
        graph.add_sector("RWY04", true);
        graph.add_sector("RWY17", true);
        graph.add_sector("RWY09", true);

        graph.add_corridor("Control_Tower", "Air_Segment", 4);
        graph.add_corridor("Control_Tower", "Cache_Fly", 3);
        graph.add_corridor("Control_Tower", "Syntax_Sector", 2);
        graph.add_corridor("Control_Tower", "Alpha_Tree", 5);
        graph.add_corridor("Air_Segment", "RWY12", 3);
        graph.add_corridor("Air_Segment", "Holding_Pattern_Delta", 2);
        graph.add_corridor("Cache_Fly", "RWY04", 4);
        graph.add_corridor("Cache_Fly", "Holding_Pattern_Delta", 3);
        graph.add_corridor("Syntax_Sector", "RWY17", 6);
        graph.add_corridor("Syntax_Sector", "Alpha_Tree", 2);
        graph.add_corridor("Alpha_Tree", "Restricted_Zone", 3);
        graph.add_corridor("Alpha_Tree", "RWY09", 9);
        graph.add_corridor("Holding_Pattern_Delta", "RWY09", 4);
        graph.add_corridor("Holding_Pattern_Delta", "RWY12", 6);
    }

    // Fallback 24-option game menu (exact manual numbering)
    void print_default_game_menu() {
        cout << "======= MayDay-404 Game Menu =======" << endl;
        cout << "01. Register Aircraft" << endl;
        cout << "02. Register Crew Member" << endl;
        cout << "03. Search Flight by Call Sign" << endl;
        cout << "04. Add Normal Arrival" << endl;
        cout << "05. Add Emergency Arrival" << endl;
        cout << "06. Process Next Landing" << endl;
        cout << "07. Clear Flight for Runway" << endl;
        cout << "08. Reroute Flight" << endl;
        cout << "09. Intercept Aircraft" << endl;
        cout << "10. Escalate Priority" << endl;
        cout << "11. Counter Cyber Attack" << endl;
        cout << "12. Undo Last Command" << endl;
        cout << "13. Display Pending Arrivals" << endl;
        cout << "14. Display Crew Members" << endl;
        cout << "15. Display Flight Records" << endl;
        cout << "16. Display Airspace Network" << endl;
        cout << "17. BFS Radar Sweep" << endl;
        cout << "18. DFS Sector Traversal" << endl;
        cout << "19. Search for Missing Flight" << endl;
        cout << "20. Find Fastest Route to Runway" << endl;
        cout << "21. Generate Minimum Radar Coverage" << endl;
        cout << "22. Compare BST and AVL Heights" << endl;
        cout << "23. End Turn/Advance Tick" << endl;
        cout << "24. Exit to Main Menu" << endl;
        cout << "====================================" << endl;
    }
};

#endif
