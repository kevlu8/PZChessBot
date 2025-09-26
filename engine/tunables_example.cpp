/*
 * Example usage of the tunables system
 * 
 * This file demonstrates how to use the TUNE macro to create tunable parameters.
 * To use tunables in your code:
 * 
 * 1. Include tunables.hpp
 * 2. Use TUNE(name, initial, min, max, increment, learning_rate) to declare a tunable parameter
 * 3. The parameter becomes a regular int variable that can be used anywhere
 * 4. Parameters are automatically registered and can be changed via UCI setoption
 * 
 * Example usage in a search file:
 * 
 * #include "tunables.hpp"
 * 
 * // Declare tunable parameters at global scope
 * TUNE(NullMovePruningDepth, 3, 1, 10, 1, 0.1);
 * TUNE(LateMovePruningCount, 4, 1, 20, 1, 0.05);
 * TUNE(AspWindowSize, 50, 10, 200, 5, 0.02);
 * 
 * void some_search_function() {
 *     if (depth >= NullMovePruningDepth) {
 *         // Apply null move pruning
 *     }
 *     
 *     if (move_count > LateMovePruningCount) {
 *         // Apply late move pruning  
 *     }
 *     
 *     int window = AspWindowSize;
 *     // Use aspiration window
 * }
 * 
 * On program startup, this will print:
 * NullMovePruningDepth, int, 3, 1, 10, 1, 0.1
 * LateMovePruningCount, int, 4, 1, 20, 1, 0.05  
 * AspWindowSize, int, 50, 10, 200, 5, 0.02
 * 
 * And parameters can be changed via UCI:
 * setoption name NullMovePruningDepth value 5
 * setoption name LateMovePruningCount value 8
 */

// This file contains example tunable parameters for demonstration
// Remove this file and add your actual tunables to your search/eval files

#include "tunables.hpp"

// Example tunable parameters
// TUNE(ExampleParam1, 100, 50, 200, 10, 0.1);
// TUNE(ExampleParam2, 25, 1, 100, 5, 0.05);

// To use these parameters, just reference them by name:
// if (some_condition && value > ExampleParam1) { ... }