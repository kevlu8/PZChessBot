#include "tunables.hpp"
#include <iostream>
#include <algorithm>

// Global registry of tunable parameters (using function to ensure initialization)
std::vector<TunableParam>& get_tunable_params() {
    static std::vector<TunableParam> tunable_params;
    return tunable_params;
}

void register_tunable(const std::string& name, int* value_ptr, int initial, int min, int max, int increment, double learning_rate) {
    get_tunable_params().emplace_back(name, value_ptr, initial, min, max, increment, learning_rate);
}

void print_tunables() {
    auto& params = get_tunable_params();
    for (const auto& param : params) {
        std::cout << param.name << ", int, " << param.initial << ", " 
                  << param.min << ", " << param.max << ", " << param.increment 
                  << ", " << param.learning_rate << std::endl;
    }
}

bool set_tunable_value(const std::string& name, int value) {
    auto& params = get_tunable_params();
    auto it = std::find_if(params.begin(), params.end(),
                          [&name](const TunableParam& param) {
                              return param.name == name;
                          });
    
    if (it != params.end()) {
        // Clamp value to valid range
        int clamped_value = std::max(it->min, std::min(it->max, value));
        *(it->value_ptr) = clamped_value;
        return true;
    }
    return false;
}

void init_tunables() {
    // This function can be called to ensure all tunables are properly initialized
    // Currently just a placeholder, but could be extended for loading from config files
}

