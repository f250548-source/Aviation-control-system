#ifndef AVL_FLIGHT_H
#define AVL_FLIGHT_H

#include "flight.h"
#include <iostream>
#include <string>
using namespace std;

// AVL node keyed by callsign
struct avl_node {
    flight data;
    int height;
    avl_node* left;
    avl_node* right;
};

// Person 2 stub: AVL tree of flights
class avl_flight {
private:
    avl_node* root;

    int node_height(avl_node* node) {
        if (node == 0) {
            return 0;
        }
        return node->height;
    }

    int max_of(int a, int b) {
        if (a > b) {
            return a;
        }
        return b;
    }

    void update_height(avl_node* node) {
        if (node == 0) {
            return;
        }
        node->height = 1 + max_of(node_height(node->left), node_height(node->right));
    }

    int balance_factor(avl_node* node) {
        if (node == 0) {
            return 0;
        }
        return node_height(node->left) - node_height(node->right);
    }

    avl_node* rotate_right(avl_node* y) {
        avl_node* x = y->left;
        avl_node* t2 = x->right;
        x->right = y;
        y->left = t2;
        update_height(y);
        update_height(x);
        return x;
    }

    avl_node* rotate_left(avl_node* x) {
        avl_node* y = x->right;
        avl_node* t2 = y->left;
        y->left = x;
        x->right = t2;
        update_height(x);
        update_height(y);
        return y;
    }

    avl_node* rebalance(avl_node* node) {
        update_height(node);
        int balance = balance_factor(node);
        if (balance > 1 && balance_factor(node->left) >= 0) {
            return rotate_right(node);
        }
        if (balance > 1 && balance_factor(node->left) < 0) {
            node->left = rotate_left(node->left);
            return rotate_right(node);
        }
        if (balance < -1 && balance_factor(node->right) <= 0) {
            return rotate_left(node);
        }
        if (balance < -1 && balance_factor(node->right) > 0) {
            node->right = rotate_right(node->right);
            return rotate_left(node);
        }
        return node;
    }

    avl_node* insert_node(avl_node* node, flight* f) {
        if (node == 0) {
            avl_node* created = new avl_node;
            created->data = *f;
            created->data.next = 0;
            created->left = 0;
            created->right = 0;
            created->height = 1;
            return created;
        }
        if (f->callsign < node->data.callsign) {
            node->left = insert_node(node->left, f);
        } else if (f->callsign > node->data.callsign) {
            node->right = insert_node(node->right, f);
        } else {
            return node;
        }
        return rebalance(node);
    }

    avl_node* find_min(avl_node* node) {
        while (node != 0 && node->left != 0) {
            node = node->left;
        }
        return node;
    }

    avl_node* remove_node(avl_node* node, string callsign) {
        if (node == 0) {
            return 0;
        }
        if (callsign < node->data.callsign) {
            node->left = remove_node(node->left, callsign);
        } else if (callsign > node->data.callsign) {
            node->right = remove_node(node->right, callsign);
        } else {
            if (node->left == 0 || node->right == 0) {
                avl_node* temp = node->left;
                if (temp == 0) {
                    temp = node->right;
                }
                delete node;
                return temp;
            }
            avl_node* successor = find_min(node->right);
            node->data = successor->data;
            node->right = remove_node(node->right, successor->data.callsign);
        }
        if (node == 0) {
            return 0;
        }
        return rebalance(node);
    }

    avl_node* search_node(avl_node* node, string callsign) {
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

    void inorder_node(avl_node* node) {
        if (node == 0) {
            return;
        }
        inorder_node(node->left);
        cout << node->data.callsign << " ";
        inorder_node(node->right);
    }

    void destroy(avl_node* node) {
        if (node == 0) {
            return;
        }
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

public:
    avl_flight() {
        root = 0;
    }

    ~avl_flight() {
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
        avl_node* node = search_node(root, callsign);
        if (node == 0) {
            return 0;
        }
        return &node->data;
    }

    void remove(string callsign) {
        root = remove_node(root, callsign);
    }

    void inorder_traversal() {
        cout << "AVL inorder: ";
        inorder_node(root);
        cout << endl;
    }

    int height() {
        return node_height(root);
    }
};

#endif
