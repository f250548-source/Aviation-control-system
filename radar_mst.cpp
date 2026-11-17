#include "radar_mst.h"
using namespace std;

// Grow dynamic MST edge array if needed
void radar_mst::ensure_mst_capacity(int needed_count) {
    if (needed_count <= mst_capacity) {
        return;
    }
    int new_capacity = mst_capacity * 2;
    if (new_capacity < needed_count) {
        new_capacity = needed_count;
    }
    if (new_capacity < 4) {
        new_capacity = 4;
    }
    edge* new_array = new edge[new_capacity];
    int i = 0;
    while (i < mst_edge_count) {
        new_array[i] = mst_edges[i];
        i = i + 1;
    }
    delete[] mst_edges;
    mst_edges = new_array;
    mst_capacity = new_capacity;
}

// Walk adjacency lists once; keep only from < to to avoid duplicates
edge* radar_mst::collect_all_edges(int& edge_count) {
    edge_count = 0;
    if (graph_ref == 0) {
        return 0;
    }
    int sector_count = graph_ref->get_sector_count();
    // Upper bound: every neighbor entry (each undirected edge counted twice)
    int capacity = 0;
    int i = 0;
    while (i < sector_count) {
        neighbor_node* neighbor = graph_ref->get_neighbors(i);
        while (neighbor != 0) {
            capacity = capacity + 1;
            neighbor = neighbor->next;
        }
        i = i + 1;
    }
    if (capacity == 0) {
        return 0;
    }

    edge* edges = new edge[capacity];
    i = 0;
    while (i < sector_count) {
        neighbor_node* neighbor = graph_ref->get_neighbors(i);
        while (neighbor != 0) {
            int j = neighbor->sector_index;
            // Store each undirected edge once
            if (i < j) {
                edges[edge_count].from = i;
                edges[edge_count].to = j;
                edges[edge_count].weight = neighbor->weight;
                edge_count = edge_count + 1;
            }
            neighbor = neighbor->next;
        }
        i = i + 1;
    }
    return edges;
}

// Merge two sorted halves of edge_array by weight
void radar_mst::merge_edge_ranges(edge* edge_array, int left, int mid, int right) {
    int left_size = mid - left + 1;
    int right_size = right - mid;

    edge* left_array = new edge[left_size];
    edge* right_array = new edge[right_size];

    int i = 0;
    while (i < left_size) {
        left_array[i] = edge_array[left + i];
        i = i + 1;
    }
    int j = 0;
    while (j < right_size) {
        right_array[j] = edge_array[mid + 1 + j];
        j = j + 1;
    }

    i = 0;
    j = 0;
    int k = left;
    while (i < left_size && j < right_size) {
        if (left_array[i].weight <= right_array[j].weight) {
            edge_array[k] = left_array[i];
            i = i + 1;
        } else {
            edge_array[k] = right_array[j];
            j = j + 1;
        }
        k = k + 1;
    }
    while (i < left_size) {
        edge_array[k] = left_array[i];
        i = i + 1;
        k = k + 1;
    }
    while (j < right_size) {
        edge_array[k] = right_array[j];
        j = j + 1;
        k = k + 1;
    }

    delete[] left_array;
    delete[] right_array;
}

// Recursive merge sort on edges by ascending weight
void radar_mst::sort_edges(edge* edge_array, int left, int right) {
    if (left >= right) {
        return;
    }
    int mid = left + (right - left) / 2;
    sort_edges(edge_array, left, mid);
    sort_edges(edge_array, mid + 1, right);
    merge_edge_ranges(edge_array, left, mid, right);
}

// Allocate parent/rank arrays and set each node as its own parent
void radar_mst::uf_init(int n) {
    uf_clear();
    uf_size = n;
    parent_array = new int[n];
    rank_array = new int[n];
    int i = 0;
    while (i < n) {
        parent_array[i] = i;
        rank_array[i] = 0;
        i = i + 1;
    }
}

// Path-compressed find
int radar_mst::uf_find(int x) {
    if (parent_array[x] != x) {
        parent_array[x] = uf_find(parent_array[x]);
    }
    return parent_array[x];
}

