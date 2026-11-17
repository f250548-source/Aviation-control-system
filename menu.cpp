#include "menu.h"
#include "file_loader.h"
#include "leader_board.h"
#include "utils.h"
#include <fstream>
#include <string>
using namespace std;

// Print the top-level menu and validate the player's choice
int main_menu() {
    cout << endl;
    cout << "====================================" << endl;
    cout << "        MayDay-404  MAIN MENU       " << endl;
    cout << "====================================" << endl;
    cout << "[1] New Game" << endl;
    cout << "[2] View Leader Board" << endl;
    cout << "[3] About" << endl;
    cout << "[4] Rules" << endl;
    cout << "[5] Exit" << endl;
    cout << "====================================" << endl;
    return read_int("Select option: ", 1, 5);
}

// Load and print gamemenu.txt through file_loader
void display_game_menu() {
    file_loader loader;
    loader.load_game_menu("gamemenu.txt");
}

// Print about.txt
void show_about() {
    file_loader loader;
    loader.load_about("about.txt");
}

// Print rules.txt contents and report MAX_TICKS
void show_rules() {
    file_loader loader;
    int max_ticks = loader.load_rules("rules.txt");
    cout << endl;
    cout << "=== Rules Summary ===" << endl;
    cout << "Max ticks: " << max_ticks << endl;
    cout << "Each tick = 30 simulated seconds." << endl;
    cout << "Credibility starts at 100 (max 200). Game lost at 0." << endl;
    cout << "2 actions per turn. Wrong commands cost credibility." << endl;
    cout << endl;

    // Also dump the raw file if present
    ifstream input("rules.txt");
    if (input.is_open()) {
        cout << "--- rules.txt ---" << endl;
        string line;
        while (getline(input, line)) {
            cout << line << endl;
        }
        input.close();
    }
}

// Display leaderboard.txt
void show_leaderboard() {
    leader_board board;
    board.display_leaderboard("leaderboard.txt");
}
