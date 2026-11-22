# MayDay-404 — Person 2 Design (Modules 4–8)

## 1. Project context

MayDay-404 is a console Air Traffic Control simulation built for the Data Structures course. The player keeps flights moving through a network of airspace sectors, handles emergencies and lands flights on runways. Every data structure is written by hand with no STL containers.

Person 2 provides the **ordered flight indexes** (BST, AVL) and the **airspace network with its graph algorithms** (graph storage, BFS/DFS, Dijkstra). Person 3's `game_engine` and `radar_mst` are built on these modules.

## 2. Module responsibilities

| Module | Class (brief name) | Files | Responsibility |
|---|---|---|---|
| M4 | `bst_flight` (`BSTFlight`) | `bst_flight.h/.cpp` | Binary search tree of active flights keyed by callsign. Gives an alphabetical manifest. |
| M5 | `avl_flight` (`AVLFlight`) | `avl_flight.h/.cpp` | Self-balancing AVL tree with the same interface. Also holds the 15-callsign BST vs AVL height demo. |
| M6 | `space_graph` (`SpaceGraph`) | `space_graph.h/.cpp` | Airspace sectors and corridors, stored as both an adjacency list and an adjacency matrix. |
| M7 | `radar_sweep` (`RadarSweep`) | `radar_sweep.h/.cpp` | BFS (full and 2-hop), DFS, reachability, missing-flight search. |
| M8 | `route_finder` (`RouteFinder`) | `route_finder.h/.cpp` | Dijkstra fastest route with a custom min-heap, route rebuilding, next hop. |

The files and classes use the repository's existing snake_case names, because Person 3's code already includes and uses them. Each header adds a `typedef` with the brief's name, e.g. `typedef bst_flight BSTFlight;`.

## 3. Architecture and relationships

```
                 Person 1                         Person 3
          +---------------------+        +--------------------------+
          | flight.h            |        | game_engine              |
          |  flight, route      |        |  owns: registration,     |
          | registration (owns  |        |  hash_table, bst_flight, |
          |  flight records)    |        |  avl_flight, space_graph |
          | file_loader --------+--+     |  new: radar_sweep,       |
          +----------+----------+  |     |  route_finder, radar_mst |
                     |             |     +---+-----+-----+-----+----+
        flight*      |             |         |     |     |     |
     (non-owning)    v             |         |     |     |     |
          +------------------+     |         |     |     |     |
          | bst_flight  (M4) |<----+---------+     |     |     |
          | avl_flight  (M5) |<----+---------------+     |     |
          +------------------+     | add_sector/         |     |
                                   | add_corridor        |     |
                                   v                     v     v
                          +------------------+   +-------------------+
                          | space_graph (M6) |<--| radar_sweep  (M7) |
                          |  adjacency list  |<--| route_finder (M8) |
                          |  adjacency matrix|<--| radar_mst (P3, M9)|
                          +------------------+   +-------------------+
                                 graph* (non-owning, never copied)
```

- `game_engine` owns one `space_graph`. `radar_sweep`, `route_finder` and `radar_mst` each get a pointer to it and share that one graph. When the cyber event calls `remove_corridor`, all three algorithms see the change on their next call.
- The trees store `flight*` pointers into Person 1's registry list. They never copy or delete flights.
- `route_finder` returns Person 1's `route` struct from `flight.h`.

## 4. Class design

### bst_flight (M4)
- **Data:** `bst_node* root`, `int node_count`.
- **Public:** `insert(flight*)`, `search(callsign)`, `remove(callsign)`, `inorder_traversal()`, `inorder_callsigns(out, cap)`, `height()`, `size()`, `destroy_tree()`.
- **Private (recursive):** `insert_node`, `search_node`, `find_min`, `remove_node`, `inorder_node`, `inorder_collect`, `height_node`, `destroy`.
- **Memory:** owns its `bst_node`s and frees them in `destroy_tree()` and the destructor. It never deletes a `flight`. Copying is disabled.

### avl_flight (M5)
- **Data:** `avl_node* root`, `node_count`, and four counters for the rotation cases.
- **Public:** the same as the BST, plus `root_callsign()`, `is_valid_avl()` and `get_rotation_count("LL"|"RR"|"LR"|"RL")`.
- **Private:** `get_height`, `balance_factor`, `update_height`, `rotate_left`, `rotate_right`, `rotate_left_right`, `rotate_right_left`, `rebalance`, the recursive insert/remove/search helpers, and `check_node` (validation).
- **Free function:** `compare_bst_avl_heights_demo()`.
- **Memory:** the same rules as the BST.

