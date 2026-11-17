#ifndef RADAR_SWEEP_H
#define RADAR_SWEEP_H

#include "space_graph.h"
#include <iostream>
#include <string>
using namespace std;

// Person 2 stub: BFS/DFS reachability over the airspace graph
class radar_sweep {
private:
    space_graph* graph_ref;

    // Hand-written BFS using a dynamic circular buffer as a queue
    void run_bfs(int start_index, bool* visited) {
        int n = graph_ref->get_sector_count();
        int* bfs_queue = new int[n];
        int front = 0;
        int rear = 0;
        int i = 0;
        while (i < n) {
            visited[i] = false;
            i = i + 1;
        }
        if (start_index < 0 || start_index >= n) {
            delete[] bfs_queue;
            return;
        }
        visited[start_index] = true;
        bfs_queue[rear] = start_index;
        rear = rear + 1;
        while (front < rear) {
            int current = bfs_queue[front];
            front = front + 1;
            neighbor_node* neighbor = graph_ref->get_neighbors(current);
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
        delete[] bfs_queue;
    }

    // Recursive DFS helper
    void run_dfs(int current, bool* visited) {
        visited[current] = true;
        neighbor_node* neighbor = graph_ref->get_neighbors(current);
        while (neighbor != 0) {
            if (!visited[neighbor->sector_index]) {
                run_dfs(neighbor->sector_index, visited);
            }
            neighbor = neighbor->next;
        }
    }

public:
    radar_sweep(space_graph* graph) {
        graph_ref = graph;
    }

    ~radar_sweep() {
        graph_ref = 0;
    }

    // BFS listing from a named sector
    void bfs_from(string start_name) {
        if (graph_ref == 0) {
            cout << "[ATC Error] No airspace graph." << endl;
            return;
        }
        int start = graph_ref->get_sector_index(start_name);
        if (start < 0) {
            cout << "[ATC Error] Sector not found: " << start_name << endl;
            return;
        }
        int n = graph_ref->get_sector_count();
        bool* visited = new bool[n];
        run_bfs(start, visited);
        cout << "BFS from " << start_name << ": ";
        int i = 0;
        while (i < n) {
            if (visited[i]) {
                cout << graph_ref->get_sector_name(i) << " ";
            }
            i = i + 1;
        }
        cout << endl;
        delete[] visited;
    }

    // DFS listing from a named sector
    void dfs_from(string start_name) {
        if (graph_ref == 0) {
            cout << "[ATC Error] No airspace graph." << endl;
            return;
        }
        int start = graph_ref->get_sector_index(start_name);
        if (start < 0) {
            cout << "[ATC Error] Sector not found: " << start_name << endl;
            return;
        }
        int n = graph_ref->get_sector_count();
        bool* visited = new bool[n];
        int i = 0;
        while (i < n) {
            visited[i] = false;
            i = i + 1;
        }
        run_dfs(start, visited);
        cout << "DFS from " << start_name << ": ";
        i = 0;
        while (i < n) {
            if (visited[i]) {
                cout << graph_ref->get_sector_name(i) << " ";
            }
            i = i + 1;
        }
        cout << endl;
        delete[] visited;
    }

    // True if target is reachable from start
    bool is_reachable(string start_name, string target_name) {
        if (graph_ref == 0) {
            return false;
        }
        int start = graph_ref->get_sector_index(start_name);
        int target = graph_ref->get_sector_index(target_name);
        if (start < 0 || target < 0) {
            return false;
        }
        int n = graph_ref->get_sector_count();
        bool* visited = new bool[n];
        run_bfs(start, visited);
        bool result = visited[target];
        delete[] visited;
        return result;
    }

    // Search for a missing flight last known at a sector (BFS from tower)
    bool search_missing_flight(string tower_name, string last_known_sector) {
        cout << "Radar searching for missing flight near "
             << last_known_sector << "..." << endl;
        bool found = is_reachable(tower_name, last_known_sector);
        if (found) {
            cout << "Roger. Contact established with sector "
                 << last_known_sector << "." << endl;
        } else {
            cout << "[ATC Error] Sector unreachable from " << tower_name << endl;
        }
        return found;
    }
};

#endif
