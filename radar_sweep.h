#ifndef RADAR_SWEEP_H
#define RADAR_SWEEP_H

#include "space_graph.h"
#include <iostream>
#include <string>
using namespace std;

// ============================================================================
// Module 7 (Person 2): RadarSweep - BFS, DFS, reachability, missing-flight search
//
// Works on the engine's existing space_graph through a pointer; it never copies
// the graph, so a corridor removed by a cyber attack is seen immediately.
// All traversal storage (visited array, queue, order arrays) is allocated per
// call and freed before the call returns.
//
// Every operation has an index-based form (as in the project brief) and a
// name-based form (the one game_engine already calls).
// ============================================================================

// Fixed-capacity circular queue of sector indexes used by BFS.
// Each sector is enqueued at most once per BFS, so capacity = sector count.
class sector_queue {
private:
    int* items;
    int capacity;
    int front_index;   // position of the next item to dequeue
    int item_count;

    sector_queue(const sector_queue& other);              // copying disabled
    sector_queue& operator=(const sector_queue& other);

public:
    sector_queue(int max_items);
    ~sector_queue();
    bool enqueue(int sector_index);   // false if full
    int dequeue();                    // -1 if empty
    bool is_empty();
};

class radar_sweep {
private:
    space_graph* graph_ref;   // non-owning; game_engine owns the graph

    // Core BFS from source. Fills hop_level[i] with the number of corridors
    // from source to i (-1 if not reached) and order[] with sectors in the
    // order they were dequeued. Stops expanding past max_hops (-1 = no limit).
    // Returns how many sectors were reached.
    int run_bfs(int source, int max_hops, int* order, int* hop_level);

    // Recursive DFS: marks visited and appends to order[] in visit order
    void run_dfs(int current, bool* visited, int* order, int& order_count);

    // Shared logic of both search_missing_flight overloads
    bool missing_search_core(int tower_index, int last_known_index);

    // Print BFS results grouped by hop level
    void print_levels(int* order, int* hop_level, int reached, int max_level);

    bool graph_ready();

    radar_sweep(const radar_sweep& other);              // copying disabled
    radar_sweep& operator=(const radar_sweep& other);

public:
    radar_sweep(space_graph* graph);
    ~radar_sweep();

    // BFS: print sectors level by level and in visit order.
    // Returns the number of sectors visited (0 for an invalid source).
    int bfs_from(int source);
    int bfs_from(string start_name);

    // Sectors within two corridors (hops) of source, grouped by hop level
    void bfs_within_2_hops(int source);
    void bfs_within_2_hops(string start_name);

    // DFS (recursive): print visit order. Returns sectors visited.
    int dfs_from(int source);
    int dfs_from(string start_name);

    // True if a path of corridors exists (false for invalid indexes)
    bool is_reachable(int from_index, int to_index);
    bool is_reachable(string from_name, string to_name);

    // Fewest corridors between two sectors, -1 if unreachable/invalid
    int hop_distance(int from_index, int to_index);

    // BFS from the Control Tower to see whether the missing flight's last
    // known sector can still be reached by radar
    bool search_missing_flight(int last_known_sector);
    bool search_missing_flight(string tower_name, string last_known_sector);
};

// Name used in the project brief
typedef radar_sweep RadarSweep;

#endif
