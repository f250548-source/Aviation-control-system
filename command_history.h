#ifndef COMMAND_HISTORY_H
#define COMMAND_HISTORY_H

#include <iostream>
#include <string>
using namespace std;

// Snapshot of game-relevant counters for undo/redo
struct game_snapshot {
    int tick;
    int credibility;
    int flights_landed;
    int emergencies_resolved;
    string last_action;
    game_snapshot* next;
};

// Command log node
struct command_node {
    string command_text;
    int tick;
    command_node* next;
};

// Person 1 stub: command log + undo/redo stacks
class command_history {
private:
    command_node* history_head;
    game_snapshot* undo_top;
    game_snapshot* redo_top;
    string last_command;

public:
    command_history() {
        history_head = 0;
        undo_top = 0;
        redo_top = 0;
        last_command = "";
    }

    // Free history and both stacks
    ~command_history() {
        command_node* c = history_head;
        while (c != 0) {
            command_node* temp = c;
            c = c->next;
            delete temp;
        }
        game_snapshot* u = undo_top;
        while (u != 0) {
            game_snapshot* temp = u;
            u = u->next;
            delete temp;
        }
        game_snapshot* r = redo_top;
        while (r != 0) {
            game_snapshot* temp = r;
            r = r->next;
            delete temp;
        }
        history_head = 0;
        undo_top = 0;
        redo_top = 0;
    }

    // Append a command string to the history list
    void log_command(string command_text, int tick) {
        command_node* node = new command_node;
        node->command_text = command_text;
        node->tick = tick;
        node->next = history_head;
        history_head = node;
        last_command = command_text;
    }

    // Push a snapshot onto the undo stack; clear redo
    void push_snapshot(int tick, int credibility, int flights_landed,
                       int emergencies_resolved, string last_action) {
        game_snapshot* node = new game_snapshot;
        node->tick = tick;
        node->credibility = credibility;
        node->flights_landed = flights_landed;
        node->emergencies_resolved = emergencies_resolved;
        node->last_action = last_action;
        node->next = undo_top;
        undo_top = node;

        // Clear redo stack
        game_snapshot* r = redo_top;
        while (r != 0) {
            game_snapshot* temp = r;
            r = r->next;
            delete temp;
        }
        redo_top = 0;
    }

    // Pop undo into redo; returns null if empty
    game_snapshot* undo() {
        if (undo_top == 0) {
            cout << "[ATC Error] Nothing to undo." << endl;
            return 0;
        }
        game_snapshot* node = undo_top;
        undo_top = undo_top->next;
        node->next = redo_top;
        redo_top = node;
        cout << "Undo: restored snapshot at tick " << node->tick << endl;
        return node;
    }

    // Pop redo into undo; returns null if empty
    game_snapshot* redo() {
        if (redo_top == 0) {
            cout << "[ATC Error] Nothing to redo." << endl;
            return 0;
        }
        game_snapshot* node = redo_top;
        redo_top = redo_top->next;
        node->next = undo_top;
        undo_top = node;
        cout << "Redo: restored snapshot at tick " << node->tick << endl;
        return node;
    }

    // Print full command history
    void display_history() {
        cout << "=== Command History ===" << endl;
        command_node* current = history_head;
        if (current == 0) {
            cout << "No commands logged." << endl;
            return;
        }
        while (current != 0) {
            cout << "[Tick " << current->tick << "] " << current->command_text << endl;
            current = current->next;
        }
    }

    // Print the most recent command
    void display_last_command() {
        if (last_command.length() == 0) {
            cout << "No commands yet." << endl;
        } else {
            cout << "Last command: " << last_command << endl;
        }
    }
};

#endif
