#include "space_graph.h"
#include <iomanip>
using namespace std;

// ============================================================================
// Construction / destruction
// ============================================================================

// Start with a small capacity; add_sector() grows it on demand
space_graph::space_graph() {
    capacity = INITIAL_CAPACITY;
    sector_count = 0;
    corridor_count = 0;
    sectors = new sector_node[capacity];
    int i = 0;
    while (i < capacity) {
        sectors[i].name = "";
        sectors[i].is_runway = false;
        sectors[i].neighbors = 0;
        i = i + 1;
    }
    adjacency_matrix = allocate_matrix(capacity);
}

// Free every adjacency-list node, every matrix row, then the arrays themselves
space_graph::~space_graph() {
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

    free_matrix(adjacency_matrix, capacity);
    adjacency_matrix = 0;

    sector_count = 0;
    corridor_count = 0;
    capacity = 0;
}

// ============================================================================
// Private helpers
// ============================================================================

// Build a size x size matrix where every cell starts as "no corridor"
int** space_graph::allocate_matrix(int size) {
    int** matrix = new int*[size];
    int row = 0;
    while (row < size) {
        matrix[row] = new int[size];
        int col = 0;
        while (col < size) {
            matrix[row][col] = NO_EDGE;
            col = col + 1;
        }
        row = row + 1;
    }
    return matrix;
}

// Delete each row first, then the array of row pointers
void space_graph::free_matrix(int** matrix, int size) {
    if (matrix == 0) {
        return;
    }
    int row = 0;
    while (row < size) {
        delete[] matrix[row];
        row = row + 1;
    }
    delete[] matrix;
}

// Double capacity. Sector records are copied into a bigger array; their
// adjacency-list heads are carried over by pointer (the list nodes themselves
// are NOT duplicated, so nothing is leaked or freed twice). Existing matrix
// weights are copied into the top-left corner of the bigger matrix.
void space_graph::grow_capacity() {
    int new_capacity = capacity * 2;
    if (new_capacity < INITIAL_CAPACITY) {
        new_capacity = INITIAL_CAPACITY;
    }

    // 1. Bigger sector array
    sector_node* new_sectors = new sector_node[new_capacity];
    int i = 0;
    while (i < new_capacity) {
        if (i < sector_count) {
            new_sectors[i] = sectors[i];   // copies name, flag and list head pointer
        } else {
            new_sectors[i].name = "";
            new_sectors[i].is_runway = false;
            new_sectors[i].neighbors = 0;
        }
        i = i + 1;
    }

    // 2. Bigger matrix, preserving every existing corridor weight
    int** new_matrix = allocate_matrix(new_capacity);
    int row = 0;
    while (row < sector_count) {
        int col = 0;
        while (col < sector_count) {
            new_matrix[row][col] = adjacency_matrix[row][col];
            col = col + 1;
        }
        row = row + 1;
    }

    // 3. Release the old storage (old array only; list nodes now belong to new array)
    delete[] sectors;
    free_matrix(adjacency_matrix, capacity);

    sectors = new_sectors;
    adjacency_matrix = new_matrix;
    capacity = new_capacity;
}

// Linear scan of one sector's list for the entry pointing at to_index
neighbor_node* space_graph::find_list_entry(int from_index, int to_index) {
    if (!is_valid_index(from_index)) {
        return 0;
    }
    neighbor_node* current = sectors[from_index].neighbors;
    while (current != 0) {
        if (current->sector_index == to_index) {
            return current;
        }
        current = current->next;
    }
    return 0;
}

// Insert a new neighbor node at the head of from_index's list (O(1))
void space_graph::push_neighbor(int from_index, int to_index, int weight) {
    neighbor_node* created = new neighbor_node;
    created->sector_index = to_index;
    created->weight = weight;
    created->next = sectors[from_index].neighbors;
    sectors[from_index].neighbors = created;
}

