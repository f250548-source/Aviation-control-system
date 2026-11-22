#ifndef AVL_FLIGHT_H
#define AVL_FLIGHT_H

#include "flight.h"
#include <iostream>
#include <string>
using namespace std;

// ============================================================================
// Module 5 (Person 2): AVLFlight - self-balancing AVL tree of active flights
//
// Same ordering (callsign), same public interface and same non-owning
// flight pointers as bst_flight. After every insert/remove each node on the
// path back to the root is rebalanced, so |balance factor| <= 1 everywhere and
// the height stays O(log n) even when callsigns arrive in alphabetical order.
//
// OWNERSHIP: only avl_node objects are owned. Flights belong to the registry
// and are never deleted by this tree.
//
// Height convention: nodes on the longest path (empty = 0, leaf = 1).
// Balance factor = height(left) - height(right).
// ============================================================================

struct avl_node {
    string callsign;      // search key (own copy)
    flight* flight_ref;   // non-owning pointer to the registry's flight
    int height;           // height of the subtree rooted here (leaf = 1)
    avl_node* left;
    avl_node* right;
};

class avl_flight {
private:
    avl_node* root;
    int node_count;

    // How many times each rebalancing case has fired (for demos/tests)
    int left_left_count;
    int right_right_count;
    int left_right_count;
    int right_left_count;

    // ---- Height / balance helpers ----
    int get_height(avl_node* node);
    int balance_factor(avl_node* node);
    void update_height(avl_node* node);

    // ---- Rotations ----
    avl_node* rotate_left(avl_node* node);         // fixes Right-Right
    avl_node* rotate_right(avl_node* node);        // fixes Left-Left
    avl_node* rotate_left_right(avl_node* node);   // fixes Left-Right
    avl_node* rotate_right_left(avl_node* node);   // fixes Right-Left

    // Pick and apply the correct rotation case for this node
    avl_node* rebalance(avl_node* node);

    // ---- Recursive helpers ----
    avl_node* insert_node(avl_node* node, flight* new_flight, bool& inserted);
    avl_node* remove_node(avl_node* node, string callsign, bool& removed);
    avl_node* search_node(avl_node* node, string callsign);
    avl_node* find_min(avl_node* node);
    void inorder_node(avl_node* node);
    int inorder_collect(avl_node* node, string* output, int capacity, int filled);
    int check_node(avl_node* node, const string* lower, const string* upper, bool& ok);
    void destroy(avl_node* node);

    // Copying is disabled (would share nodes and cause double deletes)
    avl_flight(const avl_flight& other);
    avl_flight& operator=(const avl_flight& other);

public:
    avl_flight();
    ~avl_flight();

    // Same contract as bst_flight::insert (duplicates ignored, returns false)
    bool insert(flight* new_flight);

    // Flight with this callsign or 0 (null)
    flight* search(string callsign);

    // AVL deletion with rebalancing; never deletes the flight record
    bool remove(string callsign);

    // Print callsigns alphabetically
    void inorder_traversal();

    // Copy callsigns alphabetically into output; returns count written
    int inorder_callsigns(string* output, int capacity);

    // Height in nodes (0 for empty)
    int height();

    int size();

    // Callsign stored at the root ("" if empty) - lets tests see rotations
    string root_callsign();

    // True if ordering, stored heights and every balance factor are valid
    bool is_valid_avl();

    // Number of rotations performed for "LL", "RR", "LR" or "RL"
    int get_rotation_count(string rotation_case);

    // Free every node (flights untouched) and leave an empty tree
    void destroy_tree();
};

// Name used in the project brief
typedef avl_flight AVLFlight;

// Module 5 demonstration: insert 15 alphabetically ordered callsigns into a
// BST and an AVL tree and print both heights side by side. Self-contained
// (uses its own temporary flights), so game_engine option 22 can call it.
void compare_bst_avl_heights_demo();

#endif
