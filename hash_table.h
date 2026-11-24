
#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "flight.h"
#include <iostream>
#include <string>
using namespace std;

class hash_table {
private:
    static const int TABLE_SIZE = 101;
    enum state { EMPTY, OCCUPIED, DELETED };
    flight* buckets[TABLE_SIZE];
    state states[TABLE_SIZE];
    int entry_count;
    int collisions;
    int longest_probe;

public:
    hash_table() {
        entry_count = 0;
        collisions = 0;
        longest_probe = 0;
        for (int i = 0; i < TABLE_SIZE; i++) {
            buckets[i] = nullptr;
            states[i] = EMPTY;
        }
    }

    int hash_key(string callsign) {
        int sum = 0;
        for (int i = 0; i < callsign.length(); i++) {
            sum += (unsigned char)callsign[i];
        }
        return sum % TABLE_SIZE;
    }

    bool insert(string callsign, flight* f) {
        if (f == nullptr || f->callsign != callsign)
            return false;
        if (find_flight(callsign) != nullptr)
            return false;
        int index = hash_key(callsign);
        for (int i = 0; i < TABLE_SIZE; i++) {
            int pos = (index + i) % TABLE_SIZE;
            if (states[pos] != OCCUPIED) {
                buckets[pos] = f;
                states[pos] = OCCUPIED;
                entry_count++;
                if (i + 1 > longest_probe)
                    longest_probe = i + 1;
                return true;
            }
            collisions++;
        }
        return false;
    }

    bool insert(flight* f) {
        if (f == nullptr)
            return false;
        return insert(f->callsign, f);
    }

    flight* find_flight(string callsign) {
        int index = hash_key(callsign);
        for (int i = 0; i < TABLE_SIZE; i++) {
            int pos = (index + i) % TABLE_SIZE;
            if (states[pos] == EMPTY)
                return nullptr;
            if (states[pos] == OCCUPIED &&
                buckets[pos]->callsign == callsign)
                return buckets[pos];
        }
        return nullptr;
    }

    bool remove(string callsign) {
        int index = hash_key(callsign);
        for (int i = 0; i < TABLE_SIZE; i++) {
            int pos = (index + i) % TABLE_SIZE;
            if (states[pos] == EMPTY)
                return false;
            if (states[pos] == OCCUPIED &&
                buckets[pos]->callsign == callsign) {
                buckets[pos] = nullptr;
                states[pos] = DELETED;
                entry_count--;
                return true;
            }
        }
        return false;
    }

    void display_stats() {
        cout << "=== Hash Table Stats ===" << endl;
        cout << "Entries: " << entry_count << "/" << TABLE_SIZE << endl;
        cout << "Load Factor: " << (double)entry_count / TABLE_SIZE << endl;
        cout << "Collisions: " << collisions << endl;
        cout << "Longest Probe Chain: " << longest_probe << endl;
    }
};

#endif
