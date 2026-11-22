# MayDay-404 — Person 2 Decision Record (Modules 4–8)

ADR format: context → options → decision → reasoning → consequences. The status is **Accepted** unless marked **Pending**.

---

### D1 — Trees hold non-owning `flight*` pointers and their own copy of the callsign
- **Context:** The stub trees stored a *copy* of each `flight`, so tree data went stale. The engine's `unregister_flight_everywhere()` deletes the registry flight *before* it calls `bst.remove()` and `avl.remove()`.
- **Options:**
  1. Copy the whole flight.
  2. Store only a pointer and compare `flight_ref->callsign`.
  3. Store a pointer plus a copy of the callsign as the key.
- **Decision:** Option 3. Nodes never delete flights.
- **Reasoning:** The registry is the single owner. A pointer gives live data. The key copy means removal never reads the already-deleted flight. Option 2 would be a use-after-free with the engine's call order.
- **Consequences:** `search()` returns the registry's own record. Test 3 deletes a flight and then removes it from both trees successfully.

### D2 — Duplicate callsigns are ignored
- **Context:** The engine makes random callsigns like `TN45`, which can collide.
- **Options:** Overwrite the pointer, store duplicates, or ignore.
- **Decision:** Ignore. `insert` returns `false` and keeps the original entry, with no console message.
- **Reasoning:** This matches the stub, keeps keys unique, and avoids adding messages to game output.
- **Consequences:** A colliding second flight is in the registry but not in the trees.

### D3 — Height counts nodes (empty tree = 0, leaf = 1)
- **Context:** The stubs and the AVL `height` field already used this convention.
- **Decision:** Both trees count nodes on the longest path.
- **Consequences:** 15 sorted callsigns give BST height 15 and AVL height 4. Measured in edges, these would be 14 and 3.

### D4 — AVL rotations: four named helpers; the case is chosen by the child's balance factor
- **Decision:**
  - `rotate_left` and `rotate_right` are the basic rotations.
  - `rotate_left_right` and `rotate_right_left` combine two of them.
  - `rebalance()` uses `bf(child) ≥ 0` → LL and `bf(child) ≤ 0` → RR. Otherwise it uses LR or RL.
- **Reasoning:** The `≥` / `≤` tests make the same code correct after deletion, where the child can be exactly balanced. Per-case counters let tests and the viva show which rotation ran.
- **Consequences:** Tests check all four cases plus a rotation caused by deletion.

### D5 — Adjacency list: per-sector singly linked list, head insertion, two nodes per corridor
- **Context:** `radar_mst.cpp` uses `neighbor_node {sector_index, weight, next}` and `get_neighbors()` directly.
- **Decision:** Keep that struct and those field names exactly. Insert at the head (O(1)).
- **Consequences:** Neighbor order is the reverse of insertion order, which sets the BFS/DFS tie order. The MST code needed no changes.

### D6 — Adjacency matrix: dynamic `int**`, capacity × capacity
- **Options:** A fixed 2D array, a flat `int*`, or `int**` with rows.
- **Decision:** `int**`, with each row allocated separately.
- **Reasoning:** The brief asks for a dynamically allocated 2D array, and `int**` is the textbook form.
- **Consequences:** `get_weight` is O(1). The destructor frees every row and then the row array.

### D7 — Corridors are undirected
- **Context:** The stub, `airspace.txt` (each pair listed once) and `radar_mst` (which keeps `i < j`) all treat corridors as two-way.
- **Decision:** `add_corridor` and `remove_corridor` always write both directions in both representations. Re-adding an existing pair **updates** its weight.
- **Consequences:** `is_consistent()` checks the matrix is symmetric and that the list matches it.

### D8 — The no-edge value is 0
- **Context:** The stub returned 0 for no edge and rejected weights ≤ 0.
- **Decision:** Keep 0. `get_weight` also returns 0 for invalid indexes. The matrix display prints `.` for 0.
- **Reasoning:** Weights must be positive, so 0 can never be a real corridor. Positive weights also meet Dijkstra's non-negative requirement.

### D9 — Graph storage grows by doubling
- **Context:** The stub had a fixed limit of 64 sectors.
- **Decision:** Start at capacity 4 and double when full.
  - Sector records are copied into the bigger array, but their list heads move **by pointer**, so no list node is copied or freed.
  - The used `count × count` part of the matrix is copied into the new matrix before the old one is freed.
- **Reasoning:** The small starting capacity means resizing actually happens on the real data (10 sectors) and is tested.
- **Consequences:** Tests check corridors survive growth from capacity 4 to 16.

