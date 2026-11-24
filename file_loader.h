#ifndef FILE_LOADER_H
#define FILE_LOADER_H

#include "flight.h"
#include "registration.h"
#include "space_graph.h"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
using namespace std;

// Person 1 stub: loads text data files into structures
class file_loader {
public:
    file_loader() {
    }

    ~file_loader() {
    }

    // Load flights from flights.txt into registration
    bool load_flights(registration& reg, string filename) {
        ifstream input(filename.c_str());
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
        ifstream input(filename.c_str());
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

    // Load airspace sectors and corridors into space_graph
    bool load_airspace(space_graph& graph, string filename) {
        ifstream input(filename.c_str());
        if (!input.is_open()) {
            cout << "[ATC Error] Could not open " << filename
                 << " - using default airspace." << endl;
            build_default_airspace(graph);
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
        return true;
    }

    scenario_event* load_scenario(string filename, int& count) {
        count = 0;
        ifstream input(filename.c_str());
        if (!input.is_open()) {
            cout << "Could not open " << filename << endl;
            return nullptr;
        }
        string line;
        int capacity = 0;
        while (getline(input, line))
            capacity++;
        input.clear();
        input.seekg(0);
        scenario_event* events = new scenario_event[capacity];
        while (getline(input, line)) {
            istringstream row(line);
            scenario_event event;
            if (row >> event.tick >> event.event_type
                >> event.callsign >> event.sector >> event.extra) {
                events[count] = event;
                count++;
            }
        }
        input.close();
        return events;
    }

    bool load_scenario(string filename) {
        int count;
        scenario_event* events = load_scenario(filename, count);
        if (events == nullptr)
            return false;
        cout << "Scenario events loaded: " << count << endl;
        delete[] events;
        return true;
    }

    // Load max ticks and other rules; returns max_ticks (default 30)
    int load_rules(string filename) {
        ifstream input(filename.c_str());
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
        ifstream input(filename.c_str());
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
        ifstream input(filename.c_str());
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
    // Fallback airspace when airspace.txt is missing
    void build_default_airspace(space_graph& graph) {
        graph.add_sector("Control_Tower", false);
        graph.add_sector("Restricted_Zone", false);
        graph.add_sector("Syntax_Sector", false);
        graph.add_sector("Cache_Fly", false);
        graph.add_sector("Air_Segment", false);
        graph.add_sector("NullAir_Approach", false);
        graph.add_sector("RWY09", true);
        graph.add_sector("RWY12", true);
        graph.add_sector("RWY04", true);
        graph.add_sector("RWY17", true);

        graph.add_corridor("Control_Tower", "Syntax_Sector", 2);
        graph.add_corridor("Control_Tower", "Cache_Fly", 3);
        graph.add_corridor("Control_Tower", "Air_Segment", 4);
        graph.add_corridor("Syntax_Sector", "Restricted_Zone", 2);
        graph.add_corridor("Syntax_Sector", "NullAir_Approach", 3);
        graph.add_corridor("Cache_Fly", "Air_Segment", 2);
        graph.add_corridor("Air_Segment", "RWY09", 2);
        graph.add_corridor("Air_Segment", "RWY12", 3);
        graph.add_corridor("NullAir_Approach", "RWY04", 2);
        graph.add_corridor("Restricted_Zone", "RWY17", 4);
        graph.add_corridor("Cache_Fly", "RWY09", 5);
        graph.add_corridor("Syntax_Sector", "RWY12", 4);
        graph.add_corridor("Control_Tower", "Restricted_Zone", 6);
        graph.add_corridor("NullAir_Approach", "Air_Segment", 3);
    }

    // Fallback 24-option game menu
    void print_default_game_menu() {
        cout << "======= MayDay-404 Game Menu =======" << endl;
        cout << "01. Assign Runway" << endl;
        cout << "02. Reroute Flight" << endl;
        cout << "03. Process Landing" << endl;
        cout << "04. Manual Spawn Normal Flight" << endl;
        cout << "05. Manual Spawn Emergency" << endl;
        cout << "06. Declare Emergency Priority" << endl;
        cout << "07. Clear War Aircraft" << endl;
        cout << "08. Counter Cyber Attack" << endl;
        cout << "09. Intercept Hostile" << endl;
        cout << "10. Counter Hijack" << endl;
        cout << "11. Medical Priority Landing" << endl;
        cout << "12. Weather Reroute" << endl;
        cout << "13. View Pending Arrivals" << endl;
        cout << "14. View Crew" << endl;
        cout << "15. View Flight Records" << endl;
        cout << "16. View Airspace Network" << endl;
        cout << "17. Radar Sweep (BFS/DFS)" << endl;
        cout << "18. Compute Radar MST" << endl;
        cout << "19. Search Missing Flight" << endl;
        cout << "20. Undo Last Command" << endl;
        cout << "21. Redo Last Command" << endl;
        cout << "22. Compare BST vs AVL Heights" << endl;
        cout << "23. End Turn / Advance Tick" << endl;
        cout << "24. Exit to Main Menu" << endl;
        cout << "====================================" << endl;
    }
};

#endif
