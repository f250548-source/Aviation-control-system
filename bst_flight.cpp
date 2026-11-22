#include "bst_flight.h"
using namespace std;

bst_flight::bst_flight() {
    root = 0;
    node_count = 0;
}

// Destructor frees tree nodes only; flight records belong to the registry
bst_flight::~bst_flight() {
    destroy_tree();
}

// ============================================================================
// Private recursive helpers
// ============================================================================

// Standard BST insert: go left if smaller, right if larger, create a node at
// the empty spot. An equal key means a duplicate, which is ignored.
bst_node* bst_flight::insert_node(bst_node* node, flight* new_flight, bool& inserted) {
    if (node == 0) {
        bst_node* created = new bst_node;
        created->callsign = new_flight->callsign;
        created->flight_ref = new_flight;
        created->left = 0;
        created->right = 0;
        inserted = true;
        return created;
    }
    if (new_flight->callsign < node->callsign) {
        node->left = insert_node(node->left, new_flight, inserted);
    } else if (new_flight->callsign > node->callsign) {
        node->right = insert_node(node->right, new_flight, inserted);
    }
    // equal callsign: duplicate, tree unchanged
    return node;
}

// Follow one path down from the root: O(h)
bst_node* bst_flight::search_node(bst_node* node, string callsign) {
    if (node == 0) {
        return 0;
    }
    if (callsign == node->callsign) {
        return node;
    }
    if (callsign < node->callsign) {
        return search_node(node->left, callsign);
    }
    return search_node(node->right, callsign);
}

// Leftmost node of a subtree = smallest callsign (inorder successor helper)
bst_node* bst_flight::find_min(bst_node* node) {
    while (node != 0 && node->left != 0) {
        node = node->left;
    }
    return node;
}

// Remove `callsign` from this subtree and return the new subtree root.
//  Case 1 - leaf:          delete it, parent gets 0
//  Case 2 - one child:     delete it, parent gets the child
//  Case 3 - two children:  copy the inorder successor's key + flight pointer
//                          into this node, then delete the successor from the
//                          right subtree (the successor has at most one child)
// Only bst_node objects are deleted; flight_ref is never deleted.
bst_node* bst_flight::remove_node(bst_node* node, string callsign, bool& removed) {
    if (node == 0) {
        return 0;   // not found in this subtree
    }
    if (callsign < node->callsign) {
        node->left = remove_node(node->left, callsign, removed);
        return node;
    }
    if (callsign > node->callsign) {
        node->right = remove_node(node->right, callsign, removed);
        return node;
    }

    // Found the node to remove
    if (node->left == 0 || node->right == 0) {
        // Cases 1 and 2: zero or one child
        bst_node* child = node->left;
        if (child == 0) {
            child = node->right;
        }
        delete node;
        removed = true;
        return child;
    }

    // Case 3: two children
    bst_node* successor = find_min(node->right);
    node->callsign = successor->callsign;
    node->flight_ref = successor->flight_ref;
    node->right = remove_node(node->right, successor->callsign, removed);
    return node;
}

// Left - node - right gives alphabetical order
void bst_flight::inorder_node(bst_node* node) {
    if (node == 0) {
        return;
    }
    inorder_node(node->left);
    cout << node->callsign << " ";
    inorder_node(node->right);
}

// Same traversal, but writes into an array; returns the new fill count
int bst_flight::inorder_collect(bst_node* node, string* output, int capacity, int filled) {
    if (node == 0) {
        return filled;
    }
    filled = inorder_collect(node->left, output, capacity, filled);
    if (filled < capacity) {
        output[filled] = node->callsign;
        filled = filled + 1;
    }
    return inorder_collect(node->right, output, capacity, filled);
}

// height(empty) = 0, height(node) = 1 + max(height(left), height(right))
int bst_flight::height_node(bst_node* node) {
    if (node == 0) {
        return 0;
    }
    int left_height = height_node(node->left);
    int right_height = height_node(node->right);
    if (left_height > right_height) {
        return left_height + 1;
    }
    return right_height + 1;
}

// Post-order delete: children first, then the node itself
void bst_flight::destroy(bst_node* node) {
    if (node == 0) {
        return;
    }
    destroy(node->left);
    destroy(node->right);
    node->flight_ref = 0;   // drop the reference only; the flight stays alive
    delete node;
}

// ============================================================================
// Public interface
// ============================================================================

bool bst_flight::insert(flight* new_flight) {
    if (new_flight == 0) {
        return false;
    }
    bool inserted = false;
    root = insert_node(root, new_flight, inserted);
    if (inserted) {
        node_count = node_count + 1;
    }
    return inserted;
}

flight* bst_flight::search(string callsign) {
    bst_node* node = search_node(root, callsign);
    if (node == 0) {
        return 0;
    }
    return node->flight_ref;
}

bool bst_flight::remove(string callsign) {
    bool removed = false;
    root = remove_node(root, callsign, removed);
    if (removed) {
        node_count = node_count - 1;
    }
    return removed;
}

void bst_flight::inorder_traversal() {
    cout << "BST inorder: ";
    if (root == 0) {
        cout << "(empty)";
    }
    inorder_node(root);
    cout << endl;
}

int bst_flight::inorder_callsigns(string* output, int capacity) {
    if (output == 0 || capacity <= 0) {
        return 0;
    }
    return inorder_collect(root, output, capacity, 0);
}

int bst_flight::height() {
    return height_node(root);
}

int bst_flight::size() {
    return node_count;
}

void bst_flight::destroy_tree() {
    destroy(root);
    root = 0;
    node_count = 0;
}
