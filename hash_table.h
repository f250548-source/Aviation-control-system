#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "flight.h"
#include <iostream>
#include <string>
using namespace std;

// Slot states for linear probing with tombstones
enum hash_slot_state {
    EMPTY = 0,
    OCCUPIED = 1,
    DELETED = 2
};

// Open-addressing hash table (linear probing) keyed by callsign.
// Table size is the prime 101. Stores NON-OWNING flight* pointers;
// registration owns the flight records.
//
// IMPORTANT: each slot also stores its own copy of the callsign key.
// find/remove compare against that key, never against a possibly-deleted
// flight object's string. That avoids crashes after unregister.
class hash_table {
private:
    static const int TABLE_SIZE = 101;

    flight* buckets[TABLE_SIZE];
    string keys[TABLE_SIZE];
    hash_slot_state states[TABLE_SIZE];
    int entry_count;
    int collision_count;
    int longest_probe_chain;

    // Polynomial rolling hash of callsign characters, then mod 101
    int hash_key(string callsign) {
        unsigned long hash_value = 0;
        int i = 0;
        while (i < (int)callsign.length()) {
            hash_value = hash_value * 31UL + (unsigned long)(unsigned char)callsign[i];
            i = i + 1;
        }
        return (int)(hash_value % (unsigned long)TABLE_SIZE);
    }

public:
    // Mark every slot empty and reset stats
    hash_table() {
        entry_count = 0;
        collision_count = 0;
        longest_probe_chain = 0;
        int i = 0;
        while (i < TABLE_SIZE) {
            buckets[i] = 0;
            keys[i] = "";
            states[i] = EMPTY;
            i = i + 1;
        }
    }

    // No dynamic ownership; pointers belong to registration
    ~hash_table() {
        int i = 0;
        while (i < TABLE_SIZE) {
            buckets[i] = 0;
            keys[i] = "";
            states[i] = EMPTY;
            i = i + 1;
        }
        entry_count = 0;
    }

    // Insert a non-owning pointer; returns false if duplicate or table full
    bool insert(flight* f) {
        if (f == 0) {
            return false;
        }
        if (find_flight(f->callsign) != 0) {
            return false;
        }
        if (entry_count >= TABLE_SIZE) {
            cout << "[ATC Error] Hash table is full." << endl;
            return false;
        }

        int start = hash_key(f->callsign);
        int first_deleted = -1;
        int probe_steps = 0;
        int i = 0;
        while (i < TABLE_SIZE) {
            int pos = (start + i) % TABLE_SIZE;
            if (states[pos] == OCCUPIED) {
                collision_count = collision_count + 1;
                probe_steps = probe_steps + 1;
            } else if (states[pos] == DELETED) {
                if (first_deleted < 0) {
                    first_deleted = pos;
                }
                probe_steps = probe_steps + 1;
            } else {
                int place = pos;
                if (first_deleted >= 0) {
                    place = first_deleted;
                }
                buckets[place] = f;
                keys[place] = f->callsign;
                states[place] = OCCUPIED;
                entry_count = entry_count + 1;
                int chain = probe_steps + 1;
                if (chain > longest_probe_chain) {
                    longest_probe_chain = chain;
                }
                return true;
            }
            i = i + 1;
        }

        if (first_deleted >= 0) {
            buckets[first_deleted] = f;
            keys[first_deleted] = f->callsign;
            states[first_deleted] = OCCUPIED;
            entry_count = entry_count + 1;
            if (TABLE_SIZE > longest_probe_chain) {
                longest_probe_chain = TABLE_SIZE;
            }
            return true;
        }

        cout << "[ATC Error] Hash table is full." << endl;
        return false;
    }

    // Linear probe until EMPTY (stop) or matching OCCUPIED key
    flight* find_flight(string callsign) {
        int start = hash_key(callsign);
        int i = 0;
        while (i < TABLE_SIZE) {
            int pos = (start + i) % TABLE_SIZE;
            if (states[pos] == EMPTY) {
                return 0;
            }
            // Compare stored key only — never touch a deleted flight object here
            if (states[pos] == OCCUPIED && keys[pos] == callsign) {
                return buckets[pos];
            }
            i = i + 1;
        }
        return 0;
    }

    // Mark slot DELETED (tombstone) so later probes still work
    bool remove(string callsign) {
        int start = hash_key(callsign);
        int i = 0;
        while (i < TABLE_SIZE) {
            int pos = (start + i) % TABLE_SIZE;
            if (states[pos] == EMPTY) {
                return false;
            }
            if (states[pos] == OCCUPIED && keys[pos] == callsign) {
                buckets[pos] = 0;
                keys[pos] = "";
                states[pos] = DELETED;
                entry_count = entry_count - 1;
                return true;
            }
            i = i + 1;
        }
        return false;
    }

    // Print load factor, total collisions and longest probe chain
    void display_stats() {
        cout << "=== Hash Table Stats (linear probing, size 101) ===" << endl;
        cout << "Entries: " << entry_count << " / " << TABLE_SIZE << endl;
        double load_factor = 0.0;
        if (TABLE_SIZE > 0) {
            load_factor = (double)entry_count / (double)TABLE_SIZE;
        }
        cout << "Load factor: " << load_factor << endl;
        cout << "Total collisions: " << collision_count << endl;
        cout << "Longest probe chain: " << longest_probe_chain << endl;
    }
};

#endif
