#ifndef BST_FLIGHT_H
#define BST_FLIGHT_H

#include "flight.h"
#include <iostream>
#include <string>
using namespace std;

// ============================================================================
// Module 4 (Person 2): BSTFlight - Binary Search Tree of active flights
//
// Ordered by callsign, so an inorder traversal prints an alphabetical manifest
// (the hash table gives fast lookups but no ordering).
//
// OWNERSHIP: the tree owns only its bst_node objects. The flight records belong
// to Person 1's registration list. Removing or destroying a node NEVER deletes
// the flight it points to.
//
// Each node keeps its OWN copy of the callsign as the key. game_engine removes a
// landed flight from the registry (which deletes it) before removing it from the
// trees, so tree operations must never need to read through flight_ref to
// compare keys.
//
// Height convention: number of NODES on the longest root-to-leaf path
// (empty tree = 0, single node = 1). AVLFlight uses the same convention.
// ============================================================================

struct bst_node {
    string callsign;      // search key (copied, so it stays valid on its own)
    flight* flight_ref;   // non-owning pointer to the registry's flight record
    bst_node* left;
    bst_node* right;
};

class bst_flight {
private:
    bst_node* root;
    int node_count;

    // Recursive helpers (each works on one subtree)
    bst_node* insert_node(bst_node* node, flight* new_flight, bool& inserted);
    bst_node* search_node(bst_node* node, string callsign);
    bst_node* find_min(bst_node* node);
    bst_node* remove_node(bst_node* node, string callsign, bool& removed);
    void inorder_node(bst_node* node);
    int inorder_collect(bst_node* node, string* output, int capacity, int filled);
    int height_node(bst_node* node);
    void destroy(bst_node* node);

    // Copying would make two trees share nodes (double delete), so it is disabled
    bst_flight(const bst_flight& other);
    bst_flight& operator=(const bst_flight& other);

public:
    bst_flight();
    ~bst_flight();

    // Insert a flight pointer keyed by its callsign.
    // Returns false for a null pointer or a callsign already in the tree
    // (duplicates are ignored; the original entry is kept).
    bool insert(flight* new_flight);

    // Return the flight with this callsign, or 0 (null) if not present
    flight* search(string callsign);

    // Remove the node for this callsign (leaf / one child / two children).
    // Returns false if the callsign is not in the tree. Never deletes the flight.
    bool remove(string callsign);

    // Print callsigns in alphabetical order
    void inorder_traversal();

    // Copy callsigns in alphabetical order into output[0..capacity-1];
    // returns how many were written (used by tests and demos)
    int inorder_callsigns(string* output, int capacity);

    // Height in nodes (0 for an empty tree), computed recursively
    int height();

    // Number of nodes currently stored
    int size();

    // Free every node (flights untouched) and leave an empty tree
    void destroy_tree();
};

// Name used in the project brief
typedef bst_flight BSTFlight;

#endif
