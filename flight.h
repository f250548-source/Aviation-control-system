#ifndef FLIGHT_H
#define FLIGHT_H

#include <string>
using namespace std;

// Priority levels for flights and emergencies (lower number = higher urgency)
enum priority {
    WAR = 1,
    HIJACK,
    CYBER,
    MISSING,
    FUEL,
    MEDICAL,
    WEATHER,
    MECHANICAL,
    NORMAL
};

// Single flight record stored in linked lists and trees
struct flight {
    string callsign;
    string aircraft_type;
    string airline;
    string origin;
    string destination;
    int passenger_count;
    int fuel_remaining;
    string current_sector;
    string assigned_runway;
    int arrival_tick;
    int priority_level;
    string status;
    flight* next;
};

// Crew member linked-list node
struct crew {
    string employee_id;
    string name;
    string role;
    string assigned_callsign;
    crew* next;
};

// Scenario event loaded from scenario.txt
struct scenario_event {
    int tick;
    string event_type;
    string callsign;
    string sector;
    string extra;
};

// Dijkstra route result: ordered sector name list
struct route {
    string* sectors;
    int sector_count;
    int total_weight;

    route() {
        sectors = 0;
        sector_count = 0;
        total_weight = 0;
    }
};

#endif