// Unlink and delete one directed entry (standard singly-linked-list delete)
bool space_graph::remove_neighbor(int from_index, int to_index) {
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

// Runways in this project are named "RWY09", "RWY12", ... ("Runway_12" also accepted)
bool space_graph::name_looks_like_runway(string name) {
    if (name.length() >= 3 && name.substr(0, 3) == "RWY") {
        return true;
    }
    if (name.length() >= 6 && name.substr(0, 6) == "Runway") {
        return true;
    }
    return false;
}

// ============================================================================
// Sector management
// ============================================================================

int space_graph::add_sector(string name) {
    return add_sector(name, name_looks_like_runway(name));
}

// Append a sector, growing storage first if it is full.
// Duplicate names are not added twice: the existing index is returned.
int space_graph::add_sector(string name, bool is_runway_flag) {
    if (name.length() == 0) {
        cout << "[ATC Error] Sector name cannot be empty." << endl;
        return -1;
    }
    int existing = get_sector_index(name);
    if (existing != -1) {
        return existing;
    }
    if (sector_count >= capacity) {
        grow_capacity();
    }
    int index = sector_count;
    sectors[index].name = name;
    sectors[index].is_runway = is_runway_flag;
    sectors[index].neighbors = 0;
    sector_count = sector_count + 1;
    return index;
}

// ============================================================================
// Corridor management
// ============================================================================

// Add an undirected corridor, or update its weight if it already exists.
// Both directions are written to the list AND the matrix so they never disagree.
bool space_graph::add_corridor(int from_index, int to_index, int weight_seconds) {
    if (!is_valid_index(from_index) || !is_valid_index(to_index)) {
        cout << "[ATC Error] Corridor endpoints not found." << endl;
        return false;
    }
    if (from_index == to_index) {
        cout << "[ATC Error] A corridor cannot connect a sector to itself." << endl;
        return false;
    }
    if (weight_seconds <= 0) {
        cout << "[ATC Error] Corridor weight must be positive." << endl;
        return false;
    }

    if (adjacency_matrix[from_index][to_index] != NO_EDGE) {
        // Existing corridor: update the weight in both directions, both representations
        neighbor_node* forward = find_list_entry(from_index, to_index);
        neighbor_node* backward = find_list_entry(to_index, from_index);
        if (forward != 0) {
            forward->weight = weight_seconds;
        }
        if (backward != 0) {
            backward->weight = weight_seconds;
        }
    } else {
        push_neighbor(from_index, to_index, weight_seconds);
        push_neighbor(to_index, from_index, weight_seconds);
        corridor_count = corridor_count + 1;
    }
    adjacency_matrix[from_index][to_index] = weight_seconds;
    adjacency_matrix[to_index][from_index] = weight_seconds;
    return true;
}

// Name-based version used by file_loader
bool space_graph::add_corridor(string from_name, string to_name, int weight_seconds) {
    return add_corridor(get_sector_index(from_name), get_sector_index(to_name),
                        weight_seconds);
}

// Remove both directions from the list and clear both matrix cells.
// Used by the cyber-attack event; BFS/DFS/Dijkstra/MST see the change at once
// because they all read this same graph.
bool space_graph::remove_corridor(int from_index, int to_index) {
    if (!is_valid_index(from_index) || !is_valid_index(to_index)) {
        cout << "[ATC Error] Corridor endpoints not found." << endl;
        return false;
    }
    bool removed_forward = remove_neighbor(from_index, to_index);
    bool removed_backward = remove_neighbor(to_index, from_index);
    bool existed = adjacency_matrix[from_index][to_index] != NO_EDGE;
    adjacency_matrix[from_index][to_index] = NO_EDGE;
    adjacency_matrix[to_index][from_index] = NO_EDGE;

    if (existed || removed_forward || removed_backward) {
        corridor_count = corridor_count - 1;
        return true;
    }
    return false;
}

// Name-based version used by game_engine (cyber escalation)
bool space_graph::remove_corridor(string from_name, string to_name) {
    return remove_corridor(get_sector_index(from_name), get_sector_index(to_name));
}

// ============================================================================
// Queries
// ============================================================================

// O(1) lookup through the matrix
int space_graph::get_weight(int from_index, int to_index) {
    if (!is_valid_index(from_index) || !is_valid_index(to_index)) {
        return NO_EDGE;
    }
    return adjacency_matrix[from_index][to_index];
}

// O(degree) lookup through the list; only used to cross-check the matrix
int space_graph::get_list_weight(int from_index, int to_index) {
    neighbor_node* entry = find_list_entry(from_index, to_index);
    if (entry == 0) {
        return NO_EDGE;
    }
    return entry->weight;
}

int space_graph::get_sector_index(string name) {
    int i = 0;
    while (i < sector_count) {
        if (sectors[i].name == name) {
            return i;
        }
        i = i + 1;
    }
    return -1;
}

string space_graph::get_sector_name(int index) {
    if (!is_valid_index(index)) {
        return "";
    }
    return sectors[index].name;
}

int space_graph::get_sector_count() {
    return sector_count;
}

int space_graph::get_corridor_count() {
    return corridor_count;
}

int space_graph::get_capacity() {
    return capacity;
}

bool space_graph::is_runway(int index) {
    if (!is_valid_index(index)) {
        return false;
    }
    return sectors[index].is_runway;
}

bool space_graph::is_valid_index(int index) {
    return index >= 0 && index < sector_count;
}

// ============================================================================
// Neighbor access
// ============================================================================

neighbor_node* space_graph::get_neighbors(int index) {
    if (!is_valid_index(index)) {
        return 0;
    }
    return sectors[index].neighbors;
}

int space_graph::get_neighbor_count(int sector_index) {
    int count = 0;
    neighbor_node* current = get_neighbors(sector_index);
    while (current != 0) {
        count = count + 1;
        current = current->next;
    }
    return count;
}

// Walk to the neighbor at position neighbor_position (0-based)
int space_graph::get_neighbor_index(int sector_index, int neighbor_position) {
    if (neighbor_position < 0) {
        return -1;
    }
    neighbor_node* current = get_neighbors(sector_index);
    int position = 0;
    while (current != 0) {
        if (position == neighbor_position) {
            return current->sector_index;
        }
        position = position + 1;
        current = current->next;
    }
    return -1;
}

int space_graph::get_neighbor_weight(int sector_index, int neighbor_position) {
    if (neighbor_position < 0) {
        return NO_EDGE;
    }
    neighbor_node* current = get_neighbors(sector_index);
    int position = 0;
    while (current != 0) {
        if (position == neighbor_position) {
            return current->weight;
        }
        position = position + 1;
        current = current->next;
    }
    return NO_EDGE;
}

// ============================================================================
// Verification / display
// ============================================================================

// Every matrix cell must equal the list weight in that direction, the matrix
// must be symmetric, and each list must hold exactly one node per non-zero cell.
bool space_graph::is_consistent() {
    int edge_entries = 0;
    int row = 0;
    while (row < sector_count) {
        int non_zero_in_row = 0;
        int col = 0;
        while (col < sector_count) {
            int matrix_weight = adjacency_matrix[row][col];
            if (matrix_weight != adjacency_matrix[col][row]) {
                return false;
            }
            if (matrix_weight != get_list_weight(row, col)) {
                return false;
            }
            if (matrix_weight != NO_EDGE) {
                non_zero_in_row = non_zero_in_row + 1;
            }
            col = col + 1;
        }
        if (non_zero_in_row != get_neighbor_count(row)) {
            return false;   // duplicate or stray list nodes
        }
        edge_entries = edge_entries + non_zero_in_row;
        row = row + 1;
    }
    // Each undirected corridor appears twice (once per direction)
    return edge_entries == corridor_count * 2;
}

void space_graph::display_adjacency_list() {
    cout << "=== Airspace Adjacency List ===" << endl;
    if (sector_count == 0) {
        cout << "(no sectors loaded)" << endl;
        return;
    }
    int i = 0;
    while (i < sector_count) {
        cout << "[" << i << "] " << sectors[i].name;
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
                 << "(" << current->weight << "s) ";
            current = current->next;
        }
        cout << endl;
        i = i + 1;
    }
}

// Rows/columns are labelled by sector index; a legend maps index -> name.
// A "." marks no corridor (stored as 0).
void space_graph::display_adjacency_matrix() {
    cout << "=== Airspace Adjacency Matrix (seconds, . = no corridor) ===" << endl;
    if (sector_count == 0) {
        cout << "(no sectors loaded)" << endl;
        return;
    }
    cout << setw(5) << " ";
    int col = 0;
    while (col < sector_count) {
        cout << setw(4) << col;
        col = col + 1;
    }
    cout << endl;

    int row = 0;
    while (row < sector_count) {
        cout << setw(4) << row << " ";
        col = 0;
        while (col < sector_count) {
            if (adjacency_matrix[row][col] == NO_EDGE) {
                cout << setw(4) << ".";
            } else {
                cout << setw(4) << adjacency_matrix[row][col];
            }
            col = col + 1;
        }
        cout << endl;
        row = row + 1;
    }

    cout << "Legend: ";
    int i = 0;
    while (i < sector_count) {
        cout << i << "=" << sectors[i].name;
        if (i + 1 < sector_count) {
            cout << ", ";
        }
        i = i + 1;
    }
    cout << endl;
}
