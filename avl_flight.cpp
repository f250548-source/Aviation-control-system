#include "avl_flight.h"
#include "bst_flight.h"
using namespace std;

avl_flight::avl_flight() {
    root = 0;
    node_count = 0;
    left_left_count = 0;
    right_right_count = 0;
    left_right_count = 0;
    right_left_count = 0;
}

// Frees tree nodes only; flight records belong to the registry
avl_flight::~avl_flight() {
    destroy_tree();
}

// ============================================================================
// Height and balance helpers
// ============================================================================

// Stored height; an empty subtree has height 0
int avl_flight::get_height(avl_node* node) {
    if (node == 0) {
        return 0;
    }
    return node->height;
}

// > 0 means left-heavy, < 0 means right-heavy
int avl_flight::balance_factor(avl_node* node) {
    if (node == 0) {
        return 0;
    }
    return get_height(node->left) - get_height(node->right);
}

// Recompute a node's height from its children (children must be up to date)
void avl_flight::update_height(avl_node* node) {
    if (node == 0) {
        return;
    }
    int left_height = get_height(node->left);
    int right_height = get_height(node->right);
    if (left_height > right_height) {
        node->height = left_height + 1;
    } else {
        node->height = right_height + 1;
    }
}

// ============================================================================
// Rotations
// ============================================================================

// Left rotation (fixes a right-heavy node):
//   before: node.right = pivot, pivot.left = middle
//   after:  pivot.left = node,  node.right = middle
// pivot moves up to become the subtree root; middle changes parent.
avl_node* avl_flight::rotate_left(avl_node* node) {
    avl_node* pivot = node->right;
    avl_node* middle = pivot->left;
    pivot->left = node;
    node->right = middle;
    update_height(node);    // node is now lower, so update it first
    update_height(pivot);
    return pivot;
}

// Right rotation (fixes a left-heavy node), mirror image of rotate_left:
//   before: node.left = pivot,  pivot.right = middle
//   after:  pivot.right = node, node.left = middle
avl_node* avl_flight::rotate_right(avl_node* node) {
    avl_node* pivot = node->left;
    avl_node* middle = pivot->right;
    pivot->right = node;
    node->left = middle;
    update_height(node);
    update_height(pivot);
    return pivot;
}

// Left-Right: left child is right-heavy. Rotate the child left to turn it into
// a Left-Left shape, then rotate this node right.
avl_node* avl_flight::rotate_left_right(avl_node* node) {
    node->left = rotate_left(node->left);
    return rotate_right(node);
}

// Right-Left: right child is left-heavy. Rotate the child right, then this node left.
avl_node* avl_flight::rotate_right_left(avl_node* node) {
    node->right = rotate_right(node->right);
    return rotate_left(node);
}

// Called on every node along the path after an insert or remove.
// Choosing the case from the CHILD's balance factor (>= 0 / <= 0) also
// handles deletion, where the child can be perfectly balanced.
avl_node* avl_flight::rebalance(avl_node* node) {
    update_height(node);
    int balance = balance_factor(node);

    if (balance > 1) {
        if (balance_factor(node->left) >= 0) {
            left_left_count = left_left_count + 1;
            return rotate_right(node);
        }
        left_right_count = left_right_count + 1;
        return rotate_left_right(node);
    }
    if (balance < -1) {
        if (balance_factor(node->right) <= 0) {
            right_right_count = right_right_count + 1;
            return rotate_left(node);
        }
        right_left_count = right_left_count + 1;
        return rotate_right_left(node);
    }
    return node;   // already balanced
}

// ============================================================================
// Recursive helpers
// ============================================================================

// Normal BST insert, then rebalance each ancestor on the way back up
avl_node* avl_flight::insert_node(avl_node* node, flight* new_flight, bool& inserted) {
    if (node == 0) {
        avl_node* created = new avl_node;
        created->callsign = new_flight->callsign;
        created->flight_ref = new_flight;
        created->height = 1;
        created->left = 0;
        created->right = 0;
        inserted = true;
        return created;
    }
    if (new_flight->callsign < node->callsign) {
        node->left = insert_node(node->left, new_flight, inserted);
    } else if (new_flight->callsign > node->callsign) {
        node->right = insert_node(node->right, new_flight, inserted);
    } else {
        return node;   // duplicate callsign: nothing changes
    }
    return rebalance(node);
}

avl_node* avl_flight::find_min(avl_node* node) {
    while (node != 0 && node->left != 0) {
        node = node->left;
    }
    return node;
}

// Same three deletion cases as the BST, followed by rebalancing on the way up
avl_node* avl_flight::remove_node(avl_node* node, string callsign, bool& removed) {
    if (node == 0) {
        return 0;
    }
    if (callsign < node->callsign) {
        node->left = remove_node(node->left, callsign, removed);
    } else if (callsign > node->callsign) {
        node->right = remove_node(node->right, callsign, removed);
    } else {
        if (node->left == 0 || node->right == 0) {
            // Leaf or one child: replace node by its only child (or 0)
            avl_node* child = node->left;
            if (child == 0) {
                child = node->right;
            }
            delete node;            // the flight itself is not touched
            removed = true;
            return child;           // child subtree is already balanced
        }
        // Two children: take over the inorder successor's key and flight pointer
        avl_node* successor = find_min(node->right);
        node->callsign = successor->callsign;
        node->flight_ref = successor->flight_ref;
        node->right = remove_node(node->right, successor->callsign, removed);
    }
    return rebalance(node);
}