### D10 — Neighbor access for RadarMST
- **Decision:** Keep `neighbor_node* get_neighbors(i)`, which the existing MST uses. Also add the position-based `get_neighbor_count`, `get_neighbor_index` and `get_neighbor_weight` from the brief.
- **Consequences:** `radar_mst` is unchanged, and test 8 confirms it builds a 9-edge MST at cost 23 from the new graph.

### D11 — BFS queue: a fixed circular array of size V
- **Decision:** A `sector_queue` class with `front_index` and `item_count`, where the rear is `(front + count) % capacity`.
- **Reasoning:** Each sector is enqueued at most once (it is marked when enqueued), so V slots are always enough.

### D12 — DFS uses recursion
- **Options:** Recursion, or an explicit array stack.
- **Decision:** Recursion with a visited array, as the stub did.
- **Reasoning:** It is the simplest correct form to explain. Recursion depth is at most V (about 10).

### D13 — Dijkstra heap: binary min-heap with lazy deletion
- **Options:** Decrease-key (needs a position map), lazy deletion, or the stub's O(V²) linear scan.
- **Decision:** An array heap of `{sector_index, distance}` that doubles when full. A new push replaces decrease-key, and stale entries are skipped when popped.
- **Reasoning:** The brief asks for a custom heap. Lazy deletion is simpler than decrease-key and keeps O(E log V).

### D14 — Route ownership: reuse `route` from `flight.h`; the caller frees it *(Pending)*
- **Context:** The brief suggests `Route {int* sectors; int length; int totalCost;}`. The existing code uses `route {string* sectors; sector_count; total_weight}` in Person 1's `flight.h`, and the engine frees it with `delete[] r.sectors`.
- **Decision:** Return the existing `route`. The caller frees it once, with `delete[] r.sectors` or `route_finder::free_route(r)`. `next_hop(route, int)` matches by sector name.
- **Reasoning:** Changing the struct would mean editing Person 1's header and Person 3's engine.
- **Pending:** `route` has no copy constructor, so copies share one array. **Person 1** should confirm whether `route` stays in `flight.h` and whether to add copy control or move it into `route_finder.h`.

### D15 — Compatibility with Person 3's code
- **Decision:**
  - Keep every signature the engine and the MST call: name-based `add_corridor`, `remove_corridor`, `is_reachable`, `bfs_from`, `dfs_from`, `search_missing_flight(tower, sector)`, `find_fastest_route`, `next_hop(string,string)`, `print_route`.
  - Add the brief's index-based overloads beside them.
  - Turning `void` returns into `bool` or `int` is source-compatible, because the engine ignores those return values.
- **Changes to Person 3's files:** none. `game_engine.*`, `radar_mst.*`, `menu.*`, `utils.*` and `main.cpp` are untouched.

### D16 — Snake_case names kept, brief names added as `typedef`s; header/source split
- **Context:** The repository already had `bst_flight.h` and similar files, with classes `bst_flight` and so on, included by `game_engine.h`.
- **Decision:**
  - Keep those names.
  - Add `typedef bst_flight BSTFlight;` (and so on).
  - Split each module into `.h` and `.cpp`, as Person 3's `radar_mst` does.
- **Consequence:** I made the **one change to a shared file**: the `SRC=` line in `build_mayday.bat` now also lists the five new `.cpp` files. This build-configuration edit cannot be avoided, because the new source files must be compiled.

### D17 — Brief demonstrations run on a test fixture *(Pending)*
- **Context:** The real `airspace.txt` has no Alpha Tree or Holding Pattern Delta, and its Restricted_Zone has three corridors, not one.
- **Decision:**
  - Leave `airspace.txt` unchanged.
  - Run the brief's demos (Restricted Zone reachable only through Alpha Tree, Dijkstra Cases 1–3) on an 11-sector fixture inside `tests/person2_tests.cpp`.
  - Test the real file separately (test group 8).
- **Pending:** the team must decide whether `airspace.txt` should be changed to match the brief's topology. If it is, update the engine's hardcoded cyber corridor (`Control_Tower`–`Cache_Fly`) too.

### D18 — `next_hop(string,string)` returns `""` at the destination
- **Context:** The stub returned the current sector when already at the runway. The engine then printed "vectors to RWY09" every tick.
- **Decision:** Return `""` when there is nowhere to move: at the destination, no route, or an unknown name. The engine already skips empty hops (`hop.length() > 0`). The function prints nothing because it runs every tick for every flight.
- **Consequences:** Flights stop showing "vectors" messages once they reach their runway. Fuel still burns, as the engine decides.

### D19 — `find_fastest_route` requires a runway; `find_shortest_path` does not
- **Decision:**
  - The runway route rejects non-runway destinations with `[ATC Error] Destination X is not a runway.`
  - The general `find_shortest_path` exists for sector-to-sector use.
  - The engine's per-tick `next_hop` uses the general form, so flights still move toward whatever target they were given.
