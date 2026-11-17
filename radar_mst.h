#ifndef RADAR_MST_H
#define RADAR_MST_H

#include "space_graph.h"
#include <iostream>
#include <string>
using namespace std;

// Undirected weighted corridor used by Kruskal
struct edge {
    int from;
    int to;
    int weight;
};

// Module 9: Minimum Spanning Tree over the airspace graph (Kruskal)
class radar_mst {
private:
    space_graph* graph_ref;
    edge* mst_edges;
    int mst_edge_count;
    int mst_total_cost;
    int mst_capacity;

    // ---- Union-Find (Disjoint Set) internals ----
    int* parent_array;
    int* rank_array;
    int uf_size;

    // Ensure MST edge storage can hold needed_count edges
    void ensure_mst_capacity(int needed_count);

    // Collect unique undirected edges from the adjacency list
    edge* collect_all_edges(int& edge_count);

    // Hand-written merge for merge sort
    void merge_edge_ranges(edge* edge_array, int left, int mid, int right);

    // Hand-written merge sort on edge weights (ascending)
    void sort_edges(edge* edge_array, int left, int right);

    // Initialize union-find for n sectors
    void uf_init(int n);

    // Find with path compression
    int uf_find(int x);

    // Union by rank; returns true if sets were merged
    bool uf_union(int a, int b);

    // Free union-find arrays
    void uf_clear();

    // BFS connectivity check on a temporary edge list (hand-written queue)
    bool is_connected_with_edges(edge* edge_list, int edge_count, int sector_count,
                                 int ignore_index);

public:
    radar_mst(space_graph* graph);

    // Free MST and union-find memory
    ~radar_mst();

    // Run Kruskal: sort edges, then union-find accept non-cycle edges
    void compute_mst();

    // Print chosen corridors and total cost
    void display_mst();

    // Remove each MST edge one at a time; prove network disconnects
    void verify_connectivity();

    // Accessors for game_engine / menu
    int get_mst_edge_count();
    int get_mst_total_cost();
};

#endif
