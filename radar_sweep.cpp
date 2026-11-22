#include "radar_sweep.h"
using namespace std;

// Sector name of the Control Tower in airspace.txt
static const string CONTROL_TOWER_NAME = "Control_Tower";

// ============================================================================
// sector_queue - circular array queue
// ============================================================================

sector_queue::sector_queue(int max_items) {
    if (max_items < 1) {
        max_items = 1;
    }
    capacity = max_items;
    items = new int[capacity];
    front_index = 0;
    item_count = 0;
}

sector_queue::~sector_queue() {
    delete[] items;
    items = 0;
}

// Rear position wraps around with modulo, so freed front slots are reused
bool sector_queue::enqueue(int sector_index) {
    if (item_count >= capacity) {
        return false;
    }
    int rear_index = (front_index + item_count) % capacity;
    items[rear_index] = sector_index;
    item_count = item_count + 1;
    return true;
}

int sector_queue::dequeue() {
    if (item_count == 0) {
        return -1;
    }
    int value = items[front_index];
    front_index = (front_index + 1) % capacity;
    item_count = item_count - 1;
    return value;
}

bool sector_queue::is_empty() {
    return item_count == 0;
}

// ============================================================================
// radar_sweep - construction
// ============================================================================

radar_sweep::radar_sweep(space_graph* graph) {
    graph_ref = graph;
}

// The graph belongs to game_engine; only drop the reference
radar_sweep::~radar_sweep() {
    graph_ref = 0;
}

bool radar_sweep::graph_ready() {
    if (graph_ref == 0) {
        cout << "[ATC Error] No airspace graph." << endl;
        return false;
    }
    return true;
}

// ============================================================================
// Private traversal helpers
// ============================================================================

// Breadth-first search. A sector is marked visited when it is ENQUEUED, so it
// can never be queued twice. Its hop level is its parent's level + 1, which
// is the fewest-corridor distance from the source.
int radar_sweep::run_bfs(int source, int max_hops, int* order, int* hop_level) {
    int sector_count = graph_ref->get_sector_count();
    bool* visited = new bool[sector_count];
    int i = 0;
    while (i < sector_count) {
        visited[i] = false;
        hop_level[i] = -1;
        i = i + 1;
    }

    sector_queue frontier(sector_count);
    int reached = 0;
    visited[source] = true;
    hop_level[source] = 0;
    frontier.enqueue(source);

    while (!frontier.is_empty()) {
        int current = frontier.dequeue();
        order[reached] = current;
        reached = reached + 1;

        // Do not expand beyond the hop limit (used by bfs_within_2_hops)
        if (max_hops >= 0 && hop_level[current] >= max_hops) {
            continue;
        }

        neighbor_node* neighbor = graph_ref->get_neighbors(current);
        while (neighbor != 0) {
            int next_index = neighbor->sector_index;
            if (!visited[next_index]) {
                visited[next_index] = true;
                hop_level[next_index] = hop_level[current] + 1;
                frontier.enqueue(next_index);
            }
            neighbor = neighbor->next;
        }
    }

    delete[] visited;
    return reached;
}

// Depth-first search using recursion (the call stack acts as the DFS stack):
// visit a sector, then fully explore each unvisited neighbor before the next.
void radar_sweep::run_dfs(int current, bool* visited, int* order, int& order_count) {
    visited[current] = true;
    order[order_count] = current;
    order_count = order_count + 1;

    neighbor_node* neighbor = graph_ref->get_neighbors(current);
    while (neighbor != 0) {
        if (!visited[neighbor->sector_index]) {
            run_dfs(neighbor->sector_index, visited, order, order_count);
        }
        neighbor = neighbor->next;
    }
}

// Print "Hop k: ..." lines. BFS order is already sorted by hop level.
void radar_sweep::print_levels(int* order, int* hop_level, int reached, int max_level) {
    int level = 0;
    while (level <= max_level) {
        cout << "  Hop " << level << ": ";
        bool any = false;
        int i = 0;
        while (i < reached) {
            if (hop_level[order[i]] == level) {
                cout << graph_ref->get_sector_name(order[i]) << " ";
                any = true;
            }
            i = i + 1;
        }
        if (!any) {
            cout << "(none)";
        }
        cout << endl;
        level = level + 1;
    }
}

// ============================================================================
// BFS
// ============================================================================

int radar_sweep::bfs_from(int source) {
    if (!graph_ready()) {
        return 0;
    }
    if (!graph_ref->is_valid_index(source)) {
        cout << "[ATC Error] Invalid sector index for BFS: " << source << endl;
        return 0;
    }
    int sector_count = graph_ref->get_sector_count();
    int* order = new int[sector_count];
    int* hop_level = new int[sector_count];
    int reached = run_bfs(source, -1, order, hop_level);

    cout << "BFS from " << graph_ref->get_sector_name(source) << ": ";
    int i = 0;
    while (i < reached) {
        cout << graph_ref->get_sector_name(order[i]);
        if (i + 1 < reached) {
            cout << " -> ";
        }
        i = i + 1;
    }
    cout << endl;

    // Deepest level reached is the level of the last dequeued sector
    print_levels(order, hop_level, reached, hop_level[order[reached - 1]]);
    cout << "Sectors reached: " << reached << " of " << sector_count << endl;

    delete[] order;
    delete[] hop_level;
    return reached;
}

int radar_sweep::bfs_from(string start_name) {
    if (!graph_ready()) {
        return 0;
    }
    int source = graph_ref->get_sector_index(start_name);
    if (source < 0) {
        cout << "[ATC Error] Sector not found: " << start_name << endl;
        return 0;
    }
    return bfs_from(source);
}

