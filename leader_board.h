#ifndef LEADER_BOARD_H
#define LEADER_BOARD_H

#include "utils.h"
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

// One leaderboard row matching:
// # | CRO | Result | Cred | Ticks | Crash | Hijack | Breach | Date
struct leaderboard_entry {
    string commander;
    string result;
    int credibility;
    int ticks;
    int crashes;
    int hijacks;
    int breaches;
    string date;
};

// Append / display ranked results in leaderboard.txt
class leader_board {
private:
    // Manual token split: fill tokens[] from a line (max max_tokens)
    int split_line_tokens(string line, string* tokens, int max_tokens) {
        int count = 0;
        string current = "";
        int i = 0;
        while (i < (int)line.length() && count < max_tokens) {
            char ch = line[i];
            if (ch == ' ' || ch == '\t' || ch == '|') {
                if (current.length() > 0) {
                    tokens[count] = current;
                    count = count + 1;
                    current = "";
                }
            } else {
                current = current + ch;
            }
            i = i + 1;
        }
        if (current.length() > 0 && count < max_tokens) {
            tokens[count] = current;
            count = count + 1;
        }
        return count;
    }

    // Convert digit string to int; returns false on bad input
    bool parse_int_token(string text, int& out_value) {
        if (text.length() == 0) {
            return false;
        }
        int value = 0;
        int i = 0;
        if (text[0] == '-') {
            i = 1;
        }
        if (i >= (int)text.length()) {
            return false;
        }
        while (i < (int)text.length()) {
            if (text[i] < '0' || text[i] > '9') {
                return false;
            }
            value = value * 10 + (text[i] - '0');
            i = i + 1;
        }
        if (text[0] == '-') {
            value = -value;
        }
        out_value = value;
        return true;
    }

    // Ranking: WIN above LOSE, then higher cred, then more ticks, then newer date
    bool is_better(leaderboard_entry a, leaderboard_entry b) {
        if (a.result != b.result) {
            return a.result == "WIN";
        }
        if (a.credibility != b.credibility) {
            return a.credibility > b.credibility;
        }
        if (a.ticks != b.ticks) {
            return a.ticks > b.ticks;
        }
        return a.date > b.date;
    }

    // Hand-written bubble sort (no <algorithm>)
    void sort_entries(leaderboard_entry* entries, int count) {
        int i = 0;
        while (i < count - 1) {
            int j = 0;
            while (j < count - i - 1) {
                if (is_better(entries[j + 1], entries[j])) {
                    leaderboard_entry temp = entries[j];
                    entries[j] = entries[j + 1];
                    entries[j + 1] = temp;
                }
                j = j + 1;
            }
            i = i + 1;
        }
    }

    // Today's date as YYYY-MM-DD
    string current_date_string() {
        time_t now = time(0);
        tm local_time;
        localtime_s(&local_time, &now);
        char buffer[32];
        strftime(buffer, 32, "%Y-%m-%d", &local_time);
        return string(buffer);
    }

public:
    leader_board() {
    }

    ~leader_board() {
    }

    // Append one game result with the current date
    void save_result(string commander, string result, int credibility, int ticks,
                     int crashes, int hijacks, int breaches, string filename) {
        string path = resolve_data_file(filename);
        // If file does not exist yet, prefer writing next to other data files
        ifstream probe(path.c_str());
        if (!probe.is_open()) {
            path = string("../") + filename;
        } else {
            probe.close();
        }
        ofstream output(path.c_str(), ios::app);
        if (!output.is_open()) {
            ofstream fallback(filename.c_str(), ios::app);
            if (!fallback.is_open()) {
                cout << "[ATC Error] Could not open " << filename
                     << " for writing." << endl;
                return;
            }
            string date_text = current_date_string();
            fallback << commander << " | " << result << " | " << credibility
                     << " | " << ticks << " | " << crashes << " | " << hijacks
                     << " | " << breaches << " | " << date_text << endl;
            fallback.close();
            cout << "Result saved to " << filename << endl;
            return;
        }
        string date_text = current_date_string();
        output << commander << " | " << result << " | " << credibility
               << " | " << ticks << " | " << crashes << " | " << hijacks
               << " | " << breaches << " | " << date_text << endl;
        output.close();
        cout << "Result saved to " << path << endl;
    }

    // Load all rows, sort, print with rank numbers
    void display_leaderboard(string filename) {
        string path = resolve_data_file(filename);
        ifstream input(path.c_str());
        if (!input.is_open()) {
            cout << "[ATC Error] Could not open " << filename << endl;
            cout << "No leaderboard entries yet." << endl;
            return;
        }

        // First pass: count lines
        string line;
        int capacity = 0;
        while (getline(input, line)) {
            if (line.length() > 0) {
                capacity = capacity + 1;
            }
        }
        if (capacity == 0) {
            cout << "No leaderboard entries yet." << endl;
            input.close();
            return;
        }

        input.clear();
        input.seekg(0);

        leaderboard_entry* entries = new leaderboard_entry[capacity];
        int count = 0;
        while (getline(input, line)) {
            if (line.length() == 0) {
                continue;
            }
            string tokens[16];
            int token_count = split_line_tokens(line, tokens, 16);
            // Expected: CRO Result Cred Ticks Crash Hijack Breach Date
            if (token_count < 8) {
                continue;
            }
            leaderboard_entry e;
            e.commander = tokens[0];
            e.result = tokens[1];
            if (!parse_int_token(tokens[2], e.credibility)) {
                continue;
            }
            if (!parse_int_token(tokens[3], e.ticks)) {
                continue;
            }
            if (!parse_int_token(tokens[4], e.crashes)) {
                continue;
            }
            if (!parse_int_token(tokens[5], e.hijacks)) {
                continue;
            }
            if (!parse_int_token(tokens[6], e.breaches)) {
                continue;
            }
            e.date = tokens[7];
            entries[count] = e;
            count = count + 1;
        }
        input.close();

        sort_entries(entries, count);

        cout << "=== Leaderboard ===" << endl;
        cout << "# | CRO | Result | Cred | Ticks | Crash | Hijack | Breach | Date"
             << endl;
        if (count == 0) {
            cout << "No leaderboard entries yet." << endl;
        }
        int i = 0;
        while (i < count) {
            cout << (i + 1) << " | " << entries[i].commander
                 << " | " << entries[i].result
                 << " | " << entries[i].credibility
                 << " | " << entries[i].ticks
                 << " | " << entries[i].crashes
                 << " | " << entries[i].hijacks
                 << " | " << entries[i].breaches
                 << " | " << entries[i].date << endl;
            i = i + 1;
        }
        delete[] entries;
    }
};

#endif
