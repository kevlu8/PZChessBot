#pragma once

#include <string>
#include <vector>
#include <functional>
#include <iostream>

// Forward declarations
void register_tunable(const std::string& name, int* value_ptr, int initial, int min, int max, int increment, double learning_rate);

// Structure to hold tunable parameter information
struct TunableParam {
    std::string name;
    int* value_ptr;
    int initial;
    int min;
    int max;
    int increment;
    double learning_rate;
    
    TunableParam(const std::string& n, int* ptr, int init, int mn, int mx, int inc, double lr)
        : name(n), value_ptr(ptr), initial(init), min(mn), max(mx), increment(inc), learning_rate(lr) {
        *value_ptr = init; // Set initial value
    }
};

// Global registry of tunable parameters (using function to ensure initialization)
std::vector<TunableParam>& get_tunable_params();

// Auto-registration helper
struct TunableRegistrar {
    TunableRegistrar(const std::string& name, int* value_ptr, int initial, int min, int max, int increment, double learning_rate) {
		int expected_inc = (max - min) / 20;
		if (increment != expected_inc) {
			std::cerr << "Warning: Increment for tunable parameter '" << name << "' is unusual (" << increment << "), expected " << expected_inc << std::endl;
			increment = expected_inc; // im lazy
		}
        register_tunable(name, value_ptr, initial, min, max, increment, learning_rate);
    }
};

// Functions for managing tunables
void print_tunables();
bool set_tunable_value(const std::string& name, int value);
void init_tunables();

// Macro for easy tunable parameter definition
#define TUNE(param, initial, min, max, increment) \
    int param = initial; \
    static TunableRegistrar param##_registrar(#param, &param, initial, min, max, increment, 0.002);
#define AUTOTUNE(param, initial, min, max) \
	int param = initial; \
	static TunableRegistrar param##_registrar(#param, &param, initial, min, max, (max - min) / 20, 0.002);