#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "flight.h"
#include <iostream>
#include <string>
using namespace std;

// Person 1 stub: open-addressing / chaining hash table over callsigns
class hash_table {
private:
    static const int TABLE_SIZE = 53;
    flight** buckets;
    int entry_count;

    // Simple hash of callsign characters
    int hash_key(string callsign) {
        int sum = 0;
        int i = 0;
        while (i < (int)callsign.length()) {
            sum = sum + (int)callsign[i];
            i = i + 1;
        }
        if (sum < 0) {
            sum = -sum;
        }
        return sum % TABLE_SIZE;
    }

public:
    hash_table() {
        buckets = new flight*[TABLE_SIZE];
        int i = 0;
        while (i < TABLE_SIZE) {
            buckets[i] = 0;
            i = i + 1;
        }
        entry_count = 0;
    }

    // Free every chained flight node owned by the table
    ~hash_table() {
        int i = 0;
        while (i < TABLE_SIZE) {
            flight* current = buckets[i];
            while (current != 0) {
                flight* temp = current;
                current = current->next;
                delete temp;
            }
            buckets[i] = 0;
            i = i + 1;
        }
        delete[] buckets;
        buckets = 0;
    }

    // Insert a COPY of the flight keyed by callsign
    void insert(flight* f) {
        if (f == 0) {
            return;
        }
        int index = hash_key(f->callsign);
        flight* node = new flight;
        *node = *f;
        node->next = buckets[index];
        buckets[index] = node;
        entry_count = entry_count + 1;
    }

    // Find a flight by callsign; returns pointer or null
    flight* find_flight(string callsign) {
        int index = hash_key(callsign);
        flight* current = buckets[index];
        while (current != 0) {
            if (current->callsign == callsign) {
                return current;
            }
            current = current->next;
        }
        return 0;
    }

    // Remove a flight by callsign
    bool remove(string callsign) {
        int index = hash_key(callsign);
        flight* current = buckets[index];
        flight* previous = 0;
        while (current != 0) {
            if (current->callsign == callsign) {
                if (previous == 0) {
                    buckets[index] = current->next;
                } else {
                    previous->next = current->next;
                }
                delete current;
                entry_count = entry_count - 1;
                return true;
            }
            previous = current;
            current = current->next;
        }
        return false;
    }

    // Print load stats
    void display_stats() {
        cout << "=== Hash Table Stats ===" << endl;
        cout << "Entries: " << entry_count << " / " << TABLE_SIZE << endl;
    }
};

#endif
