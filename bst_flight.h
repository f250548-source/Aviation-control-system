#ifndef BST_FLIGHT_H
#define BST_FLIGHT_H

#include "flight.h"
#include <iostream>
#include <string>
using namespace std;

// BST node keyed by callsign
struct bst_node {
    flight data;
    bst_node* left;
    bst_node* right;
};

// Person 2 stub: binary search tree of flights
class bst_flight {
private:
    bst_node* root;

    // Recursive insert
    bst_node* insert_node(bst_node* node, flight* f) {
        if (node == 0) {
            bst_node* created = new bst_node;
            created->data = *f;
            created->data.next = 0;
            created->left = 0;
            created->right = 0;
            return created;
        }
        if (f->callsign < node->data.callsign) {
            node->left = insert_node(node->left, f);
        } else if (f->callsign > node->data.callsign) {
            node->right = insert_node(node->right, f);
        }
        return node;
    }

    // Recursive search
    bst_node* search_node(bst_node* node, string callsign) {
        if (node == 0) {
            return 0;
        }
        if (callsign == node->data.callsign) {
            return node;
        }
        if (callsign < node->data.callsign) {
            return search_node(node->left, callsign);
        }
        return search_node(node->right, callsign);
    }

    // Find minimum node in subtree
    bst_node* find_min(bst_node* node) {
        while (node != 0 && node->left != 0) {
            node = node->left;
        }
        return node;
    }

    // Recursive remove
    bst_node* remove_node(bst_node* node, string callsign) {
        if (node == 0) {
            return 0;
        }
        if (callsign < node->data.callsign) {
            node->left = remove_node(node->left, callsign);
        } else if (callsign > node->data.callsign) {
            node->right = remove_node(node->right, callsign);
        } else {
            if (node->left == 0) {
                bst_node* right_child = node->right;
                delete node;
                return right_child;
            }
            if (node->right == 0) {
                bst_node* left_child = node->left;
                delete node;
                return left_child;
            }
            bst_node* successor = find_min(node->right);
            node->data = successor->data;
            node->right = remove_node(node->right, successor->data.callsign);
        }
        return node;
    }

    // In-order print
    void inorder_node(bst_node* node) {
        if (node == 0) {
            return;
        }
        inorder_node(node->left);
        cout << node->data.callsign << " ";
        inorder_node(node->right);
    }

    // Height helper
    int height_node(bst_node* node) {
        if (node == 0) {
            return 0;
        }
        int left_h = height_node(node->left);
        int right_h = height_node(node->right);
        if (left_h > right_h) {
            return left_h + 1;
        }
        return right_h + 1;
    }

    // Free tree
    void destroy(bst_node* node) {
        if (node == 0) {
            return;
        }
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

public:
    bst_flight() {
        root = 0;
    }

    ~bst_flight() {
        destroy(root);
        root = 0;
    }

    void insert(flight* f) {
        if (f == 0) {
            return;
        }
        root = insert_node(root, f);
    }

    flight* search(string callsign) {
        bst_node* node = search_node(root, callsign);
        if (node == 0) {
            return 0;
        }
        return &node->data;
    }

    void remove(string callsign) {
        root = remove_node(root, callsign);
    }

    void inorder_traversal() {
        cout << "BST inorder: ";
        inorder_node(root);
        cout << endl;
    }

    int height() {
        return height_node(root);
    }
};

#endif
