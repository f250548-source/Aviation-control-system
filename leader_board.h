#ifndef LEADER_BOARD_H
#define LEADER_BOARD_H

#include <fstream>
#include <iostream>
#include <string>
using namespace std;

// Person 1 stub: append and display leaderboard results
class leader_board {
public:
    leader_board() {
    }

    ~leader_board() {
    }

    // Append a result line to leaderboard.txt
    void save_result(string player_name, int credibility, int ticks,
                     int flights_landed, int emergencies_resolved,
                     string outcome, string filename) {
        ofstream output(filename.c_str(), ios::app);
        if (!output.is_open()) {
            cout << "[ATC Error] Could not open " << filename
                 << " for writing." << endl;
            return;
        }
        output << player_name << " "
               << credibility << " "
               << ticks << " "
               << flights_landed << " "
               << emergencies_resolved << " "
               << outcome << endl;
        output.close();
        cout << "Result saved to " << filename << endl;
    }

    // Display all leaderboard entries
    void display_leaderboard(string filename) {
        ifstream input(filename.c_str());
        if (!input.is_open()) {
            cout << "[ATC Error] Could not open " << filename << endl;
            cout << "No leaderboard entries yet." << endl;
            return;
        }
        cout << "=== Leader Board ===" << endl;
        cout << "Name Cred Ticks Landed Emerg Outcome" << endl;
        string name;
        int credibility;
        int ticks;
        int flights_landed;
        int emergencies_resolved;
        string outcome;
        bool any = false;
        while (input >> name >> credibility >> ticks >> flights_landed
                     >> emergencies_resolved >> outcome) {
            any = true;
            cout << name << " " << credibility << " " << ticks << " "
                 << flights_landed << " " << emergencies_resolved << " "
                 << outcome << endl;
        }
        if (!any) {
            cout << "No leaderboard entries yet." << endl;
        }
        input.close();
    }
};

#endif
