#include "game_engine.h"
#include "menu.h"
#include "utils.h"
#include <iostream>
#include <string>
using namespace std;

// Entry point: main menu loop for MayDay-404
int main() {
    cout << "========================================" << endl;
    cout << "   MayDay-404  Air Traffic Control" << endl;
    cout << "   Data Structures Simulation Project" << endl;
    cout << "========================================" << endl;

    bool running = true;
    while (running) {
        int choice = main_menu();
        if (choice == 1) {
            // Fresh engine instance per new game
            game_engine engine;
            engine.start_new_game();
        } else if (choice == 2) {
            show_leaderboard();
        } else if (choice == 3) {
            show_about();
        } else if (choice == 4) {
            show_rules();
        } else if (choice == 5) {
            cout << "Exiting MayDay-404. Roger, Wilco, Out." << endl;
            running = false;
        }
    }
    return 0;
}
