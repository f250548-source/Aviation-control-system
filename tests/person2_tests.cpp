// ============================================================================
// Person 2 test runner (Modules 4-8)
//
// Separate executable with its own main(); it is NOT part of the game build.
// Build + run from the repository root with tests\build_person2_tests.bat.
//
// Section 8 reads the real airspace.txt (read-only). Everything else uses
// in-memory test data, so no live game file is modified.
//
// On MSVC debug builds (/MDd) every test group is wrapped in a heap
// checkpoint: any bytes still allocated after the group finishes are reported
// as a memory-leak failure.
// ============================================================================

#include "../avl_flight.h"
#include "../bst_flight.h"
#include "../file_loader.h"
#include "../flight.h"
#include "../radar_mst.h"
#include "../radar_sweep.h"
#include "../route_finder.h"
#include "../space_graph.h"
#include <iostream>
#include <string>
using namespace std;

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#define LEAK_CHECKS_ENABLED 1
#endif

// ---------------------------------------------------------------------------
// Tiny test framework
// ---------------------------------------------------------------------------

static int tests_passed = 0;
static int tests_failed = 0;

static void check(bool condition, string description) {
    if (condition) {
        tests_passed = tests_passed + 1;
        cout << "  [PASS] " << description << endl;
    } else {
        tests_failed = tests_failed + 1;
        cout << "  [FAIL] " << description << endl;
    }
}

static void section(string title) {
    cout << endl << "==================== " << title << " ====================" << endl;
}

