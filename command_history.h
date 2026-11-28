
#ifndef COMMAND_HISTORY_H
#define COMMAND_HISTORY_H

#include "flight.h"
#include <iostream>
#include <string>
using namespace std;

struct game_snapshot {
    int tick;
    int credibility;
    int flights_landed;
    int emergencies_resolved;
    string last_action;
    string callsign;
    string prev_runway;
    int prev_priority;
    string prev_sector;
    string prev_status;
    game_snapshot* next;
};

struct command_node {
    string command_text;
    int tick;
    command_node* next;
    command_node* prev;
};

class command_history {
private:
    command_node* history_head;
    game_snapshot* undo_top;
    game_snapshot* redo_top;
    string last_command;

    void clear_stack(game_snapshot*& top) {
        while (top != nullptr) {
            game_snapshot* temp = top;
            top = top->next;
            delete temp;
        }
    }

public:
    command_history() {
        history_head = nullptr;
        undo_top = nullptr;
        redo_top = nullptr;
        last_command = "";
    }

    ~command_history() {
        while (history_head != nullptr) {
            command_node* temp = history_head;
            history_head = history_head->next;
            delete temp;
        }
        clear_stack(undo_top);
        clear_stack(redo_top);
    }

    void log_command(string command_text, int tick) {
        command_node* node = new command_node;
        node->command_text = command_text;
        node->tick = tick;
        node->prev = nullptr;
        node->next = history_head;
        if (history_head != nullptr)
            history_head->prev = node;
        history_head = node;
        last_command = command_text;
    }

    void push_snapshot(int tick, int credibility,
        int flights_landed,
        int emergencies_resolved,
        string last_action,
        flight* f = nullptr) {
        game_snapshot* node = new game_snapshot;
        node->tick = tick;
        node->credibility = credibility;
        node->flights_landed = flights_landed;
        node->emergencies_resolved = emergencies_resolved;
        node->last_action = last_action;
        node->callsign = "";
        node->prev_priority = NORMAL;
        if (f != nullptr) {
            node->callsign = f->callsign;
            node->prev_runway = f->assigned_runway;
            node->prev_priority = f->priority_level;
            node->prev_sector = f->current_sector;
            node->prev_status = f->status;
        }
        node->next = undo_top;
        undo_top = node;
        clear_stack(redo_top);
    }

    game_snapshot* undo() {
        if (undo_top == nullptr) {
            cout << "[ATC Error] Nothing to undo (undo stack empty)." << endl;
            return nullptr;
        }
        game_snapshot* node = undo_top;
        undo_top = undo_top->next;
        node->next = redo_top;
        redo_top = node;
        return node;
    }

    game_snapshot* redo() {
        if (redo_top == nullptr) {
            cout << "[ATC Error] Nothing to redo (redo stack empty)." << endl;
            return nullptr;
        }
        game_snapshot* node = redo_top;
        redo_top = redo_top->next;
        node->next = undo_top;
        undo_top = node;
        return node;
    }

    void display_history(int count = -1) {
        cout << "=== Command History ===" << endl;
        command_node* current = history_head;
        if (current == nullptr) {
            cout << "No commands logged." << endl;
            return;
        }
        while (current != nullptr && count != 0) {
            cout << "[Tick " << current->tick << "] "
                << current->command_text << endl;
            current = current->next;
            if (count > 0)
                count--;
        }
    }

    void display_last_command() {
        if (last_command.empty())
            cout << "No commands yet." << endl;
        else
            cout << "Last command: " << last_command << endl;
    }
};

#endif
