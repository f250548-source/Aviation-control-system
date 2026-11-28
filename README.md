# MayDay-404

A console based Air Traffic Control game written in C++ for the CL2001 Data Structures semester project (BCS 3, Fall 2026), National University of Computer and Emerging Sciences, Faisalabad-Chiniot Campus.

## Team

| Name | Roll Number | Modules |
|------|-------------|---------|
| Uzair Bin Waqas | 25F-0703 | 1, 2, 3, 11 |
| Ayan Abdullah | 25F-0732 | 4, 5, 6, 7, 8 |
| Haziq Salman | 25F-0548 | 9, 10 (plus menu, utils, main) |

## About the Game

The player is the Chief Routing Officer (CRO) of the BlackBox airline during an island wide air emergency over BitLand. Flights arrive, fuel drops every tick, and emergencies (war, hijack, cyber attack, missing flight, fuel, medical, weather, mechanical) get worse if they are ignored.

- Each turn is 1 tick (30 simulated seconds) and the player gets 2 actions per turn.
- The game lasts at most 30 ticks.
- Credibility starts at 100 and can go up to 200. The game is lost when it reaches 0.
- The player wins by surviving all ticks with no crash, hijack, cyber breach or war failure.
- Results are saved to `leaderboard.txt`.

## Project Rules We Followed

- No STL containers and no `<algorithm>`. All data structures are written by us with pointers and `new` / `delete`.
- Own helper functions: `swap_values`, `min_value`, `max_value`, `swap_strings`, and our own sorting routines.
- `std::string` is used with basic operations only.
- Allowed headers: `iostream`, `fstream`, `string`, `cstdlib`, `ctime`.
- `using namespace std;` in every file.
- snake_case for files, classes, functions and variables. UPPER_SNAKE_CASE for constants and enum values.
- Every class that owns dynamic memory has a destructor.
- Wrong input is handled with `cout` messages starting with `[ATC Error]`. No exception classes.

## Data Structures Used

| Module | Name | Data Structure |
|--------|------|----------------|
| 1 | Aircraft and Crew Registry | Singly linked list, hash table (linear probing, size 101, tombstones) |
| 2 | Landing and Emergency Dispatch | Queue, min-heap |
| 3 | Command History and Undo | Doubly linked list, undo and redo stacks |
| 4 | Flight Manifest | Binary search tree |
| 5 | Balanced Flight Index | AVL tree |
| 6 | Airspace Network | Graph (adjacency list and adjacency matrix) |
| 7 | Radar Sweep | BFS, DFS |
| 8 | Fastest Intercept Route | Dijkstra with a custom min-heap |
| 9 | Minimum Radar Coverage | Kruskal MST, union-find, merge sort |
| 10 | Game Loop and UI | Integration of all structures |
| 11 | File Persistence | File handling, leaderboard |

## Project Files

Source files (each has a `.h` and `.cpp` unless noted):

- `flight.h` shared types (flight, crew, scenario_event, route, priority enum)
- `registration`, `hash_table`, `emergency_landing`, `command_history`, `file_loader`, `leader_board`
- `bst_flight`, `avl_flight`, `space_graph`, `radar_sweep`, `route_finder`
- `radar_mst`, `game_engine`, `menu`, `utils`
- `main.cpp`
- `build_mayday.bat`

Text files:

| File | Purpose |
|------|---------|
| `rules.txt` | Maximum ticks, starting credibility, actions per turn, win and lose notes |
| `gamemenu.txt` | The 24 in-game menu options |
| `about.txt` | About the team and the game |
| `airspace.txt` | SECTOR and CORRIDOR definitions for the graph |
| `flights.txt` | Starter flights |
| `crew.txt` | Pilot and crew profiles |
| `scenario.txt` | Spawn schedule and scenario events |
| `avl_demo.txt` | 15 alphabetical callsigns for the BST and AVL height comparison |
| `leaderboard.txt` | Saved results, format: `# | CRO | Result | Cred | Ticks | Crash | Hijack | Breach | Date` |

If a text file is missing, the loader prints a message and uses default data where it can.

## Build and Run

### Visual Studio

1. Open the project in Visual Studio and make sure all `.cpp` files are added.
2. Right-click the project, choose Properties, then Debugging.
3. Set Working Directory to `$(ProjectDir)` (select All Configurations and All Platforms first).
4. Keep all `.txt` files in the same folder as the `.cpp` files.
5. Build and run with Ctrl+F5.

