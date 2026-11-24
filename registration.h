#ifndef REGISTRATION_H
#define REGISTRATION_H

#include "flight.h"
#include <iostream>
#include <string>
using namespace std;

// Person 1 stub: linked-list registry of flights and crew
class registration {
private:
    flight* flight_head;
    crew* crew_head;
    int flight_count;
    int crew_count;

public:
    registration() {
        flight_head = 0;
        crew_head = 0;
        flight_count = 0;
        crew_count = 0;
    }

    // Free all flight and crew nodes
    ~registration() {
        flight* f = flight_head;
        while (f != 0) {
            flight* temp = f;
            f = f->next;
            delete temp;
        }
        crew* c = crew_head;
        while (c != 0) {
            crew* temp = c;
            c = c->next;
            delete temp;
        }
        flight_head = 0;
        crew_head = 0;
    }

    // Add a flight to the front of the list
    void add_flight(flight* new_flight) {
        if (new_flight == nullptr)
            return;
        new_flight->next = nullptr;
        if (flight_head == nullptr) {
            flight_head = new_flight;
        }
        else {
            flight* current = flight_head;
            while (current->next != nullptr) {
                current = current->next;
            }
            current->next = new_flight;
        }
        flight_count++;
    }

    // Remove a flight by callsign; returns true if found
    bool remove_flight(string callsign) {
        flight* current = flight_head;
        flight* previous = 0;
        while (current != 0) {
            if (current->callsign == callsign) {
                if (previous == 0) {
                    flight_head = current->next;
                } else {
                    previous->next = current->next;
                }
                delete current;
                flight_count = flight_count - 1;
                return true;
            }
            previous = current;
            current = current->next;
        }
        return false;
    }

    // Stub: advance status fields for all flights
    void update_all_flights() {
        flight* current = flight_head;
        while (current != nullptr) {
            if (current->status == "ACTIVE" && current->fuel_remaining > 0) {
                current->fuel_remaining--;
            }
            current = current->next;
        }
    }

    // Print every registered flight
    void display_all_flights() {
        cout << "=== Flight Registry ===" << endl;
        flight* current = flight_head;
        if (current == 0) {
            cout << "No flights registered." << endl;
            return;
        }
        while (current != 0) {
            cout << current->callsign << " | " << current->aircraft_type
                 << " | " << current->status << " | sector=" << current->current_sector
                 << " | fuel=" << current->fuel_remaining
                 << " | priority=" << current->priority_level << endl;
            current = current->next;
        }
    }

    // Add a crew member to the front of the list
    void add_crew(crew* new_crew) {
        if (new_crew == nullptr)
            return;
        new_crew->next = nullptr;
        if (crew_head == nullptr) {
            crew_head = new_crew;
        }
        else {
            crew* current = crew_head;
            while (current->next != nullptr) {
                current = current->next;
            }
            current->next = new_crew;
        }
        crew_count++;
    }

    // Print every crew member
    void display_all_crew() {
        cout << "=== Crew Roster ===" << endl;
        crew* current = crew_head;
        if (current == 0) {
            cout << "No crew registered." << endl;
            return;
        }
        while (current != 0) {
            cout << current->employee_id << " | " << current->name
                 << " | " << current->role
                 << " | assigned=" << current->assigned_callsign << endl;
            current = current->next;
        }
    }

    // Return head of flight list
    flight* get_head() {
        return flight_head;
    }

    // Return head of crew list
    crew* get_crew_head() {
        return crew_head;
    }

    // Return number of flights
    int get_flight_count() {
        return flight_count;
    }
};

#endif
