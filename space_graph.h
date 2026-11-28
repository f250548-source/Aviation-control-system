#ifndef SPACE_GRAPH_H
#define SPACE_GRAPH_H

#include <iostream>
#include <string>
using namespace std;

// ============================================================================
// Module 6 (Person 2): SpaceGraph - the airspace network
//
// Sectors are vertices, corridors are UNDIRECTED weighted edges (weight = travel
// time in seconds). The graph is stored TWICE and both copies are always kept
// in agreement:
//   1. Adjacency list   - one hand-built linked list of neighbor_node per sector
//   2. Adjacency matrix - a dynamically allocated 2D int array (int**)
//
// No-edge value: 0. Corridor weights must be strictly positive, so 0 can never
// be a real corridor and safely means "no corridor" in the matrix and in
// get_weight().
// ============================================================================

// Adjacency-list neighbor node for one direction of a corridor.
// NOTE: radar_mst.cpp (Person 3) uses this struct and its field names directly,
// so the name and the three fields must stay exactly as they are.
struct neighbor_node {
    int sector_index;      // index of the neighboring sector
    int weight;            // corridor travel time in seconds
    neighbor_node* next;   // next neighbor in this sector's list
};

// One vertex (sector) of the airspace
struct sector_node {
    string name;               // e.g. "Control_Tower", "RWY09"
    bool is_runway;            // true for landing runways
    neighbor_node* neighbors;  // head of this sector's adjacency list
};

class space_graph {
private:
    static const int INITIAL_CAPACITY = 4;  // small on purpose so resizing is exercised
    static const int NO_EDGE = 0;           // matrix value meaning "no corridor"

    sector_node* sectors;     // dynamic array of sectors (size = capacity)
    int** adjacency_matrix;   // capacity x capacity matrix of weights
    int sector_count;         // number of sectors actually in use
    int capacity;             // allocated rows/columns
    int corridor_count;       // number of undirected corridors

    // Double the capacity of the sector array and matrix, copying old data
    void grow_capacity();

    // Allocate a size x size matrix filled with NO_EDGE
    int** allocate_matrix(int size);

    // Free a matrix that has `size` rows
    void free_matrix(int** matrix, int size);

    // Find the neighbor node for from->to in the adjacency list (0 if absent)
    neighbor_node* find_list_entry(int from_index, int to_index);

    // Remove one directed neighbor entry from the adjacency list
    bool remove_neighbor(int from_index, int to_index);

    // Push one directed neighbor entry at the front of a sector's list
    void push_neighbor(int from_index, int to_index, int weight);

    // Runway detection by name ("RWY..." or "Runway...") for add_sector(name)
    bool name_looks_like_runway(string name);

    // Copying would share neighbor nodes and matrix rows (double delete),
    // so copying is disabled: declared private and intentionally not defined.
    space_graph(const space_graph& other);
    space_graph& operator=(const space_graph& other);

public:
    space_graph();
    ~space_graph();

    // ---- Sector management ----

    // Add a sector; runway status detected from its name. Returns its index,
    // the existing index for a duplicate name, or -1 on invalid input.
    int add_sector(string name);

    // Add a sector with explicit runway metadata (used by file_loader).
    int add_sector(string name, bool is_runway_flag);

    // ---- Corridor management (undirected) ----

    // Add or update a corridor in BOTH representations. Returns false on
    // invalid indexes, self-loops or non-positive weights.
    bool add_corridor(int from_index, int to_index, int weight_seconds);
    bool add_corridor(string from_name, string to_name, int weight_seconds);

    // Remove a corridor (both directions) from BOTH representations.
    // Returns true if a corridor existed and was removed.
    bool remove_corridor(int from_index, int to_index);
    bool remove_corridor(string from_name, string to_name);

    // ---- Queries ----

    // Corridor weight from the adjacency matrix; 0 = no corridor / invalid index
    int get_weight(int from_index, int to_index);

    // Corridor weight read from the adjacency LIST (used to verify consistency)
    int get_list_weight(int from_index, int to_index);

    int get_sector_index(string name);   // -1 if not found
    string get_sector_name(int index);   // "" if invalid
    int get_sector_count();
    int get_corridor_count();
    int get_capacity();
    bool is_runway(int index);
    bool is_valid_index(int index);

    // ---- Neighbor access for RadarSweep, RouteFinder and RadarMST ----

    // Head of a sector's adjacency list (read-only use; 0 if invalid/none)
    neighbor_node* get_neighbors(int index);

    // Position-based neighbor access (no pointers needed by the caller)
    int get_neighbor_count(int sector_index);
    int get_neighbor_index(int sector_index, int neighbor_position);   // -1 if invalid
    int get_neighbor_weight(int sector_index, int neighbor_position);  // 0 if invalid

    // ---- Verification / display ----

    // True if the adjacency list and adjacency matrix describe the same graph
    bool is_consistent();

    // Startup check against the project-manual airspace rules.
    // Prints [ATC Error] for each broken rule; returns true if all pass.
    bool validate_manual_rules();

    void display_adjacency_list();
    void display_adjacency_matrix();
};

// Name used in the project brief
typedef space_graph SpaceGraph;

#endif
