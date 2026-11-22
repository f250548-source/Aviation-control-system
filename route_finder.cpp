#include "route_finder.h"
using namespace std;

// ============================================================================
// distance_min_heap
// ============================================================================

distance_min_heap::distance_min_heap(int initial_capacity) {
    if (initial_capacity < 4) {
        initial_capacity = 4;
    }
    capacity = initial_capacity;
    entries = new heap_entry[capacity];
    entry_count = 0;
}

distance_min_heap::~distance_min_heap() {
    delete[] entries;
    entries = 0;
}

void distance_min_heap::swap_entries(int first, int second) {
    heap_entry temp = entries[first];
    entries[first] = entries[second];
    entries[second] = temp;
}

// Copy into an array twice as large (only needed if many re-pushes happen)
void distance_min_heap::grow() {
    int new_capacity = capacity * 2;
    heap_entry* bigger = new heap_entry[new_capacity];
    int i = 0;
    while (i < entry_count) {
        bigger[i] = entries[i];
        i = i + 1;
    }
    delete[] entries;
    entries = bigger;
    capacity = new_capacity;
}

// Parent of position p is (p - 1) / 2
void distance_min_heap::heapify_up(int position) {
    while (position > 0) {
        int parent = (position - 1) / 2;
        if (entries[position].distance >= entries[parent].distance) {
            break;
        }
        swap_entries(position, parent);
        position = parent;
    }
}

// Children of position p are 2p + 1 and 2p + 2; swap with the smaller child
void distance_min_heap::heapify_down(int position) {
    while (true) {
        int left = 2 * position + 1;
        int right = 2 * position + 2;
        int smallest = position;
        if (left < entry_count && entries[left].distance < entries[smallest].distance) {
            smallest = left;
        }
        if (right < entry_count && entries[right].distance < entries[smallest].distance) {
            smallest = right;
        }
        if (smallest == position) {
            break;
        }
        swap_entries(position, smallest);
        position = smallest;
    }
}

// Add at the end, then bubble up: O(log n)
void distance_min_heap::insert(int sector_index, int distance) {
    if (entry_count >= capacity) {
        grow();
    }
    entries[entry_count].sector_index = sector_index;
    entries[entry_count].distance = distance;
    entry_count = entry_count + 1;
    heapify_up(entry_count - 1);
}

// Take the root, move the last entry to the root, then sift down: O(log n)
heap_entry distance_min_heap::extract_min() {
    heap_entry result;
    result.sector_index = -1;
    result.distance = -1;
    if (entry_count == 0) {
        return result;
    }
    result = entries[0];
    entry_count = entry_count - 1;
    if (entry_count > 0) {
        entries[0] = entries[entry_count];
        heapify_down(0);
    }
    return result;
}

bool distance_min_heap::is_empty() {
    return entry_count == 0;
}

int distance_min_heap::size() {
    return entry_count;
}

// ============================================================================
// route_finder - construction
// ============================================================================

route_finder::route_finder(space_graph* graph) {
    graph_ref = graph;
}

// The graph belongs to game_engine; only drop the reference
route_finder::~route_finder() {
    graph_ref = 0;
}

// ============================================================================
// Dijkstra
// ============================================================================

