#include "utils.h"
using namespace std;

// Exchange two integer values without std::swap
void swap_values(int& a, int& b) {
    int temp = a;
    a = b;
    b = temp;
}

// Manual minimum
int min_value(int a, int b) {
    if (a < b) {
        return a;
    }
    return b;
}

// Manual maximum
int max_value(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

// Discard leftover characters after a failed or partial read
void clear_input_buffer() {
    cin.clear();
    char ch = ' ';
    while (cin.get(ch)) {
        if (ch == '\n') {
            break;
        }
    }
}

// Keep prompting until the user enters an int inside the closed range
int read_int(string prompt, int min_value_bound, int max_value_bound) {
    while (true) {
        cout << prompt;
        int value = 0;
        cin >> value;
        if (cin.fail()) {
            cout << "[ATC Error] Please enter a valid integer." << endl;
            clear_input_buffer();
            continue;
        }
        // Consume trailing newline / junk on the same line
        clear_input_buffer();
        if (value < min_value_bound || value > max_value_bound) {
            cout << "[ATC Error] Value must be between "
                 << min_value_bound << " and " << max_value_bound << "." << endl;
            continue;
        }
        return value;
    }
}

// Player name must be non-empty and contain no spaces
string read_name() {
    while (true) {
        cout << "Enter player name (no spaces; use _ or . instead): ";
        string name;
        cin >> name;
        if (cin.fail()) {
            cout << "[ATC Error] Could not read name." << endl;
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        if (name.length() == 0) {
            cout << "[ATC Error] Name cannot be empty." << endl;
            continue;
        }

        // Reject embedded spaces (should not happen with >>, but check anyway)
        bool has_space = false;
        int i = 0;
        while (i < (int)name.length()) {
            if (name[i] == ' ') {
                has_space = true;
                break;
            }
            i = i + 1;
        }
        if (has_space) {
            cout << "[ATC Error] Name cannot contain spaces. Use underscore or dot."
                 << endl;
            continue;
        }
        return name;
    }
}

// Draw a simple modal-style result box in the console
void print_result_box(string player_name, int credibility, int ticks,
                      int flights_landed, int emergencies_resolved,
                      string outcome, string failure_cause) {
    cout << endl;
    cout << "+------------------------------------------+" << endl;
    cout << "|           MayDay-404 MISSION END         |" << endl;
    cout << "+------------------------------------------+" << endl;
    cout << "| Player : " << player_name << endl;
    cout << "| Outcome: " << outcome << endl;
    cout << "| Credibility : " << credibility << endl;
    cout << "| Ticks       : " << ticks << endl;
    cout << "| Flights landed      : " << flights_landed << endl;
    cout << "| Emergencies resolved: " << emergencies_resolved << endl;
    if (failure_cause.length() > 0) {
        cout << "| Failure cause: " << failure_cause << endl;
    }
    cout << "+------------------------------------------+" << endl;
    cout << endl;
}

// Echo a radio phrase for immersion / viva demos
void print_aviation_call(string call_text) {
    cout << "*** " << call_text << " ***" << endl;
}