// Union by rank; false if already same set (would form a cycle)
bool radar_mst::uf_union(int a, int b) {
    int root_a = uf_find(a);
    int root_b = uf_find(b);
    if (root_a == root_b) {
        return false;
    }
    if (rank_array[root_a] < rank_array[root_b]) {
        parent_array[root_a] = root_b;
    } else if (rank_array[root_a] > rank_array[root_b]) {
        parent_array[root_b] = root_a;
    } else {
        parent_array[root_b] = root_a;
        rank_array[root_a] = rank_array[root_a] + 1;
    }
    return true;
}

// Release union-find storage
void radar_mst::uf_clear() {
    if (parent_array != 0) {
        delete[] parent_array;
        parent_array = 0;
    }
    if (rank_array != 0) {
        delete[] rank_array;
        rank_array = 0;
    }
    uf_size = 0;
}

// Hand-written BFS over edge_list, skipping ignore_index; checks one component covers all used vertices
bool radar_mst::is_connected_with_edges(edge* edge_list, int edge_count,
                                        int sector_count, int ignore_index) {
    if (sector_count <= 0) {
        return true;
    }

    // Build temporary adjacency lists from the edge list
    neighbor_node** temp_adj = new neighbor_node*[sector_count];
    int i = 0;
    while (i < sector_count) {
        temp_adj[i] = 0;
        i = i + 1;
    }

    i = 0;
    while (i < edge_count) {
        if (i != ignore_index) {
            int u = edge_list[i].from;
            int v = edge_list[i].to;
            neighbor_node* a = new neighbor_node;
            a->sector_index = v;
            a->weight = edge_list[i].weight;
            a->next = temp_adj[u];
            temp_adj[u] = a;

            neighbor_node* b = new neighbor_node;
            b->sector_index = u;
            b->weight = edge_list[i].weight;
            b->next = temp_adj[v];
            temp_adj[v] = b;
        }
        i = i + 1;
    }

    // Mark which sectors appear in the MST (ignore isolated unused vertices)
    bool* used = new bool[sector_count];
    i = 0;
    while (i < sector_count) {
        used[i] = false;
        i = i + 1;
    }
    i = 0;
    while (i < edge_count) {
        used[edge_list[i].from] = true;
        used[edge_list[i].to] = true;
        i = i + 1;
    }

    int start = -1;
    i = 0;
    while (i < sector_count) {
        if (used[i]) {
            start = i;
            break;
        }
        i = i + 1;
    }
    if (start < 0) {
        // No edges at all
        i = 0;
        while (i < sector_count) {
            neighbor_node* current = temp_adj[i];
            while (current != 0) {
                neighbor_node* temp = current;
                current = current->next;
                delete temp;
            }
            i = i + 1;
        }
        delete[] temp_adj;
        delete[] used;
        return true;
    }

    bool* visited = new bool[sector_count];
    i = 0;
    while (i < sector_count) {
        visited[i] = false;
        i = i + 1;
    }

    int* bfs_queue = new int[sector_count];
    int front = 0;
    int rear = 0;
    visited[start] = true;
    bfs_queue[rear] = start;
    rear = rear + 1;

    while (front < rear) {
        int current = bfs_queue[front];
        front = front + 1;
        neighbor_node* neighbor = temp_adj[current];
        while (neighbor != 0) {
            int next_index = neighbor->sector_index;
            if (!visited[next_index]) {
                visited[next_index] = true;
                bfs_queue[rear] = next_index;
                rear = rear + 1;
            }
            neighbor = neighbor->next;
        }
    }

    bool connected = true;
    i = 0;
    while (i < sector_count) {
        if (used[i] && !visited[i]) {
            connected = false;
            break;
        }
        i = i + 1;
    }

    // Free temporary adjacency memory
    i = 0;
    while (i < sector_count) {
        neighbor_node* current = temp_adj[i];
        while (current != 0) {
            neighbor_node* temp = current;
            current = current->next;
            delete temp;
        }
        i = i + 1;
    }
    delete[] temp_adj;
    delete[] used;
    delete[] visited;
    delete[] bfs_queue;
    return connected;
}

radar_mst::radar_mst(space_graph* graph) {
    graph_ref = graph;
    mst_edges = 0;
    mst_edge_count = 0;
    mst_total_cost = 0;
    mst_capacity = 0;
    parent_array = 0;
    rank_array = 0;
    uf_size = 0;
}