avl_node* avl_flight::search_node(avl_node* node, string callsign) {
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

void avl_flight::inorder_node(avl_node* node) {
    if (node == 0) {
        return;
    }
    inorder_node(node->left);
    cout << node->callsign << " ";
    inorder_node(node->right);
}

int avl_flight::inorder_collect(avl_node* node, string* output, int capacity, int filled) {
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

// Validates the subtree and returns its real height. Sets ok = false if a key
// is outside its allowed (lower, upper) range, a stored height is wrong, or a
// balance factor is outside [-1, 1].
int avl_flight::check_node(avl_node* node, const string* lower, const string* upper, bool& ok) {
    if (node == 0) {
        return 0;
    }
    if (lower != 0 && !(node->callsign > *lower)) {
        ok = false;
    }
    if (upper != 0 && !(node->callsign < *upper)) {
        ok = false;
    }
    int left_height = check_node(node->left, lower, &node->callsign, ok);
    int right_height = check_node(node->right, &node->callsign, upper, ok);
    int real_height = left_height + 1;
    if (right_height > left_height) {
        real_height = right_height + 1;
    }
    if (node->height != real_height) {
        ok = false;
    }
    int balance = left_height - right_height;
    if (balance > 1 || balance < -1) {
        ok = false;
    }
    return real_height;
}

void avl_flight::destroy(avl_node* node) {
    if (node == 0) {
        return;
    }
    destroy(node->left);
    destroy(node->right);
    node->flight_ref = 0;   // reference only; the flight stays alive
    delete node;
}

// ============================================================================
// Public interface
// ============================================================================

bool avl_flight::insert(flight* new_flight) {
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

flight* avl_flight::search(string callsign) {
    avl_node* node = search_node(root, callsign);
    if (node == 0) {
        return 0;
    }
    return node->flight_ref;
}

bool avl_flight::remove(string callsign) {
    bool removed = false;
    root = remove_node(root, callsign, removed);
    if (removed) {
        node_count = node_count - 1;
    }
    return removed;
}

void avl_flight::inorder_traversal() {
    cout << "AVL inorder: ";
    if (root == 0) {
        cout << "(empty)";
    }
    inorder_node(root);
    cout << endl;
}

int avl_flight::inorder_callsigns(string* output, int capacity) {
    if (output == 0 || capacity <= 0) {
        return 0;
    }
    return inorder_collect(root, output, capacity, 0);
}

int avl_flight::height() {
    return get_height(root);
}

int avl_flight::size() {
    return node_count;
}

string avl_flight::root_callsign() {
    if (root == 0) {
        return "";
    }
    return root->callsign;
}

bool avl_flight::is_valid_avl() {
    bool ok = true;
    check_node(root, 0, 0, ok);
    return ok;
}

int avl_flight::get_rotation_count(string rotation_case) {
    if (rotation_case == "LL") {
        return left_left_count;
    }
    if (rotation_case == "RR") {
        return right_right_count;
    }
    if (rotation_case == "LR") {
        return left_right_count;
    }
    if (rotation_case == "RL") {
        return right_left_count;
    }
    return 0;
}

void avl_flight::destroy_tree() {
    destroy(root);
    root = 0;
    node_count = 0;
}

// ============================================================================
// Module 5 demonstration: BST vs AVL on sorted input
// ============================================================================

void compare_bst_avl_heights_demo() {
    const int DEMO_COUNT = 15;
    // Already in alphabetical order: the worst case for a plain BST
    string demo_callsigns[DEMO_COUNT] = {
        "AA101", "BA202", "CX303", "DL404", "EK505",
        "FZ606", "GF707", "HU808", "IB909", "JL110",
        "KL211", "LH312", "MH413", "NH514", "OS615"
    };

    // The flights are declared BEFORE the trees, so they are destroyed AFTER
    // them: the trees' non-owning pointers never outlive the records.
    flight demo_flights[DEMO_COUNT];
    int i = 0;
    while (i < DEMO_COUNT) {
        demo_flights[i].callsign = demo_callsigns[i];
        demo_flights[i].aircraft_type = "A320";
        demo_flights[i].airline = "DemoAir";
        demo_flights[i].origin = "Demo";
        demo_flights[i].destination = "RWY09";
        demo_flights[i].passenger_count = 0;
        demo_flights[i].fuel_remaining = 0;
        demo_flights[i].current_sector = "Control_Tower";
        demo_flights[i].assigned_runway = "RWY09";
        demo_flights[i].arrival_tick = 0;
        demo_flights[i].priority_level = NORMAL;
        demo_flights[i].status = "DEMO";
        demo_flights[i].next = 0;
        i = i + 1;
    }

    {
        bst_flight demo_bst;
        avl_flight demo_avl;

        cout << "=== BST vs AVL: 15 callsigns inserted in alphabetical order ===" << endl;
        cout << "#  Callsign  BST height  AVL height" << endl;
        i = 0;
        while (i < DEMO_COUNT) {
            demo_bst.insert(&demo_flights[i]);
            demo_avl.insert(&demo_flights[i]);
            cout << (i + 1) << "  " << demo_callsigns[i]
                 << "  " << demo_bst.height()
                 << "  " << demo_avl.height() << endl;
            i = i + 1;
        }

        cout << "Final BST height: " << demo_bst.height()
             << "  |  Final AVL height: " << demo_avl.height() << endl;
        cout << "Sorted input turns the BST into a linked list (height = n), while"
             << " AVL rotations keep it near log2(n)." << endl;
        cout << "AVL rotations used: LL=" << demo_avl.get_rotation_count("LL")
             << " RR=" << demo_avl.get_rotation_count("RR")
             << " LR=" << demo_avl.get_rotation_count("LR")
             << " RL=" << demo_avl.get_rotation_count("RL") << endl;
        demo_bst.inorder_traversal();
        demo_avl.inorder_traversal();
    }   // both trees destroyed here; demo_flights are still alive
}