### Command line

1. Run `build_mayday.bat`. It compiles `main.cpp`, `utils.cpp`, `menu.cpp`, `radar_mst.cpp`, `game_engine.cpp`, `bst_flight.cpp`, `avl_flight.cpp`, `space_graph.cpp`, `radar_sweep.cpp` and `route_finder.cpp`.
2. Run the program from the folder that contains the `.txt` files.

If the game prints `Could not open ...`, the working directory is wrong. Fix it as described above or copy the `.txt` files next to the `.exe`.

## Main Menu

1. New Game
2. View Leader Board
3. About
4. Rules
5. Exit

## Game Menu

| No. | Option | No. | Option |
|-----|--------|-----|--------|
| 01 | Register Aircraft | 13 | Display Pending Arrivals |
| 02 | Register Crew Member | 14 | Display Crew Members |
| 03 | Search Flight by Call Sign | 15 | Display Flight Records |
| 04 | Add Normal Arrival | 16 | Display Airspace Network |
| 05 | Add Emergency Arrival | 17 | BFS Radar Sweep |
| 06 | Process Next Landing | 18 | DFS Sector Traversal |
| 07 | Clear Flight for Runway | 19 | Search for Missing Flight |
| 08 | Reroute Flight | 20 | Find Fastest Route to Runway |
| 09 | Intercept Aircraft | 21 | Generate Minimum Radar Coverage |
| 10 | Escalate Priority | 22 | Compare BST and AVL Heights |
| 11 | Counter Cyber Attack | 23 | End Turn / Advance Tick |
| 12 | Undo Last Command (undo or redo) | 24 | Exit to Main Menu (counts as LOSE) |

- Free actions: 13, 14, 15, 16, 22 and 23. All other options cost 1 action, and the turn ends after 2 actions.
- Undoable commands: 07 to 11.
- Options 04 and 05 are for manual testing.
- A wrong command costs credibility, and the penalty grows for each type of mistake.

## Credibility

| Event | Change |
|-------|--------|
| Flight lands safely | +2 |
| Hijack countered | +20 |
| Cyber alert countered | +15 |
| War aircraft intercepted or cleared | +30 |
| Missing flight located | +20 |
| Wrong command | -5 (grows by 5 for each type) |
| Failed hijack counter | -20 |
| Failed cyber counter | -15 |
| Emergency landing on runway 04 or 17 | -10 |
| Crash | -50 |

## Deviations from the Manual

1. **Runway 27 check.** The manual asks if Runway 27 is reachable from Alpha Tree, but our airspace has no Runway 27. Option 18 checks Runway 09 and Runway 12 instead.
2. **Cyber attack timing.** The manual gives different timings in different sections. We use the tick window of each cyber group (Phantom Root 2, Leak Air 3, Crack Heap 1). When the window ends, the corridor between Syntax Sector and Alpha Tree is disabled, and the tower is disabled one tick later.
3. **Restricted Zone connection.** The manual's corridor table has a Restricted Zone to Air Segment row, but the text says the Restricted Zone is reachable only through Alpha Tree. We followed the text.
4. **Redo.** The manual asks for redo but the menu has only one undo option, so redo is inside option 12 together with undo.
5. **Tree pointers.** The BST and AVL store a non-owning `flight*` plus their own copy of the callsign key, so removing a flight from the registry never leaves a broken pointer in the trees.
6. **Undirected corridors.** Corridors in the airspace graph are treated as two-way.

## Known Limitations

1. **Console only.** The radar and airspace map are printed as text, with no graphical map.
2. **Working directory dependence.** The `.txt` files must be found in the working directory, otherwise the game falls back to default data.
3. **No saving of a game in progress.** Only the final result goes to `leaderboard.txt`, so a game stopped halfway cannot be resumed.
4. **Limited and random content.** There are only four missing flights and six threat actors, and events spawn randomly, so a game cannot be replayed exactly. Adding content needs changes in the text files and the engine.

## Contributions

- **Uzair Bin Waqas:** registration, hash table, emergency landing queue and heap, command history, file loader, leaderboard.
- **Ayan Abdullah:** BST, AVL tree, airspace graph, radar sweep, route finder, design notes and tests.
- **Haziq Salman:** radar MST, game engine, menu, utils, main.
