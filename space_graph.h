#ifndef SPACE_GRAPH_H
#define SPACE_GRAPH_H

#include <iostream>
#include <string>
using namespace std;

// Adjacency-list neighbor node for an undirected weighted corridor
struct neighbor_node {
    int sector_index;
    int weight;
    neighbor_node* next;
};

// Sector vertex
struct sector_node {
    string name;
    bool is_runway;
    neighbor_node* neighbors;
};

// Person 2 stub: undirected weighted airspace graph
class space_graph {
private:
    static const int MAX_SECTORS = 64;
    sector_node* sectors;
    int sector_count;

public:
    space_graph() {
        sectors = new sector_node[MAX_SECTORS];
        sector_count = 0;
        int i = 0;
        while (i < MAX_SECTORS) {
            sectors[i].name = "";
            sectors[i].is_runway = false;
            sectors[i].neighbors = 0;
            i = i + 1;
        }
    }

    // Free all neighbor lists and sector array
    ~space_graph() {
        int i = 0;
        while (i < sector_count) {
            neighbor_node* current = sectors[i].neighbors;
            while (current != 0) {
                neighbor_node* temp = current;
                current = current->next;
                delete temp;
            }
            sectors[i].neighbors = 0;
            i = i + 1;
        }
        delete[] sectors;
        sectors = 0;
        sector_count = 0;
    }

    // Add a named sector; returns index or -1 on failure
    int add_sector(string name, bool is_runway_flag) {
        if (sector_count >= MAX_SECTORS) {
            cout << "[ATC Error] Sector capacity reached." << endl;
            return -1;
        }
        if (get_sector_index(name) != -1) {
            return get_sector_index(name);
        }
        sectors[sector_count].name = name;
        sectors[sector_count].is_runway = is_runway_flag;
        sectors[sector_count].neighbors = 0;
        int index = sector_count;
        sector_count = sector_count + 1;
        return index;
    }

    // Add undirected corridor between two named sectors
    bool add_corridor(string from_name, string to_name, int weight) {
        int from_index = get_sector_index(from_name);
        int to_index = get_sector_index(to_name);
        if (from_index < 0 || to_index < 0) {
            cout << "[ATC Error] Corridor endpoints not found." << endl;
            return false;
        }
        if (weight <= 0) {
            cout << "[ATC Error] Corridor weight must be positive." << endl;
            return false;
        }
        // Avoid duplicate edges
        if (get_weight(from_index, to_index) > 0) {
            return true;
        }

        neighbor_node* a = new neighbor_node;
        a->sector_index = to_index;
        a->weight = weight;
        a->next = sectors[from_index].neighbors;
        sectors[from_index].neighbors = a;

        neighbor_node* b = new neighbor_node;
        b->sector_index = from_index;
        b->weight = weight;
        b->next = sectors[to_index].neighbors;
        sectors[to_index].neighbors = b;
        return true;
    }

    // Remove undirected corridor between named sectors
    bool remove_corridor(string from_name, string to_name) {
        int from_index = get_sector_index(from_name);
        int to_index = get_sector_index(to_name);
        if (from_index < 0 || to_index < 0) {
            cout << "[ATC Error] Corridor endpoints not found." << endl;
            return false;
        }
        bool removed_a = remove_neighbor(from_index, to_index);
        bool removed_b = remove_neighbor(to_index, from_index);
        return removed_a || removed_b;
    }

    // Weight between two indices; 0 if no edge
    int get_weight(int from_index, int to_index) {
        if (from_index < 0 || to_index < 0 ||
            from_index >= sector_count || to_index >= sector_count) {
            return 0;
        }
        neighbor_node* current = sectors[from_index].neighbors;
        while (current != 0) {
            if (current->sector_index == to_index) {
                return current->weight;
            }
            current = current->next;
        }
        return 0;
    }

    // Look up sector index by name; -1 if missing
    int get_sector_index(string name) {
        int i = 0;
        while (i < sector_count) {
            if (sectors[i].name == name) {
                return i;
            }
            i = i + 1;
        }
        return -1;
    }

    // Sector name by index
    string get_sector_name(int index) {
        if (index < 0 || index >= sector_count) {
            return "";
        }
        return sectors[index].name;
    }

    // Number of sectors
    int get_sector_count() {
        return sector_count;
    }

    // True if sector is a runway
    bool is_runway(int index) {
        if (index < 0 || index >= sector_count) {
            return false;
        }
        return sectors[index].is_runway;
    }

    // Head of neighbor list for a sector
    neighbor_node* get_neighbors(int index) {
        if (index < 0 || index >= sector_count) {
            return 0;
        }
        return sectors[index].neighbors;
    }

    // Print adjacency list
    void display_adjacency_list() {
        cout << "=== Airspace Adjacency List ===" << endl;
        int i = 0;
        while (i < sector_count) {
            cout << sectors[i].name;
            if (sectors[i].is_runway) {
                cout << " [RWY]";
            }
            cout << " -> ";
            neighbor_node* current = sectors[i].neighbors;
            if (current == 0) {
                cout << "(none)";
            }
            while (current != 0) {
                cout << sectors[current->sector_index].name
                     << "(" << current->weight << ") ";
                current = current->next;
            }
            cout << endl;
            i = i + 1;
        }
    }

    // Print adjacency matrix
    void display_adjacency_matrix() {
        cout << "=== Airspace Adjacency Matrix ===" << endl;
        int i = 0;
        while (i < sector_count) {
            int j = 0;
            while (j < sector_count) {
                cout << get_weight(i, j);
                if (j + 1 < sector_count) {
                    cout << " ";
                }
                j = j + 1;
            }
            cout << endl;
            i = i + 1;
        }
    }

private:
    // Remove one directed neighbor entry
    bool remove_neighbor(int from_index, int to_index) {
        neighbor_node* current = sectors[from_index].neighbors;
        neighbor_node* previous = 0;
        while (current != 0) {
            if (current->sector_index == to_index) {
                if (previous == 0) {
                    sectors[from_index].neighbors = current->next;
                } else {
                    previous->next = current->next;
                }
                delete current;
                return true;
            }
            previous = current;
            current = current->next;
        }
        return false;
    }
};

#endif
