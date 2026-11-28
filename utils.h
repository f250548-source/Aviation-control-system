#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <string>
using namespace std;

// Shared helpers (no <algorithm>, no STL containers)

// Swap two integers in place
void swap_values(int& a, int& b);

// Swap two strings in place (no string::swap / std::swap)
void swap_strings(string& a, string& b);

// Return the smaller of two integers
int min_value(int a, int b);

// Return the larger of two integers
int max_value(int a, int b);

// Read an integer in [min_value_bound, max_value_bound] with validation
int read_int(string prompt, int min_value_bound, int max_value_bound);

// Read a non-empty player name with no spaces
string read_name();

// Clear failed cin state and discard rest of line
void clear_input_buffer();

// Print a boxed end-of-game summary
void print_result_box(string player_name, int credibility, int ticks,
                      int flights_landed, int emergencies_resolved,
                      string outcome, string failure_cause);

// Print a common aviation radio call
void print_aviation_call(string call_text);

#endif