// Runs one test group between two heap checkpoints
static void run_group(string name, void (*group)()) {
#ifdef LEAK_CHECKS_ENABLED
    _CrtMemState before;
    _CrtMemState after;
    _CrtMemState difference;
    _CrtMemCheckpoint(&before);
#endif
    group();
#ifdef LEAK_CHECKS_ENABLED
    _CrtMemCheckpoint(&after);
    bool leaked = _CrtMemDifference(&difference, &before, &after) != 0 &&
                  difference.lCounts[_NORMAL_BLOCK] > 0;
    check(!leaked, name + ": no memory leaked by the group");
    if (leaked) {
        _CrtMemDumpStatistics(&difference);
    }
#else
    cout << "  (leak check skipped: not an MSVC debug build)" << endl;
    (void)name;
#endif
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static flight* make_flight(string callsign) {
    flight* f = new flight;
    f->callsign = callsign;
    f->aircraft_type = "A320";
    f->airline = "TestAir";
    f->origin = "Test";
    f->destination = "RWY09";
    f->passenger_count = 100;
    f->fuel_remaining = 10;
    f->current_sector = "Control_Tower";
    f->assigned_runway = "RWY09";
    f->arrival_tick = 0;
    f->priority_level = NORMAL;
    f->status = "ACTIVE";
    f->next = 0;
    return f;
}

static string join(string* items, int count) {
    string result = "";
    int i = 0;
    while (i < count) {
        if (i > 0) {
            result = result + " ";
        }
        result = result + items[i];
        i = i + 1;
    }
    return result;
}

static string route_text(route& r) {
    if (r.sectors == 0) {
        return "(none)";
    }
    return join(r.sectors, r.sector_count);
}

// Test fixture modelled on the project brief's airspace description. It is
// NOT the game's airspace.txt (which uses different sectors); it exists so the
// brief's demonstrations (Alpha Tree, Restricted Zone only via Alpha Tree,
// Holding Pattern Delta) can be shown.
static void build_brief_fixture(space_graph& g) {
    g.add_sector("Control_Tower");
    g.add_sector("Air_Segment");             // North
    g.add_sector("Cache_Fly");               // East
    g.add_sector("Syntax_Sector");           // West
    g.add_sector("Alpha_Tree");              // South
    g.add_sector("Restricted_Zone");
    g.add_sector("Holding_Pattern_Delta");
    g.add_sector("RWY12");
    g.add_sector("RWY04");
    g.add_sector("RWY17");
    g.add_sector("RWY09");

    g.add_corridor(string("Control_Tower"), string("Air_Segment"), 4);
    g.add_corridor(string("Control_Tower"), string("Cache_Fly"), 3);
    g.add_corridor(string("Control_Tower"), string("Syntax_Sector"), 2);
    g.add_corridor(string("Control_Tower"), string("Alpha_Tree"), 5);
    g.add_corridor(string("Air_Segment"), string("RWY12"), 3);
    g.add_corridor(string("Air_Segment"), string("Holding_Pattern_Delta"), 2);
    g.add_corridor(string("Cache_Fly"), string("RWY04"), 4);
    g.add_corridor(string("Cache_Fly"), string("Holding_Pattern_Delta"), 3);
    g.add_corridor(string("Syntax_Sector"), string("RWY17"), 6);
    g.add_corridor(string("Syntax_Sector"), string("Alpha_Tree"), 2);
    g.add_corridor(string("Alpha_Tree"), string("Restricted_Zone"), 3);   // only RZ link
    g.add_corridor(string("Alpha_Tree"), string("RWY09"), 9);
    g.add_corridor(string("Holding_Pattern_Delta"), string("RWY09"), 4);
    g.add_corridor(string("Holding_Pattern_Delta"), string("RWY12"), 6);
}

// ---------------------------------------------------------------------------
// 1. BST (Module 4)
// ---------------------------------------------------------------------------

static void test_bst() {
    section("1. BSTFlight");
    const int COUNT = 8;
    string names[COUNT] = {"M", "F", "T", "B", "H", "P", "W", "A"};
    flight* flights[COUNT];
    int i = 0;
    while (i < COUNT) {
        flights[i] = make_flight(names[i]);
        i = i + 1;
    }
    {
        bst_flight tree;
        check(tree.height() == 0 && tree.size() == 0, "empty tree has height 0, size 0");
        check(tree.search("M") == 0, "search on empty tree returns null");
        check(!tree.remove("M"), "remove on empty tree is safe and returns false");
        check(!tree.insert(0), "inserting a null flight pointer is rejected");

        i = 0;
        while (i < COUNT) {
            tree.insert(flights[i]);
            i = i + 1;
        }
        check(tree.size() == 8, "8 flights inserted");
        check(tree.height() == 4, "height of M,F,T,B,H,P,W,A tree is 4 (nodes)");
        check(tree.search("H") == flights[4], "search returns the SAME flight pointer");
        check(tree.search("Z") == 0, "search for a missing callsign returns null");

        check(!tree.insert(flights[0]), "duplicate callsign insert is rejected");
        flight* duplicate = make_flight("M");
        check(!tree.insert(duplicate), "different flight with same callsign is rejected");
        check(tree.search("M") == flights[0], "original entry kept after duplicate attempt");
        delete duplicate;
        check(tree.size() == 8, "size unchanged by duplicates");

        string sorted[16];
        int n = tree.inorder_callsigns(sorted, 16);
        check(join(sorted, n) == "A B F H M P T W", "inorder traversal is alphabetical");
        tree.inorder_traversal();

        check(tree.remove("H"), "remove leaf H");
        check(tree.search("H") == 0, "H gone after removal");
        check(tree.remove("B"), "remove B (one child: A)");
        n = tree.inorder_callsigns(sorted, 16);
        check(join(sorted, n) == "A F M P T W", "order intact after one-child removal");
        check(tree.remove("T"), "remove T (two children: P, W)");
        n = tree.inorder_callsigns(sorted, 16);
        check(join(sorted, n) == "A F M P W", "order intact after two-child removal");
        check(tree.remove("M"), "remove the root M (two children)");
        n = tree.inorder_callsigns(sorted, 16);
        check(join(sorted, n) == "A F P W", "order intact after root removal");
        check(tree.search("P") == flights[5], "successor P still maps to its own flight");
        check(!tree.remove("M"), "removing a callsign that is gone returns false");
        check(tree.size() == 4, "size is 4 after 4 removals");

        tree.destroy_tree();
        check(tree.size() == 0 && tree.height() == 0, "destroy_tree empties the tree");
        check(tree.insert(flights[0]), "tree usable again after destroy_tree");
    }
    // Trees are gone; every flight must still be intact (the tree never owned them)
    bool all_intact = true;
    i = 0;
    while (i < COUNT) {
        if (flights[i]->callsign != names[i]) {
            all_intact = false;
        }
        delete flights[i];   // would be a double delete if the tree had freed it
        i = i + 1;
    }
    check(all_intact, "all flights intact after tree removal + destruction");
}

// ---------------------------------------------------------------------------
// 2. AVL (Module 5)
// ---------------------------------------------------------------------------

// Insert three callsigns and report which rotation fired
static void avl_case(string a, string b, string c, string expected_case) {
    flight* fa = make_flight(a);
    flight* fb = make_flight(b);
    flight* fc = make_flight(c);
    {
        avl_flight tree;
        tree.insert(fa);
        tree.insert(fb);
        tree.insert(fc);
        check(tree.get_rotation_count(expected_case) == 1 && tree.root_callsign() == "B" &&
              tree.height() == 2 && tree.is_valid_avl(),
              expected_case + " case (" + a + "," + b + "," + c +
              "): one " + expected_case + " rotation, root B, height 2");
    }
    delete fa;
    delete fb;
    delete fc;
}

static void test_avl() {
    section("2. AVLFlight");
    avl_case("C", "B", "A", "LL");
    avl_case("A", "B", "C", "RR");
    avl_case("C", "A", "B", "LR");
    avl_case("A", "C", "B", "RL");

    // Deletion that requires a rotation: B(A, C(-, D)); removing A unbalances B
    {
        flight* f[4] = {make_flight("B"), make_flight("A"), make_flight("C"), make_flight("D")};
        {
            avl_flight tree;
            int i = 0;
            while (i < 4) {
                tree.insert(f[i]);
                i = i + 1;
            }
            int rr_before = tree.get_rotation_count("RR");
            tree.remove("A");
            check(tree.root_callsign() == "C" && tree.get_rotation_count("RR") == rr_before + 1 &&
                  tree.is_valid_avl(), "remove A from B(A,C(-,D)) triggers RR rotation, root C");
        }
        int i = 0;
        while (i < 4) {
            delete f[i];
            i = i + 1;
        }
    }

    // Stress: 40 inserts in sorted order, then remove every 3rd, then the rest;
    // the AVL invariants must hold after EVERY single operation
    const int N = 40;
    flight* flights[N];
    int i = 0;
    while (i < N) {
        string callsign = "K";
        callsign = callsign + (char)('0' + i / 10) + (char)('0' + i % 10);
        flights[i] = make_flight(callsign);
        i = i + 1;
    }
    {
        avl_flight tree;
        bool valid_after_inserts = true;
        i = 0;
        while (i < N) {
            tree.insert(flights[i]);
            if (!tree.is_valid_avl()) {
                valid_after_inserts = false;
            }
            i = i + 1;
        }
        check(valid_after_inserts, "balance factors valid after each of 40 sorted inserts");
        check(tree.height() == 6, "40 sorted inserts give AVL height 6 (BST would be 40)");

        bool valid_after_removes = true;
        bool searches_ok = true;
        i = 0;
        while (i < N) {
            if (i % 3 == 0) {
                tree.remove(flights[i]->callsign);
                if (!tree.is_valid_avl() || tree.search(flights[i]->callsign) != 0) {
                    valid_after_removes = false;
                }
            }
            i = i + 1;
        }
        i = 0;
        while (i < N) {
            if (i % 3 != 0 && tree.search(flights[i]->callsign) != flights[i]) {
                searches_ok = false;
            }
            i = i + 1;
        }
        check(valid_after_removes, "balance factors valid after each removal");
        check(searches_ok, "remaining flights still found with correct pointers");
        check(tree.size() == N - 14, "size correct after removing 14 flights");
        check(!tree.remove("NOPE"), "removing a nonexistent callsign returns false");
        check(!tree.insert(flights[1]), "duplicate insert rejected");

        string sorted[64];
        int n = tree.inorder_callsigns(sorted, 64);
        bool ascending = true;
        int k = 1;
        while (k < n) {
            if (!(sorted[k - 1] < sorted[k])) {
                ascending = false;
            }
            k = k + 1;
        }
        check(ascending && n == N - 14, "inorder traversal still strictly alphabetical");

        bool valid_until_empty = true;
        i = 0;
        while (i < N) {
            tree.remove(flights[i]->callsign);
            if (!tree.is_valid_avl()) {
                valid_until_empty = false;
            }
            i = i + 1;
        }
        check(valid_until_empty && tree.size() == 0 && tree.height() == 0,
              "tree valid down to empty");
        tree.inorder_traversal();
    }
    i = 0;
    while (i < N) {
        delete flights[i];
        i = i + 1;
    }
}

// ---------------------------------------------------------------------------
// 3. Tree ownership with the engine's real call order
// ---------------------------------------------------------------------------

static void test_tree_ownership() {
    section("3. Tree ownership (non-owning Flight pointers)");
    flight* f = make_flight("OWN1");
    flight* g = make_flight("OWN2");
    {
        bst_flight bst;
        avl_flight avl;
        bst.insert(f);
        avl.insert(f);
        bst.insert(g);
        avl.insert(g);
        check(bst.remove("OWN1") && avl.remove("OWN1"), "remove OWN1 from both trees");
        check(f->callsign == "OWN1" && f->fuel_remaining == 10,
              "OWN1 flight record untouched after tree removal");

        // game_engine::unregister_flight_everywhere deletes the registry
        // flight FIRST, then removes it from the trees. The trees must not
        // read through the (now dangling) pointer while removing.
        delete g;
        g = 0;
        check(bst.remove("OWN2") && avl.remove("OWN2"),
              "removal by callsign works after the flight was already deleted");
        check(bst.size() == 0 && avl.size() == 0, "both trees empty");
    }
    check(f->callsign == "OWN1", "flight still alive after both trees destroyed");
    delete f;   // debug heap would abort here if a tree had freed it
}

// ---------------------------------------------------------------------------
// 4. BST vs AVL height demonstration (Module 5 requirement / menu option 22)
// ---------------------------------------------------------------------------

static void test_height_demo() {
    section("4. BST vs AVL height demonstration");
    compare_bst_avl_heights_demo();

    const int COUNT = 15;
    string callsigns[COUNT] = {"AA101", "BA202", "CX303", "DL404", "EK505",
                               "FZ606", "GF707", "HU808", "IB909", "JL110",
                               "KL211", "LH312", "MH413", "NH514", "OS615"};
    flight flights[COUNT];   // outlive both trees (declared first)
    int i = 0;
    while (i < COUNT) {
        flights[i].callsign = callsigns[i];
        flights[i].next = 0;
        i = i + 1;
    }
    bst_flight bst;
    avl_flight avl;
    i = 0;
    while (i < COUNT) {
        bst.insert(&flights[i]);
        avl.insert(&flights[i]);
        i = i + 1;
    }
    check(bst.height() == 15, "BST height on 15 sorted callsigns = 15 (degenerate)");
    check(avl.height() == 4, "AVL height on 15 sorted callsigns = 4 (balanced)");
    check(avl.is_valid_avl(), "AVL invariants hold after the demo inserts");
}

// ---------------------------------------------------------------------------
// 5. SpaceGraph (Module 6)
// ---------------------------------------------------------------------------

static void test_graph() {
    section("5. SpaceGraph");
    space_graph g;
    check(g.get_sector_count() == 0 && g.get_capacity() == 4, "new graph: 0 sectors, capacity 4");

    int a = g.add_sector("Control_Tower");
    int b = g.add_sector("Cache_Fly");
    int c = g.add_sector("RWY09");
    check(a == 0 && b == 1 && c == 2, "add_sector returns consecutive indexes");
    check(g.add_sector("Cache_Fly") == 1 && g.get_sector_count() == 3,
          "duplicate sector name returns existing index, no new sector");
    check(g.add_sector("") == -1, "empty sector name rejected with -1");
    check(g.is_runway(c) && !g.is_runway(a), "RWY09 detected as runway by name");
    check(g.is_runway(g.add_sector("Runway_12")), "'Runway_12' detected as runway");
    check(g.is_runway(g.add_sector("Pad_X", true)), "explicit runway flag respected");
    check(!g.is_runway(99) && !g.is_runway(-1), "is_runway false for invalid indexes");

    check(g.add_corridor(a, b, 5), "add corridor Control_Tower-Cache_Fly (5s)");
    check(g.get_weight(a, b) == 5 && g.get_weight(b, a) == 5, "matrix symmetric (undirected)");
    check(g.get_list_weight(a, b) == 5 && g.get_list_weight(b, a) == 5,
          "adjacency list has both directions");

    // Capacity was 4, we now have 5 sectors -> must have resized at least once
    check(g.get_capacity() >= 5, "matrix resized when sector count exceeded capacity");
    int added = 0;
    while (added < 10) {
        string name = "Extra_";
        name = name + (char)('A' + added);
        g.add_sector(name);
        added = added + 1;
    }
    check(g.get_sector_count() == 15 && g.get_capacity() == 16, "15 sectors -> capacity 16");
    check(g.get_weight(a, b) == 5 && g.get_list_weight(a, b) == 5,
          "corridor preserved across several resizes");

    check(!g.add_corridor(a, 99, 3), "corridor to invalid index rejected");
    check(!g.add_corridor(-1, a, 3), "corridor from negative index rejected");
    check(!g.add_corridor(a, a, 3), "self-loop rejected");
    check(!g.add_corridor(a, c, 0), "zero weight rejected");
    check(!g.add_corridor(a, c, -4), "negative weight rejected (Dijkstra needs >= 0)");
    check(!g.add_corridor(string("Nowhere"), string("RWY09"), 3), "unknown sector name rejected");

    check(g.add_corridor(a, c, 7), "add corridor Control_Tower-RWY09 (7s)");
    check(g.get_corridor_count() == 2, "corridor count = 2");
    check(g.add_corridor(c, a, 4), "re-adding the same corridor updates it");
    check(g.get_weight(a, c) == 4 && g.get_list_weight(c, a) == 4,
          "updated weight in matrix and list, both directions");
    check(g.get_corridor_count() == 2 && g.get_neighbor_count(a) == 2,
          "update did not create a duplicate corridor");

    check(g.get_neighbor_count(a) == 2, "Control_Tower has 2 neighbors");
    int p0 = g.get_neighbor_index(a, 0);
    int p1 = g.get_neighbor_index(a, 1);
    check((p0 == b && p1 == c) || (p0 == c && p1 == b), "neighbor indexes enumerate both");
    check(g.get_neighbor_weight(a, 0) + g.get_neighbor_weight(a, 1) == 9,
          "neighbor weights 5 + 4");
    check(g.get_neighbor_index(a, 5) == -1 && g.get_neighbor_index(a, -1) == -1 &&
          g.get_neighbor_index(99, 0) == -1, "invalid neighbor positions return -1");
    check(g.get_weight(a, 99) == 0 && g.get_weight(-3, a) == 0, "get_weight 0 for invalid index");
    check(g.get_sector_name(99) == "" && g.get_sector_index("Ghost") == -1,
          "invalid name/index lookups return \"\" / -1");
    check(g.is_consistent(), "list and matrix consistent after adds/updates");

    check(g.remove_corridor(a, b), "remove Control_Tower-Cache_Fly");
    check(g.get_weight(a, b) == 0 && g.get_weight(b, a) == 0, "removed from matrix (both cells)");
    check(g.get_list_weight(a, b) == 0 && g.get_list_weight(b, a) == 0,
          "removed from adjacency list (both directions)");
    check(g.get_corridor_count() == 1, "corridor count = 1 after removal");
    check(!g.remove_corridor(a, b), "removing a missing corridor returns false");
    check(!g.remove_corridor(a, 99), "removing with invalid index returns false");
    check(g.is_consistent(), "list and matrix consistent after removal");

    g.display_adjacency_list();
}

// ---------------------------------------------------------------------------
// 6. RadarSweep (Module 7) on the brief fixture
// ---------------------------------------------------------------------------

static void test_radar_sweep() {
    section("6. RadarSweep (brief fixture)");
    space_graph g;
    build_brief_fixture(g);
    radar_sweep sweep(&g);
    g.display_adjacency_list();
    g.display_adjacency_matrix();
    check(g.is_consistent(), "fixture: list and matrix consistent");

    int tower = g.get_sector_index("Control_Tower");
    int alpha = g.get_sector_index("Alpha_Tree");
    int restricted = g.get_sector_index("Restricted_Zone");
    int rwy12 = g.get_sector_index("RWY12");
    int rwy04 = g.get_sector_index("RWY04");

    check(sweep.bfs_from(tower) == 11, "BFS from Control_Tower reaches all 11 sectors");
    check(sweep.hop_distance(tower, tower) == 0, "BFS level of source is 0");
    check(sweep.hop_distance(tower, alpha) == 1, "Alpha_Tree is at BFS level 1");
    check(sweep.hop_distance(tower, restricted) == 2, "Restricted_Zone is at BFS level 2");
    check(sweep.hop_distance(restricted, rwy04) == 4, "Restricted_Zone -> RWY04 needs 4 hops");
    sweep.bfs_within_2_hops(tower);
    sweep.bfs_within_2_hops(restricted);
    check(sweep.dfs_from(tower) == 11, "DFS from Control_Tower visits all 11 sectors");
    check(sweep.dfs_from(string("Restricted_Zone")) == 11, "DFS by name works");

    check(sweep.is_reachable(restricted, rwy12), "Restricted_Zone can reach RWY12");
    check(sweep.is_reachable(string("RWY17"), string("RWY09")), "reachability by name");
    check(sweep.is_reachable(tower, tower), "a sector is reachable from itself");
    check(!sweep.is_reachable(-1, tower) && !sweep.is_reachable(tower, 42),
          "invalid indexes are not reachable");
    check(!sweep.is_reachable(string("Ghost"), string("RWY09")), "unknown name not reachable");
    check(sweep.bfs_from(42) == 0 && sweep.dfs_from(-1) == 0, "BFS/DFS reject invalid source");

    // Restricted Zone is reachable ONLY through Alpha Tree
    check(g.get_neighbor_count(restricted) == 1 && g.get_neighbor_index(restricted, 0) == alpha,
          "Restricted_Zone's only corridor is to Alpha_Tree");
    check(sweep.search_missing_flight(restricted), "missing flight near Restricted_Zone found");

    g.remove_corridor(alpha, restricted);
    check(g.is_consistent() && g.get_weight(alpha, restricted) == 0 &&
          g.get_list_weight(restricted, alpha) == 0,
          "Alpha_Tree-Restricted_Zone removed from both representations");
    check(!sweep.is_reachable(tower, restricted),
          "Restricted_Zone unreachable once its Alpha_Tree corridor is cut");
    check(sweep.bfs_from(tower) == 10, "BFS now reaches only 10 sectors");
    check(!sweep.search_missing_flight(restricted), "missing-flight search now fails");
    check(!sweep.search_missing_flight(string("Control_Tower"), string("Ghost")),
          "missing-flight search with unknown sector fails safely");

    // Graph without a Control Tower
    space_graph no_tower;
    no_tower.add_sector("Cache_Fly");
    no_tower.add_sector("RWY09");
    no_tower.add_corridor(0, 1, 3);
    radar_sweep lonely(&no_tower);
    check(!lonely.search_missing_flight(1), "missing Control_Tower handled safely");

    // Null graph pointer
    radar_sweep broken(0);
    check(broken.bfs_from(0) == 0 && !broken.is_reachable(0, 1), "null graph handled safely");
}

// ---------------------------------------------------------------------------
// 7. RouteFinder (Module 8) on the brief fixture
// ---------------------------------------------------------------------------

static void test_route_finder() {
    section("7. RouteFinder (brief fixture)");

    // Heap on its own
    {
        distance_min_heap heap(2);   // tiny capacity: forces grow()
        int values[7] = {5, 3, 8, 1, 9, 2, 7};
        int i = 0;
        while (i < 7) {
            heap.insert(i, values[i]);
            i = i + 1;
        }
        string popped = "";
        while (!heap.is_empty()) {
            heap_entry e = heap.extract_min();
            popped = popped + (char)('0' + e.distance);
        }
        check(popped == "1235789", "min-heap extracts in ascending order (with growth)");
        check(heap.extract_min().sector_index == -1, "extract from empty heap returns -1");
    }

    space_graph g;
    build_brief_fixture(g);
    route_finder finder(&g);
    radar_sweep sweep(&g);
    int tower = g.get_sector_index("Control_Tower");
    int north = g.get_sector_index("Air_Segment");
    int syntax = g.get_sector_index("Syntax_Sector");
    int alpha = g.get_sector_index("Alpha_Tree");
    int restricted = g.get_sector_index("Restricted_Zone");
    int rwy12 = g.get_sector_index("RWY12");
    int rwy09 = g.get_sector_index("RWY09");
    int rwy04 = g.get_sector_index("RWY04");

    // Direct route
    route direct = finder.find_fastest_route(north, rwy12);
    check(route_text(direct) == "Air_Segment RWY12" && direct.total_weight == 3,
          "direct route Air_Segment -> RWY12 = 3s");
    finder.free_route(direct);
    check(direct.sectors == 0 && direct.sector_count == 0, "free_route resets the route");

    // Case 1
    cout << "-- Case 1: Alpha_Tree -> RWY12 --" << endl;
    route case1 = finder.find_fastest_route(alpha, rwy12);
    finder.print_route(case1);
    check(route_text(case1) == "Alpha_Tree Syntax_Sector Control_Tower Air_Segment RWY12" &&
          case1.total_weight == 11, "Case 1: fastest = 11s via Syntax_Sector");
    check(sweep.hop_distance(alpha, rwy12) == 3 && case1.sector_count - 1 == 4,
          "fewest hops (3, cost 12) is NOT the fastest route (4 hops, cost 11)");

    // next_hop along the Case 1 route
    check(finder.next_hop(case1, alpha) == syntax, "next_hop(Alpha_Tree) = Syntax_Sector");
    check(finder.next_hop(case1, north) == rwy12, "next_hop(Air_Segment) = RWY12");
    check(finder.next_hop(case1, rwy12) == -1, "next_hop at destination = -1");
    check(finder.next_hop(case1, rwy04) == -1, "next_hop for sector not on route = -1");
    route empty_route;
    check(finder.next_hop(empty_route, alpha) == -1, "next_hop on empty route = -1");
    check(finder.next_hop(string("Alpha_Tree"), string("RWY12")) == "Syntax_Sector",
          "engine form next_hop(name, runway) = Syntax_Sector");
    check(finder.next_hop(string("RWY12"), string("RWY12")) == "",
          "engine form returns \"\" when already at the runway");
    check(finder.next_hop(string("Ghost"), string("RWY12")) == "",
          "engine form returns \"\" for unknown sector");
    finder.free_route(case1);

    // Case 2
    cout << "-- Case 2: Restricted_Zone -> nearest emergency runway (RWY12 / RWY09) --" << endl;
    int emergency_runways[2] = {rwy12, rwy09};
    route case2 = finder.find_route_to_nearest(restricted, emergency_runways, 2);
    finder.print_route(case2);
    check(route_text(case2) == "Restricted_Zone Alpha_Tree RWY09" && case2.total_weight == 12,
          "Case 2: nearest emergency runway is RWY09 (12s) - RWY12 would be 14s");
    finder.free_route(case2);

    // Edge cases
    route same = finder.find_shortest_path(tower, tower);
    check(same.sector_count == 1 && same.total_weight == 0, "source == destination: 1 sector, 0s");
    check(finder.next_hop(same, tower) == -1, "next_hop when source == destination = -1");
    finder.free_route(same);
    route not_runway = finder.find_fastest_route(alpha, tower);
    check(not_runway.sectors == 0, "find_fastest_route rejects a non-runway destination");
    route bad_index = finder.find_fastest_route(-1, rwy12);
    route bad_index2 = finder.find_fastest_route(alpha, 77);
    check(bad_index.sectors == 0 && bad_index2.sectors == 0, "invalid indexes give empty route");
    finder.print_route(bad_index);

    // Case 3: cyber alert disables Syntax_Sector - Alpha_Tree
    cout << "-- Case 3: cyber alert disables Syntax_Sector <-> Alpha_Tree --" << endl;
    check(g.remove_corridor(string("Syntax_Sector"), string("Alpha_Tree")), "corridor disabled");
    check(g.get_weight(syntax, alpha) == 0 && g.get_list_weight(alpha, syntax) == 0 &&
          g.is_consistent(), "disabled corridor gone from BOTH matrix and list");
    route case3 = finder.find_fastest_route(alpha, rwy12);
    finder.print_route(case3);
    check(route_text(case3) == "Alpha_Tree Control_Tower Air_Segment RWY12" &&
          case3.total_weight == 12, "Case 3: Dijkstra reroutes via Control_Tower (12s)");
    finder.free_route(case3);

    cout << "-- Case 3b: Alpha_Tree <-> Restricted_Zone also disabled --" << endl;
    g.remove_corridor(alpha, restricted);
    route cut = finder.find_fastest_route(restricted, rwy09);
    finder.print_route(cut);
    check(cut.sectors == 0 && cut.sector_count == 0, "disconnected destination: no route");
    route cut_nearest = finder.find_route_to_nearest(restricted, emergency_runways, 2);
    check(cut_nearest.sectors == 0, "nearest-runway search also reports no route");
    check(finder.next_hop(string("Restricted_Zone"), string("RWY09")) == "",
          "engine next_hop returns \"\" when no route exists");

    route_finder broken(0);
    route nothing = broken.find_fastest_route(0, 1);
    check(nothing.sectors == 0, "null graph handled safely");
}

// ---------------------------------------------------------------------------
// 8. Integration with the real airspace.txt, file_loader and radar_mst
// ---------------------------------------------------------------------------

static void test_real_airspace() {
    section("8. Integration: airspace.txt + file_loader + radar_mst");
    space_graph g;
    file_loader loader;
    check(loader.load_airspace(g, "airspace.txt"), "airspace.txt loaded through file_loader");
    check(g.get_sector_count() == 10 && g.get_corridor_count() == 14,
          "10 sectors and 14 corridors loaded");
    check(g.is_consistent(), "loaded list and matrix consistent");
    int runways = 0;
    int i = 0;
    while (i < g.get_sector_count()) {
        if (g.is_runway(i)) {
            runways = runways + 1;
        }
        i = i + 1;
    }
    check(runways == 4, "4 runways flagged from the file's runway column");
    g.display_adjacency_matrix();

    radar_sweep sweep(&g);
    route_finder finder(&g);
    check(sweep.bfs_from(string("Control_Tower")) == 10, "BFS from Control_Tower reaches all 10");

    // Every sector the engine uses for missing flights must be found
    check(sweep.search_missing_flight(string("Control_Tower"), string("Restricted_Zone")) &&
          sweep.search_missing_flight(string("Control_Tower"), string("Syntax_Sector")) &&
          sweep.search_missing_flight(string("Control_Tower"), string("Cache_Fly")) &&
          sweep.search_missing_flight(string("Control_Tower"), string("Air_Segment")),
          "missing-flight search succeeds for MH370/PK404/FT739/GA542 sectors");

    route r1 = finder.find_fastest_route(string("Control_Tower"), string("RWY09"));
    finder.print_route(r1);
    check(route_text(r1) == "Control_Tower Air_Segment RWY09" && r1.total_weight == 6,
          "Control_Tower -> RWY09 = 6s via Air_Segment");
    delete[] r1.sectors;   // same release pattern game_engine uses

    int emergency_runways[2] = {g.get_sector_index("RWY12"), g.get_sector_index("RWY09")};
    route r2 = finder.find_route_to_nearest(g.get_sector_index("Restricted_Zone"),
                                            emergency_runways, 2);
    finder.print_route(r2);
    check(route_text(r2) == "Restricted_Zone Syntax_Sector RWY12" && r2.total_weight == 6,
          "Restricted_Zone nearest emergency runway = RWY12 (6s)");
    finder.free_route(r2);

    radar_mst mst(&g);
    mst.compute_mst();
    check(mst.get_mst_edge_count() == 9 && mst.get_mst_total_cost() == 23,
          "radar_mst (Person 3) builds a 9-edge MST of cost 23 from the new graph");

    // Exactly what game_engine::apply_escalation does on cyber timeout
    route before = finder.find_shortest_path(g.get_sector_index("Control_Tower"),
                                             g.get_sector_index("Cache_Fly"));
    check(before.total_weight == 3, "before cyber event: Control_Tower -> Cache_Fly = 3s");
    finder.free_route(before);
    check(g.remove_corridor("Control_Tower", "Cache_Fly"), "engine cyber removal call succeeds");
    check(g.is_consistent() && g.get_corridor_count() == 13, "13 corridors, still consistent");
    route after = finder.find_shortest_path(g.get_sector_index("Control_Tower"),
                                            g.get_sector_index("Cache_Fly"));
    finder.print_route(after);
    check(after.total_weight == 6, "after cyber event: Dijkstra detours (6s via Air_Segment)");
    finder.free_route(after);
    check(finder.next_hop(string("Cache_Fly"), string("RWY12")) == "Air_Segment",
          "engine next_hop still works after the removal");
    mst.compute_mst();
    check(mst.get_mst_edge_count() == 9, "MST recomputed on the modified graph (9 edges)");
}

// ---------------------------------------------------------------------------

int main() {
    cout << "MayDay-404 - Person 2 test runner (Modules 4-8)" << endl;
#ifdef LEAK_CHECKS_ENABLED
    // Report CRT diagnostics on stderr instead of a debugger window
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif
    // Warm up cout's lazily created number formatter BEFORE the first heap
    // checkpoint, so library-internal one-time allocations are not reported
    cout << "Test groups: " << 8 << endl;
    run_group("BST", test_bst);
    run_group("AVL", test_avl);
    run_group("Tree ownership", test_tree_ownership);
    run_group("Height demo", test_height_demo);
    run_group("SpaceGraph", test_graph);
    run_group("RadarSweep", test_radar_sweep);
    run_group("RouteFinder", test_route_finder);
    run_group("Real airspace", test_real_airspace);

    cout << endl << "========================================" << endl;
    cout << "Passed: " << tests_passed << "   Failed: " << tests_failed << endl;
    cout << "========================================" << endl;
    return tests_failed == 0 ? 0 : 1;
}