// 1. All distances start at infinity except the source (0).
// 2. Repeatedly pop the sector with the smallest known distance. Skip it if it
//    was already settled or the entry is stale (an older, larger distance).
// 3. Settle it and relax each corridor: if going through it is faster,
//    update the neighbor's distance and parent and push it again.
// Corridor weights are read from the live graph, so a corridor removed by a
// cyber attack is simply not there any more.
void route_finder::run_dijkstra(int source, int* distance, int* parent) {
    int sector_count = graph_ref->get_sector_count();
    bool* settled = new bool[sector_count];
    int i = 0;
    while (i < sector_count) {
        distance[i] = INFINITE_DISTANCE;
        parent[i] = -1;
        settled[i] = false;
        i = i + 1;
    }

    distance_min_heap frontier(sector_count);
    distance[source] = 0;
    frontier.insert(source, 0);

    while (!frontier.is_empty()) {
        heap_entry closest = frontier.extract_min();
        int current = closest.sector_index;
        if (settled[current] || closest.distance > distance[current]) {
            continue;   // stale duplicate entry
        }
        settled[current] = true;

        neighbor_node* neighbor = graph_ref->get_neighbors(current);
        while (neighbor != 0) {
            int next_index = neighbor->sector_index;
            int weight = neighbor->weight;
            // Dijkstra is only correct for non-negative weights; the graph
            // already rejects weight <= 0, this is a defensive second check.
            if (weight > 0 && !settled[next_index] &&
                distance[current] + weight < distance[next_index]) {
                distance[next_index] = distance[current] + weight;
                parent[next_index] = current;
                frontier.insert(next_index, distance[next_index]);
            }
            neighbor = neighbor->next;
        }
    }
    delete[] settled;
}

// Follow parent links destination -> source into a temporary array, then copy
// it backwards so the route reads source -> destination.
route route_finder::build_route(int source, int destination, int* distance, int* parent) {
    route result;
    if (distance[destination] >= INFINITE_DISTANCE) {
        return result;   // unreachable: empty route
    }

    int sector_count = graph_ref->get_sector_count();
    int* reverse_path = new int[sector_count];
    int path_length = 0;
    int walk = destination;
    while (walk != -1 && path_length < sector_count) {
        reverse_path[path_length] = walk;
        path_length = path_length + 1;
        if (walk == source) {
            break;
        }
        walk = parent[walk];
    }

    result.sectors = new string[path_length];
    result.sector_count = path_length;
    result.total_weight = distance[destination];
    int i = 0;
    while (i < path_length) {
        result.sectors[i] = graph_ref->get_sector_name(reverse_path[path_length - 1 - i]);
        i = i + 1;
    }
    delete[] reverse_path;
    return result;
}

route route_finder::compute_route(int source, int destination, bool require_runway,
                                  bool verbose) {
    route result;
    if (graph_ref == 0) {
        if (verbose) {
            cout << "[ATC Error] No airspace graph." << endl;
        }
        return result;
    }
    if (!graph_ref->is_valid_index(source) || !graph_ref->is_valid_index(destination)) {
        if (verbose) {
            cout << "[ATC Error] Route endpoints not found." << endl;
        }
        return result;
    }
    if (require_runway && !graph_ref->is_runway(destination)) {
        if (verbose) {
            cout << "[ATC Error] Destination " << graph_ref->get_sector_name(destination)
                 << " is not a runway." << endl;
        }
        return result;
    }

    int sector_count = graph_ref->get_sector_count();
    int* distance = new int[sector_count];
    int* parent = new int[sector_count];
    run_dijkstra(source, distance, parent);
    result = build_route(source, destination, distance, parent);
    delete[] distance;
    delete[] parent;

    if (result.sector_count == 0 && verbose) {
        cout << "[ATC Error] No route from " << graph_ref->get_sector_name(source)
             << " to " << graph_ref->get_sector_name(destination) << endl;
    }
    return result;
}

// ============================================================================
// Public interface
// ============================================================================

route route_finder::find_shortest_path(int source, int destination) {
    return compute_route(source, destination, false, true);
}

route route_finder::find_fastest_route(int source, int dest_runway) {
    return compute_route(source, dest_runway, true, true);
}

route route_finder::find_fastest_route(string start_name, string end_name) {
    if (graph_ref == 0) {
        cout << "[ATC Error] No airspace graph." << endl;
        route empty;
        return empty;
    }
    return compute_route(graph_ref->get_sector_index(start_name),
                         graph_ref->get_sector_index(end_name), true, true);
}