radar_mst::~radar_mst() {
    if (mst_edges != 0) {
        delete[] mst_edges;
        mst_edges = 0;
    }
    uf_clear();
    graph_ref = 0;
    mst_edge_count = 0;
    mst_total_cost = 0;
    mst_capacity = 0;
}

// Kruskal MST: sort all corridors, accept edge if it joins two components
void radar_mst::compute_mst() {
    if (graph_ref == 0) {
        cout << "[ATC Error] No airspace graph for MST." << endl;
        return;
    }

    int sector_count = graph_ref->get_sector_count();
    if (sector_count <= 0) {
        cout << "[ATC Error] Airspace has no sectors." << endl;
        return;
    }

    // Reset previous MST result
    if (mst_edges != 0) {
        delete[] mst_edges;
        mst_edges = 0;
    }
    mst_edge_count = 0;
    mst_total_cost = 0;
    mst_capacity = 0;

    int all_edge_count = 0;
    edge* all_edges = collect_all_edges(all_edge_count);
    if (all_edges == 0 || all_edge_count == 0) {
        cout << "[ATC Error] No corridors available for MST." << endl;
        if (all_edges != 0) {
            delete[] all_edges;
        }
        return;
    }

    // Sort corridors by weight (merge sort)
    sort_edges(all_edges, 0, all_edge_count - 1);

    uf_init(sector_count);
    ensure_mst_capacity(sector_count);

    int i = 0;
    while (i < all_edge_count) {
        // Stop early once tree has V-1 edges
        if (mst_edge_count >= sector_count - 1) {
            break;
        }
        int u = all_edges[i].from;
        int v = all_edges[i].to;
        if (uf_union(u, v)) {
            ensure_mst_capacity(mst_edge_count + 1);
            mst_edges[mst_edge_count] = all_edges[i];
            mst_total_cost = mst_total_cost + all_edges[i].weight;
            mst_edge_count = mst_edge_count + 1;
        }
        i = i + 1;
    }

    delete[] all_edges;
    uf_clear();

    cout << "Radar MST computed with " << mst_edge_count
         << " corridors. Total cost = " << mst_total_cost << endl;
}

// Print each MST corridor with sector names and the total weight
void radar_mst::display_mst() {
    cout << "=== Radar Minimum Spanning Tree ===" << endl;
    if (mst_edge_count == 0 || mst_edges == 0) {
        cout << "No MST computed yet. Run compute_mst first." << endl;
        return;
    }
    int i = 0;
    while (i < mst_edge_count) {
        string from_name = graph_ref->get_sector_name(mst_edges[i].from);
        string to_name = graph_ref->get_sector_name(mst_edges[i].to);
        cout << "Corridor " << (i + 1) << ": "
             << from_name << " -- " << to_name
             << "  weight=" << mst_edges[i].weight << endl;
        i = i + 1;
    }
    cout << "Total MST cost: " << mst_total_cost << endl;
}

// Prove minimality: dropping any MST edge disconnects the tree
void radar_mst::verify_connectivity() {
    cout << "=== MST Minimality / Connectivity Check ===" << endl;
    if (mst_edge_count == 0 || mst_edges == 0 || graph_ref == 0) {
        cout << "[ATC Error] No MST available to verify." << endl;
        return;
    }

    int sector_count = graph_ref->get_sector_count();
    int i = 0;
    while (i < mst_edge_count) {
        string from_name = graph_ref->get_sector_name(mst_edges[i].from);
        string to_name = graph_ref->get_sector_name(mst_edges[i].to);

        bool still_connected = is_connected_with_edges(
            mst_edges, mst_edge_count, sector_count, i);

        if (still_connected) {
            cout << "WARNING: Removing " << from_name << " -- " << to_name
                 << " did NOT disconnect the network (unexpected)." << endl;
        } else {
            cout << "Removing corridor " << from_name << " -- " << to_name
                 << " disconnects the radar network (proves this edge is essential)."
                 << endl;
        }
        i = i + 1;
    }
    cout << "Verification complete: every MST edge is a bridge in the tree,"
         << " so the spanning tree is minimal." << endl;
}

int radar_mst::get_mst_edge_count() {
    return mst_edge_count;
}

int radar_mst::get_mst_total_cost() {
    return mst_total_cost;
}