// Hop count = number of corridors, NOT travel time in seconds
void radar_sweep::bfs_within_2_hops(int source) {
    if (!graph_ready()) {
        return;
    }
    if (!graph_ref->is_valid_index(source)) {
        cout << "[ATC Error] Invalid sector index for BFS: " << source << endl;
        return;
    }
    int sector_count = graph_ref->get_sector_count();
    int* order = new int[sector_count];
    int* hop_level = new int[sector_count];
    int reached = run_bfs(source, 2, order, hop_level);

    cout << "Sectors within 2 hops of " << graph_ref->get_sector_name(source)
         << ":" << endl;
    print_levels(order, hop_level, reached, 2);
    cout << "Total within 2 hops (including source): " << reached << endl;

    delete[] order;
    delete[] hop_level;
}

void radar_sweep::bfs_within_2_hops(string start_name) {
    if (!graph_ready()) {
        return;
    }
    int source = graph_ref->get_sector_index(start_name);
    if (source < 0) {
        cout << "[ATC Error] Sector not found: " << start_name << endl;
        return;
    }
    bfs_within_2_hops(source);
}

// ============================================================================
// DFS
// ============================================================================

int radar_sweep::dfs_from(int source) {
    if (!graph_ready()) {
        return 0;
    }
    if (!graph_ref->is_valid_index(source)) {
        cout << "[ATC Error] Invalid sector index for DFS: " << source << endl;
        return 0;
    }
    int sector_count = graph_ref->get_sector_count();
    bool* visited = new bool[sector_count];
    int* order = new int[sector_count];
    int i = 0;
    while (i < sector_count) {
        visited[i] = false;
        i = i + 1;
    }
    int order_count = 0;
    run_dfs(source, visited, order, order_count);

    cout << "DFS from " << graph_ref->get_sector_name(source) << ": ";
    i = 0;
    while (i < order_count) {
        cout << graph_ref->get_sector_name(order[i]);
        if (i + 1 < order_count) {
            cout << " -> ";
        }
        i = i + 1;
    }
    cout << endl;
    cout << "Sectors reached: " << order_count << " of " << sector_count << endl;

    delete[] visited;
    delete[] order;
    return order_count;
}

int radar_sweep::dfs_from(string start_name) {
    if (!graph_ready()) {
        return 0;
    }
    int source = graph_ref->get_sector_index(start_name);
    if (source < 0) {
        cout << "[ATC Error] Sector not found: " << start_name << endl;
        return 0;
    }
    return dfs_from(source);
}

// ============================================================================
// Reachability
// ============================================================================

// A sector is reachable exactly when BFS assigns it a hop level
int radar_sweep::hop_distance(int from_index, int to_index) {
    if (graph_ref == 0 || !graph_ref->is_valid_index(from_index) ||
        !graph_ref->is_valid_index(to_index)) {
        return -1;
    }
    int sector_count = graph_ref->get_sector_count();
    int* order = new int[sector_count];
    int* hop_level = new int[sector_count];
    run_bfs(from_index, -1, order, hop_level);
    int hops = hop_level[to_index];
    delete[] order;
    delete[] hop_level;
    return hops;
}

bool radar_sweep::is_reachable(int from_index, int to_index) {
    return hop_distance(from_index, to_index) >= 0;
}

// Quiet by design: game_engine prints its own messages around this check
bool radar_sweep::is_reachable(string from_name, string to_name) {
    if (graph_ref == 0) {
        return false;
    }
    return is_reachable(graph_ref->get_sector_index(from_name),
                        graph_ref->get_sector_index(to_name));
}

// ============================================================================
// Missing-flight search
// ============================================================================

bool radar_sweep::missing_search_core(int tower_index, int last_known_index) {
    if (tower_index < 0) {
        cout << "[ATC Error] Control Tower sector not found in airspace." << endl;
        return false;
    }
    if (!graph_ref->is_valid_index(last_known_index)) {
        cout << "[ATC Error] Last known sector of the missing flight is invalid." << endl;
        return false;
    }
    string last_known_name = graph_ref->get_sector_name(last_known_index);
    cout << "Radar searching for missing flight near " << last_known_name
         << " (BFS from " << graph_ref->get_sector_name(tower_index) << ")..." << endl;

    int hops = hop_distance(tower_index, last_known_index);
    if (hops >= 0) {
        cout << "Roger. Contact established with sector " << last_known_name
             << " (" << hops << " hop(s) from tower)." << endl;
        return true;
    }
    cout << "[ATC Error] Sector " << last_known_name
         << " unreachable from " << graph_ref->get_sector_name(tower_index) << endl;
    return false;
}

// Index form from the brief: the tower is located by name through the graph
bool radar_sweep::search_missing_flight(int last_known_sector) {
    if (!graph_ready()) {
        return false;
    }
    return missing_search_core(graph_ref->get_sector_index(CONTROL_TOWER_NAME),
                               last_known_sector);
}

// Name form called by game_engine::option_search_missing
bool radar_sweep::search_missing_flight(string tower_name, string last_known_sector) {
    if (!graph_ready()) {
        return false;
    }
    int last_known_index = graph_ref->get_sector_index(last_known_sector);
    if (last_known_index < 0) {
        cout << "[ATC Error] Sector not found: " << last_known_sector << endl;
        return false;
    }
    return missing_search_core(graph_ref->get_sector_index(tower_name),
                               last_known_index);
}
