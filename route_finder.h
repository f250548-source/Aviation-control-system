#ifndef ROUTE_FINDER_H
#define ROUTE_FINDER_H

#include "flight.h"
#include "space_graph.h"
#include <iostream>
#include <string>
using namespace std;

// Person 2 stub: Dijkstra shortest-path routing
class route_finder {
private:
    space_graph* graph_ref;

    // Manual infinity sentinel for distances
    int infinity_value() {
        return 1000000000;
    }

public:
    route_finder(space_graph* graph) {
        graph_ref = graph;
    }

    ~route_finder() {
        graph_ref = 0;
    }

    // Dijkstra from start_name to end_name; caller must delete[] route.sectors
    route find_fastest_route(string start_name, string end_name) {
        route result;
        if (graph_ref == 0) {
            cout << "[ATC Error] No airspace graph." << endl;
            return result;
        }
        int start = graph_ref->get_sector_index(start_name);
        int end = graph_ref->get_sector_index(end_name);
        if (start < 0 || end < 0) {
            cout << "[ATC Error] Route endpoints not found." << endl;
            return result;
        }

        int n = graph_ref->get_sector_count();
        int* distance = new int[n];
        int* parent = new int[n];
        bool* settled = new bool[n];
        int i = 0;
        while (i < n) {
            distance[i] = infinity_value();
            parent[i] = -1;
            settled[i] = false;
            i = i + 1;
        }
        distance[start] = 0;

        int count = 0;
        while (count < n) {
            int u = -1;
            int best = infinity_value();
            int j = 0;
            while (j < n) {
                if (!settled[j] && distance[j] < best) {
                    best = distance[j];
                    u = j;
                }
                j = j + 1;
            }
            if (u < 0) {
                break;
            }
            settled[u] = true;
            neighbor_node* neighbor = graph_ref->get_neighbors(u);
            while (neighbor != 0) {
                int v = neighbor->sector_index;
                int weight = neighbor->weight;
                if (!settled[v] && distance[u] + weight < distance[v]) {
                    distance[v] = distance[u] + weight;
                    parent[v] = u;
                }
                neighbor = neighbor->next;
            }
            count = count + 1;
        }

        if (distance[end] >= infinity_value()) {
            cout << "[ATC Error] No route from " << start_name
                 << " to " << end_name << endl;
            delete[] distance;
            delete[] parent;
            delete[] settled;
            return result;
        }

        // Rebuild path into temporary reverse buffer
        int* reverse_path = new int[n];
        int path_len = 0;
        int walk = end;
        while (walk != -1) {
            reverse_path[path_len] = walk;
            path_len = path_len + 1;
            walk = parent[walk];
        }

        result.sectors = new string[path_len];
        result.sector_count = path_len;
        result.total_weight = distance[end];
        i = 0;
        while (i < path_len) {
            result.sectors[i] = graph_ref->get_sector_name(reverse_path[path_len - 1 - i]);
            i = i + 1;
        }

        delete[] reverse_path;
        delete[] distance;
        delete[] parent;
        delete[] settled;
        return result;
    }

    // Print a route object
    void print_route(route& r) {
        if (r.sector_count == 0 || r.sectors == 0) {
            cout << "Empty route." << endl;
            return;
        }
        cout << "Route (" << r.total_weight << "): ";
        int i = 0;
        while (i < r.sector_count) {
            cout << r.sectors[i];
            if (i + 1 < r.sector_count) {
                cout << " -> ";
            }
            i = i + 1;
        }
        cout << endl;
    }

    // Next hop after current_sector along path to target runway
    string next_hop(string current_sector, string target_runway) {
        route r = find_fastest_route(current_sector, target_runway);
        string hop = "";
        if (r.sector_count >= 2 && r.sectors != 0) {
            hop = r.sectors[1];
        } else if (r.sector_count == 1 && r.sectors != 0) {
            hop = r.sectors[0];
        }
        if (r.sectors != 0) {
            delete[] r.sectors;
            r.sectors = 0;
        }
        return hop;
    }
};

#endif
