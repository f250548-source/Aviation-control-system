
#ifndef LEADER_BOARD_H
#define LEADER_BOARD_H

#include <fstream>
#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
using namespace std;

struct leaderboard_entry {
    string name;
    int credibility;
    int ticks;
    int flights_landed;
    int emergencies_resolved;
    string outcome;
    string date;
};

class leader_board {
private:
    bool is_better(leaderboard_entry a, leaderboard_entry b) {
        if (a.outcome != b.outcome)
            return a.outcome == "WIN";
        if (a.credibility != b.credibility)
            return a.credibility > b.credibility;
        if (a.ticks != b.ticks)
            return a.ticks < b.ticks;
        return a.date < b.date;
    }

public:
    void save_result(string player_name, int credibility,
        int ticks, int flights_landed,
        int emergencies_resolved,
        string outcome, string filename) {
        ofstream output(filename.c_str(), ios::app);
        if (!output.is_open()) {
            cout << "Could not open leaderboard." << endl;
            return;
        }
        time_t now = time(0);
        char date[20];
        tm time_info;
        localtime_s(&time_info, &now);
        strftime(date, sizeof(date), "%Y-%m-%d", &time_info);
        output << player_name << " "
            << credibility << " "
            << ticks << " "
            << flights_landed << " "
            << emergencies_resolved << " "
            << outcome << " "
            << date << endl;
        output.close();
        cout << "Result saved." << endl;
    }

    void display_leaderboard(string filename) {
        ifstream input(filename.c_str());
        if (!input.is_open()) {
            cout << "No leaderboard entries yet." << endl;
            return;
        }
        string line;
        int capacity = 0;
        while (getline(input, line))
            capacity++;
        input.clear();
        input.seekg(0);
        leaderboard_entry* entries = new leaderboard_entry[capacity];
        int count = 0;
        while (getline(input, line)) {
            istringstream row(line);
            leaderboard_entry e;
            if (row >> e.name >> e.credibility
                >> e.ticks >> e.flights_landed
                >> e.emergencies_resolved >> e.outcome) {
                if (!(row >> e.date))
                    e.date = "Unknown";
                entries[count] = e;
                count++;
            }
        }
        for (int i = 0; i < count - 1; i++) {
            for (int j = 0; j < count - i - 1; j++) {
                if (is_better(entries[j + 1], entries[j])) {
                    leaderboard_entry temp = entries[j];
                    entries[j] = entries[j + 1];
                    entries[j + 1] = temp;
                }
            }
        }
        cout << "=== Leaderboard ===" << endl;
        if (count == 0)
            cout << "No leaderboard entries yet." << endl;
        for (int i = 0; i < count; i++) {
            cout << i + 1 << ". "
                << entries[i].name << " | "
                << entries[i].credibility << " | "
                << entries[i].ticks << " | "
                << entries[i].flights_landed << " | "
                << entries[i].emergencies_resolved << " | "
                << entries[i].outcome << " | "
                << entries[i].date << endl;
        }
        delete[] entries;
        input.close();
    }
};

#endif
