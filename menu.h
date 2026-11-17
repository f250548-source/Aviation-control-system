#ifndef MENU_H
#define MENU_H

#include <iostream>
#include <string>
using namespace std;

// Main menu and in-game menu display helpers

// Show [1] New Game ... [5] Exit and return the chosen option
int main_menu();

// Print the 24-option game menu from gamemenu.txt (or built-in fallback)
void display_game_menu();

// Show About text via file_loader
void show_about();

// Show Rules text / max ticks info
void show_rules();

// Show leaderboard via leader_board
void show_leaderboard();

#endif