### space_graph (M6)
- **Data:** `sector_node* sectors` (dynamic array), `int** adjacency_matrix`, `sector_count`, `capacity`, `corridor_count`.
- **Public:**
  - Sectors: `add_sector(name)`, `add_sector(name, is_runway)`.
  - Corridors, by index or by name: `add_corridor(...)`, `remove_corridor(...)`.
  - Lookups: `get_weight`, `get_list_weight`, `get_sector_index`, `get_sector_name`, `get_sector_count`, `get_corridor_count`, `get_capacity`, `is_runway`, `is_valid_index`.
  - Neighbors: `get_neighbors`, `get_neighbor_count`, `get_neighbor_index`, `get_neighbor_weight`.
  - Checks and output: `is_consistent`, `display_adjacency_list`, `display_adjacency_matrix`.
- **Private:** `grow_capacity`, `allocate_matrix`, `free_matrix`, `find_list_entry`, `push_neighbor`, `remove_neighbor`, `name_looks_like_runway`.
- **Memory:** owns every `neighbor_node`, every matrix row, the row-pointer array and the sector array. All are freed in the destructor. Copying is disabled.

### radar_sweep (M7)
- **Data:** `space_graph* graph_ref` (non-owning).
- **Public (each in an index form and a name form):** `bfs_from`, `bfs_within_2_hops`, `dfs_from`, `is_reachable`, `search_missing_flight`, plus `hop_distance`.
- **Private:** `run_bfs` (core BFS with optional hop limit), `run_dfs` (recursive), `missing_search_core`, `print_levels`.
- **Helper class:** `sector_queue`, a circular array queue.
- **Memory:** every visited, order and level array and every queue is allocated inside a call and freed before the call returns.

### route_finder (M8)
- **Data:** `space_graph* graph_ref` (non-owning).
- **Public:**
  - `find_shortest_path(src, dst)`: any destination.
  - `find_fastest_route(src, runway)`: destination must be a runway. A name form also exists for the engine.
  - `find_route_to_nearest(src, runways[], k)`.
  - `print_route(r)`.
  - `next_hop(route, int)`: returns an index.
  - `next_hop(string, string)`: returns a sector name (engine form).
  - `free_route(r)`.
- **Private:** `run_dijkstra`, `build_route`, `compute_route`.
- **Helper class:** `distance_min_heap`, an array-based binary heap of `heap_entry {sector_index, distance}`.
- **Memory:** the Dijkstra arrays and the heap are temporary. A returned `route` owns its `sectors` array, and **the caller frees it** with `delete[] r.sectors` or `free_route(r)`.

## 5. Data structure design

| Structure | Layout |
|---|---|
| BST node | `{ string callsign; flight* flight_ref; bst_node* left, *right; }`. The callsign is a **copy** used as the key. `flight_ref` is non-owning. |
| AVL node | The BST node fields plus `int height` (a leaf has height 1). |
| Adjacency list | `sectors[i].neighbors` → singly linked `neighbor_node {sector_index, weight, next}`. Each undirected corridor is two nodes, one per direction. New nodes go at the head. |
| Adjacency matrix | `int** adjacency_matrix`, `capacity × capacity`. `matrix[i][j]` is the weight in seconds, and 0 means no corridor. |
| BFS queue | `sector_queue`: an `int*` circular buffer of size V with front index and count. Each sector is enqueued at most once. |
| DFS stack | Recursion, so the call stack serves as the DFS stack. |
| Dijkstra heap | `distance_min_heap`: a `heap_entry*` array that doubles when full. Parent of position p is `(p-1)/2`, children are `2p+1` and `2p+2`. |
| Distance / parent arrays | `int distance[V]` starts at 10⁹ (infinity). `int parent[V]` starts at -1. `bool settled[V]`. |

## 6. Algorithm explanations

- **BST insert and search:** compare callsigns and go left if smaller, right if larger. Insert creates a node at the first empty position. An equal key is a duplicate and is ignored.
- **BST delete:**
  - Leaf: delete it and return null to the parent.
  - One child: delete it and return the child to the parent.
  - Two children: copy the in-order successor's callsign and `flight*` into the node, then delete the successor from the right subtree.
  - Removing the root works the same way, because each recursive call returns the new subtree root.
- **AVL rotations:**
  - `rotate_left` (Right-Right case): the right child rises and the node becomes its left child.
  - `rotate_right` (Left-Left case): the mirror image.
  - Left-Right case: `rotate_left(node.left)`, then `rotate_right(node)`.
  - Right-Left case: the mirror image.
  - Heights are updated bottom-up after each rotation: first the lowered node, then the pivot.
- **AVL balancing:**
  - After a recursive insert or remove returns, `rebalance` runs on each ancestor. It updates the height and computes `bf = h(left) − h(right)`.
  - `bf > 1` is Left-Left when `bf(left) ≥ 0`, otherwise Left-Right.
  - `bf < −1` is Right-Right when `bf(right) ≤ 0`, otherwise Right-Left.
  - The `≥ 0` / `≤ 0` tests also cover deletion, where the child can be exactly balanced.
