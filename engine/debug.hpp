#pragma once

#include <iostream>
#include <map>
#include <string>
#include <limits>
#include <algorithm>
#include <vector>
#include <cstdlib>

/**
 * Simple debugging utilities for tracking variable statistics across program execution.
 * 
 * Usage:
 *   dbg_extreme_of(my_variable);  // Tracks min/max of my_variable
 *   dbg_mean_of(my_variable);     // Tracks mean of my_variable
 *   dbg_stats_of(my_variable);    // Tracks both min/max and mean
 * 
 * Example:
 *   int search_depth = 5;
 *   dbg_extreme_of(search_depth);  // Will track min/max search_depth values
 *   
 *   double eval_score = 1.23;
 *   dbg_mean_of(eval_score);       // Will track mean of eval_score values
 *   
 *   int node_count = 1000;
 *   dbg_stats_of(node_count);      // Will track both extremes and mean
 */

namespace Debug {
    template<typename T>
    struct Stats {
        T min_val = std::numeric_limits<T>::max();
        T max_val = std::numeric_limits<T>::lowest();
        double sum = 0.0;
        size_t count = 0;
        std::string name;
        
        void update(T value) {
            min_val = std::min(min_val, value);
            max_val = std::max(max_val, value);
            sum += static_cast<double>(value);
            count++;
        }
        
        double mean() const {
            return count > 0 ? sum / count : 0.0;
        }
    };

    extern std::map<std::string, Stats<int>> int_extremes;
    extern std::map<std::string, Stats<double>> double_extremes;
    extern std::map<std::string, Stats<int>> int_means;
    extern std::map<std::string, Stats<double>> double_means;

    extern bool initialized;

    void print_debug_stats();

    void init();

    template<typename T>
    void track_extreme(const std::string& name, T value);

    template<typename T>
    void track_mean(const std::string& name, T value);

    template<>
    inline void track_extreme<int>(const std::string& name, int value) {
        if (!initialized) init();
        auto& stats = int_extremes[name];
        stats.name = name;
        stats.update(value);
    }

    template<>
    inline void track_extreme<double>(const std::string& name, double value) {
        if (!initialized) init();
        auto& stats = double_extremes[name];
        stats.name = name;
        stats.update(value);
    }

    template<>
    inline void track_mean<int>(const std::string& name, int value) {
        if (!initialized) init();
        auto& stats = int_means[name];
        stats.name = name;
        stats.update(value);
    }

    template<>
    inline void track_mean<double>(const std::string& name, double value) {
        if (!initialized) init();
        auto& stats = double_means[name];
        stats.name = name;
        stats.update(value);
    }
}

#define dbg_extreme_of(var) Debug::track_extreme(#var, (var))
#define dbg_mean_of(var) Debug::track_mean(#var, (var))
#define dbg_stats_of(var) do { dbg_extreme_of(var); dbg_mean_of(var); } while(0)

namespace Debug {
    inline std::map<std::string, Stats<int>> int_extremes;
    inline std::map<std::string, Stats<double>> double_extremes;
    inline std::map<std::string, Stats<int>> int_means;
    inline std::map<std::string, Stats<double>> double_means;
    inline bool initialized = false;

    inline void print_debug_stats() {
        std::cerr << "\n=== Debug Statistics ===\n";

        if (!int_extremes.empty() || !double_extremes.empty()) {
            std::cerr << "Min/Max Values:\n";

            for (const auto& [name, stats] : int_extremes) {
                std::cerr << "  " << name << ": min=" << stats.min_val 
                         << ", max=" << stats.max_val << " (tracked " << stats.count << " times)\n";
            }

            for (const auto& [name, stats] : double_extremes) {
                std::cerr << "  " << name << ": min=" << std::fixed << stats.min_val 
                         << ", max=" << stats.max_val << " (tracked " << stats.count << " times)\n";
            }
        }

        if (!int_means.empty() || !double_means.empty()) {
            std::cerr << "Mean Values:\n";

            for (const auto& [name, stats] : int_means) {
                std::cerr << "  " << name << ": mean=" << std::fixed << stats.mean()
                         << " (tracked " << stats.count << " times)\n";
            }

            for (const auto& [name, stats] : double_means) {
                std::cerr << "  " << name << ": mean=" << std::fixed << stats.mean()
                         << " (tracked " << stats.count << " times)\n";
            }
        }

        std::cerr << "======================\n";
    }

    inline void init() {
        if (!initialized) {
            std::atexit(print_debug_stats);
            initialized = true;
        }
    }
}
