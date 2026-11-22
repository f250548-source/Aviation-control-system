#ifndef ROUTE_FINDER_H
#define ROUTE_FINDER_H

#include "flight.h"
#include "space_graph.h"
#include <iostream>
#include <string>
using namespace std;

// ============================================================================
// Module 8 (Person 2): RouteFinder - Dijkstra's shortest (fastest) path
//
// Uses the shared `route` struct from flight.h (Person 1), which game_engine
// already depends on:
//     struct route { string* sectors; int sector_count; int total_weight; };
// sectors[0] is the source, sectors[sector_count - 1] the destination.
//
// ROUTE MEMORY OWNERSHIP: a returned route owns a new[]-allocated `sectors`
// array. The CALLER must release it exactly once, either with
// `delete[] r.sectors;` (what game_engine already does) or with
// route_finder::free_route(r), which also resets the fields. An empty route
// (no path / invalid input) has sectors == 0 and sector_count == 0.
// `route` has no copy constructor, so copies share the same array: free it
// through only one copy.
// ============================================================================

// One heap item: a sector and the distance it was pushed with
struct heap_entry {
    int sector_index;
    int distance;
};

// Array-based binary min-heap ordered by distance.
// Dijkstra uses "lazy deletion": instead of decrease-key, a sector is pushed
// again with its new, smaller distance, and stale entries are skipped when
// popped. The array doubles when full.
class distance_min_heap {
private:
    heap_entry* entries;
    int capacity;
    int entry_count;

    void heapify_up(int position);     // move a new entry up while smaller than parent
    void heapify_down(int position);   // move the root down while larger than a child
    void swap_entries(int first, int second);
    void grow();

    distance_min_heap(const distance_min_heap& other);              // copying disabled
    distance_min_heap& operator=(const distance_min_heap& other);

public:
    distance_min_heap(int initial_capacity);
    ~distance_min_heap();
    void insert(int sector_index, int distance);
    heap_entry extract_min();   // returns {-1, -1} if empty
    bool is_empty();
    int size();
};

class route_finder {
private:
    space_graph* graph_ref;   // non-owning; the engine owns the graph

    static const int INFINITE_DISTANCE = 1000000000;

    // Dijkstra from source over the CURRENT graph. Fills distance[] (seconds,
    // INFINITE_DISTANCE if unreachable) and parent[] (-1 for source/unreached).
    void run_dijkstra(int source, int* distance, int* parent);

    // Walk parent[] back from destination, then reverse into source->dest order
    route build_route(int source, int destination, int* distance, int* parent);

    // Shared validation + Dijkstra + reconstruction.
    // require_runway: destination must be a runway. verbose: print errors.
    route compute_route(int source, int destination, bool require_runway, bool verbose);

    route_finder(const route_finder& other);              // copying disabled
    route_finder& operator=(const route_finder& other);

public:
    route_finder(space_graph* graph);
    ~route_finder();

    // Fastest route from any sector to any sector
    route find_shortest_path(int source, int destination);

    // Fastest route to a runway (destination must be a runway)
    route find_fastest_route(int source, int dest_runway);

    // Name-based form called by game_engine::option_reroute_flight
    route find_fastest_route(string start_name, string end_name);

    // One Dijkstra run, then pick the reachable candidate runway with the
    // lowest total cost (e.g. the emergency runways RWY09 / RWY12)
    route find_route_to_nearest(int source, int* candidate_runways, int candidate_count);

    // Print source, intermediate sectors, destination and total seconds
    void print_route(const route& r);

    // Sector index that follows current_sector in the route; -1 if the route
    // is empty, current_sector is not on it, or it is already the destination.
    int next_hop(const route& r, int current_sector);

    // Name form used every tick by game_engine::auto_move_flights.
    // Returns "" (flight stays put) when already at the target, on no route,
    // or on invalid names. Computes and frees its own route internally.
    string next_hop(string current_sector, string target_runway);

    // Release a route's sector array and reset it to the empty route
    void free_route(route& r);
};

// Name used in the project brief
typedef route_finder RouteFinder;

#endif