- **BFS:**
  - Mark the source, set its level to 0 and enqueue it.
  - Dequeue a sector and record it. Every unvisited neighbor is marked, given level = parent level + 1, and enqueued.
  - Sectors are marked when enqueued, so none is visited twice.
  - The 2-hop variant does not expand sectors at level 2, so it counts **hops**, not seconds.
- **DFS:** mark the sector, record it, and recurse into each unvisited neighbor in list order.
- **Reachability:** run BFS from `from`. `to` is reachable when it got a level. `hop_distance` returns that level.
- **Missing-flight search:** find `Control_Tower` by name through the graph, then run BFS from it. The flight is found when its last-known sector is reachable. A graph without a tower, or an invalid sector, returns false with an `[ATC Error]` message.
- **Dijkstra (lazy deletion):**
  - Push `(source, 0)`.
  - Pop the minimum. Skip the entry if its sector is already settled or the popped distance is larger than `distance[u]` (a stale entry). Otherwise settle the sector.
  - For each neighbor corridor with weight > 0: if `distance[u] + w < distance[v]`, update `distance[v]` and `parent[v]` and push `(v, distance[v])`.
  - Weights are read from the live adjacency list, so removed corridors are gone.
- **Route reconstruction:** follow `parent[]` from the destination back to the source into a temporary array, then copy it in reverse into `route.sectors` (as strings) so it reads source → destination. `total_weight = distance[dest]`. An unreachable destination gives an empty route.
- **Nearest runway:** run Dijkstra once and pick the candidate runway with the smallest finite distance.
- **Next hop:**
  - `next_hop(route, current)` finds `current` in the route and returns the sector after it. It returns -1 for an empty route, a sector not on the route, or the destination itself.
  - The engine form computes a route, takes the hop after the source, frees the route, and returns `""` when there is nowhere to move.

## 7. Complexity analysis

V = sectors, E = corridors, n = flights, h = tree height, L = route length.

| Operation | Time | Extra space |
|---|---|---|
| BST insert / search / remove | O(h): average O(log n), worst O(n) (sorted input) | O(h) recursion |
| BST inorder, height, destroy | O(n) | O(h) |
| AVL insert / search / remove | O(log n) | O(log n) recursion |
| AVL rotation, `height()` | O(1) | — |
| `add_sector` | O(V) duplicate scan, plus O(V²) copy when it resizes (amortized O(V)) | — |
| `add_corridor` (new) / `get_weight` | O(1) | — |
| `add_corridor` (update) / `remove_corridor` | O(degree) | — |
| `get_sector_index` | O(V) | — |
| `display_adjacency_list` / `_matrix` | O(V + E) / O(V²) | — |
| Graph storage | — | O(V² + E) |
| BFS, DFS, `is_reachable`, missing-flight search | O(V + E) | O(V) |
| Dijkstra (binary heap, lazy deletion) | O((V + E) log E) = O(E log V) | O(V + E) |
| Route reconstruction | O(V) | O(V) |
| `next_hop(route, idx)` / engine `next_hop(name, name)` | O(L) / O(E log V) | O(V + E) |

## 8. Integration

Person 3's code calls these interfaces. All of them are kept, with the same names and argument types:

| Caller | Calls |
|---|---|
| `file_loader` (P1 stub) | `add_sector(name, bool)`, `add_corridor(string, string, int)` |
| `game_engine` | `get_sector_count`, `get_sector_name`, `get_sector_index`, `is_runway`, `remove_corridor("Control_Tower","Cache_Fly")` (cyber), `display_adjacency_list/matrix` |
| `game_engine` | `sweep->is_reachable(string,string)`, `bfs_from(string)`, `dfs_from(string)`, `search_missing_flight("Control_Tower", sector)` |
| `game_engine` | `router->next_hop(sector, runway)` every tick, `find_fastest_route(string,string)` + `print_route(r)` + `delete[] r.sectors` |
| `game_engine` | `flight_bst/flight_avl.insert(flight*)`, `.remove(callsign)`, `.height()`, `.inorder_traversal()` |
| `radar_mst` | `get_sector_count`, `get_sector_name`, `get_neighbors(i)` → `neighbor_node{sector_index, weight, next}` |

**Landing and crash:** `unregister_flight_everywhere()` deletes the registry flight *before* it calls `flight_bst.remove()` and `flight_avl.remove()`. Each tree node keeps its own callsign copy, so removal never reads the freed flight. Test 3 covers this.