// A single Dijkstra run gives distances to every runway at once
route route_finder::find_route_to_nearest(int source, int* candidate_runways,
                                          int candidate_count) {
    route result;
    if (graph_ref == 0 || candidate_runways == 0 || candidate_count <= 0) {
        cout << "[ATC Error] No candidate runways given." << endl;
        return result;
    }
    if (!graph_ref->is_valid_index(source)) {
        cout << "[ATC Error] Route endpoints not found." << endl;
        return result;
    }

    int sector_count = graph_ref->get_sector_count();
    int* distance = new int[sector_count];
    int* parent = new int[sector_count];
    run_dijkstra(source, distance, parent);

    int best_runway = -1;
    int i = 0;
    while (i < candidate_count) {
        int runway = candidate_runways[i];
        if (graph_ref->is_valid_index(runway) && graph_ref->is_runway(runway) &&
            distance[runway] < INFINITE_DISTANCE) {
            cout << "  Candidate " << graph_ref->get_sector_name(runway) << ": "
                 << distance[runway] << " seconds" << endl;
            if (best_runway == -1 || distance[runway] < distance[best_runway]) {
                best_runway = runway;
            }
        } else if (graph_ref->is_valid_index(runway)) {
            cout << "  Candidate " << graph_ref->get_sector_name(runway)
                 << ": unreachable" << endl;
        }
        i = i + 1;
    }

    if (best_runway != -1) {
        result = build_route(source, best_runway, distance, parent);
    } else {
        cout << "[ATC Error] No candidate runway is reachable from "
             << graph_ref->get_sector_name(source) << endl;
    }
    delete[] distance;
    delete[] parent;
    return result;
}

void route_finder::print_route(const route& r) {
    if (r.sector_count == 0 || r.sectors == 0) {
        cout << "[ATC Error] No route available (destination unreachable)." << endl;
        return;
    }
    cout << "Fastest route: ";
    int i = 0;
    while (i < r.sector_count) {
        cout << r.sectors[i];
        if (i + 1 < r.sector_count) {
            cout << " -> ";
        }
        i = i + 1;
    }
    cout << endl;

    cout << "  Source: " << r.sectors[0] << " | Via: ";
    if (r.sector_count <= 2) {
        cout << "(direct)";
    }
    i = 1;
    while (i < r.sector_count - 1) {
        cout << r.sectors[i] << " ";
        i = i + 1;
    }
    cout << "| Destination: " << r.sectors[r.sector_count - 1] << endl;
    cout << "  Total travel time: " << r.total_weight << " seconds ("
         << (r.sector_count - 1) << " corridor(s))" << endl;
}

int route_finder::next_hop(const route& r, int current_sector) {
    if (graph_ref == 0 || r.sectors == 0 || r.sector_count == 0) {
        return -1;
    }
    string current_name = graph_ref->get_sector_name(current_sector);
    if (current_name.length() == 0) {
        return -1;
    }
    int i = 0;
    while (i < r.sector_count) {
        if (r.sectors[i] == current_name) {
            if (i == r.sector_count - 1) {
                return -1;   // already at the destination
            }
            return graph_ref->get_sector_index(r.sectors[i + 1]);
        }
        i = i + 1;
    }
    return -1;   // current sector is not on this route
}

// Quiet on purpose: called every tick for every active flight
string route_finder::next_hop(string current_sector, string target_runway) {
    if (graph_ref == 0) {
        return "";
    }
    int source = graph_ref->get_sector_index(current_sector);
    int target = graph_ref->get_sector_index(target_runway);
    route path = compute_route(source, target, false, false);
    string hop = "";
    int next_index = next_hop(path, source);
    if (next_index >= 0) {
        hop = graph_ref->get_sector_name(next_index);
    }
    free_route(path);
    return hop;
}

void route_finder::free_route(route& r) {
    if (r.sectors != 0) {
        delete[] r.sectors;
    }
    r.sectors = 0;
    r.sector_count = 0;
    r.total_weight = 0;
}