**Option 22:** the engine's existing option prints the live trees' heights and in-order lists, and still works. `compare_bst_avl_heights_demo()` (declared in `avl_flight.h`) is ready for Person 3 to call from `option_compare_heights()` if they want the 15-callsign table in the game. I did not edit the engine.

**Build:** `build_mayday.bat` now also compiles `bst_flight.cpp avl_flight.cpp space_graph.cpp radar_sweep.cpp route_finder.cpp`.

## 9. Testing

The test runner is `tests/person2_tests.cpp`, with its own `main()` and kept outside the game build. To run it, open a VS x64 Native Tools prompt and run `tests\build_person2_tests.bat`. It builds with the debug CRT (`/MDd`) and checks each test group for leaks with heap checkpoints. A deliberate leak was confirmed to be caught by the same check.

**Results (MSVC 14.52, run on 2026-11-22): 160 passed, 0 failed, no leaks in any group.**

| Group | What is checked |
|---|---|
| 1. BST | Empty tree; null insert; insert; search hit and miss (same pointer returned); duplicate rejection; alphabetical in-order; removing a leaf, a one-child node, a two-child node and the root; removing a missing callsign; `destroy_tree` and reuse; flights intact afterwards |
| 2. AVL | All four rotation cases (LL, RR, LR, RL), with counters and root checks; a rotation triggered by deletion; 40 sorted inserts (height 6) and interleaved removals with `is_valid_avl()` after **every** operation; search pointers; duplicates; nonexistent remove |
| 3. Ownership | Trees never free flights; removal by callsign works **after** the flight was deleted (the engine's real call order) |
| 4. Height demo | 15 sorted callsigns: BST height 15, AVL height 4, printed side by side |
| 5. SpaceGraph | Index assignment; duplicate and empty names; runway detection; invalid indexes, self-loop, zero and negative weights; resizing from capacity 4 to 16 with corridors preserved; corridor update with no duplicate; neighbor accessors; removal from **both** representations; `is_consistent()` |
| 6. RadarSweep | Brief fixture: BFS reaching 11 sectors with correct levels; 2-hop output; DFS; reachable, unreachable and invalid cases; Restricted Zone's only corridor is to Alpha Tree; after cutting it, Restricted Zone is unreachable and the missing-flight search fails; no-tower graph; null graph |
| 7. RouteFinder | Heap ordering and growth; direct route; **Case 1** Alpha_Tree→RWY12 = 11 s via Syntax_Sector (the 3-hop path costs 12, so fewest hops is not fastest); `next_hop` cases; **Case 2** Restricted_Zone→nearest emergency runway = RWY09 at 12 s (RWY12 would be 14 s); source = destination; non-runway destination; invalid indexes; **Case 3** cyber cut of Syntax_Sector–Alpha_Tree reroutes to 12 s, then cutting Alpha_Tree–Restricted_Zone gives no route |
| 8. Integration | Real `airspace.txt` through `file_loader`: 10 sectors and 14 corridors, consistent; tower BFS; the engine's four missing-flight sectors; Control_Tower→RWY09 = 6 s; Restricted_Zone's nearest emergency runway is RWY12 at 6 s; Person 3's `radar_mst` gives 9 edges at cost 23; after the engine's own cyber call, Control_Tower→Cache_Fly goes from 3 s to 6 s and the MST is recomputed |

Also verified:
- The full game builds with `/EHsc /W3`, and the Person 2 files compile with no warnings at `/W4`.
- A scripted game session (run on a copy of the data files) exercised options 16, 17, 18, 22, 02, 19, 23 and 24 and showed correct behavior.

## 10. Limitations

- **`airspace.txt` does not match the brief's topology.** It has no Alpha Tree or Holding Pattern Delta, has a NullAir_Approach sector, and Restricted_Zone has three corridors. So demos 4 and 9 and Dijkstra Cases 1–3 run on a **test fixture** (`build_brief_fixture` in the tests), and the live file is left unchanged. The team needs to decide which topology is official (see decision D17).
- **`route` has no copy control.** It is in Person 1's `flight.h`, so copies share one `sectors` array and only one copy may free it.
- **Sector lookup by name is a linear scan,** O(V). That is fine for about 10 sectors.
- **Runway detection in `add_sector(name)` is by name prefix** (`RWY`, `Runway`). The loader passes the file's explicit runway flag instead.
- **These are pre-existing behaviors in other members' code, outside Person 2's modules:**
  - Person 1's hash-table stub stores *copies* of flights. When the registry flight moves each tick, the hash copy's `current_sector` is not updated, so option 02 (Reroute) computes its route from a stale sector. In the smoke test it showed `Syntax_Sector -> RWY12` while MD101 was really at Control_Tower.
  - `pick_random_runway()` hardcodes runway names.
  - Random callsigns such as `TN45` can collide. The trees ignore a duplicate callsign.
