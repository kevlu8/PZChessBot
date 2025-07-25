#pragma once

#include <string>
#include <unordered_map>
#include <iostream>

struct TunableParam {
    int value;
    int default_value;
    int min_value;
    int max_value;
    std::string name;
    
    TunableParam(int def, int min_val, int max_val, const std::string& param_name)
        : value(def), default_value(def), min_value(min_val), max_value(max_val), name(param_name) {}
    
    void set(int new_value) {
        if (new_value >= min_value && new_value <= max_value) {
            value = new_value;
        }
    }
    
    operator int() const { return value; }
};

class TunableManager {
public:
    static TunableManager& instance() {
        static TunableManager instance;
        return instance;
    }
    
    void register_param(const std::string& name, TunableParam* param) {
        params[name] = param;
    }
    
    bool set_param(const std::string& name, int value) {
        auto it = params.find(name);
        if (it != params.end()) {
            it->second->set(value);
            return true;
        }
        return false;
    }
    
    void print_uci_options() {
        for (const auto& pair : params) {
            const TunableParam* param = pair.second;
            std::cout << "option name " << param->name 
                     << " type spin default " << param->default_value
                     << " min " << param->min_value
                     << " max " << param->max_value << std::endl;
        }
    }
    
    void reset_all() {
        for (const auto& pair : params) {
            pair.second->value = pair.second->default_value;
        }
    }
    
private:
    std::unordered_map<std::string, TunableParam*> params;
};

#define DECLARE_TUNABLE(name, default_val, min_val, max_val) \
    extern TunableParam name;

#define DEFINE_TUNABLE(name, default_val, min_val, max_val) \
    TunableParam name(default_val, min_val, max_val, #name);

void register_all_tunables();

// Declare all tunable parameters
DECLARE_TUNABLE(Killer1_Bonus, 1461, 500, 3000);
DECLARE_TUNABLE(Killer2_Bonus, 831, 300, 2000);
DECLARE_TUNABLE(Counter_Bonus, 1003, 300, 2500);
DECLARE_TUNABLE(RFP_Threshold, 131, 50, 300);
DECLARE_TUNABLE(Aspiration_Window, 36, 10, 100);
DECLARE_TUNABLE(NMP_Reduction, 4, 2, 6);
DECLARE_TUNABLE(Futility_Threshold1, 312, 100, 800);
DECLARE_TUNABLE(Futility_Threshold2, 678, 300, 1500);
DECLARE_TUNABLE(Razor_Margin, 250, 100, 600);
DECLARE_TUNABLE(History_Bonus_A, 153, 50, 300);    // 1.53 * 100
DECLARE_TUNABLE(History_Bonus_B, 87, 30, 200);     // 0.87 * 100
DECLARE_TUNABLE(History_Bonus_C, 65, 10, 150);     // 0.65 * 100
DECLARE_TUNABLE(Capture_Bonus_A, 182, 50, 400);    // 1.82 * 100
DECLARE_TUNABLE(Capture_Bonus_B, 49, 10, 150);     // 0.49 * 100
DECLARE_TUNABLE(Capture_Bonus_C, 39, 5, 100);      // 0.39 * 100
DECLARE_TUNABLE(Time_Factor, 52, 30, 80);          // 0.52 * 100
DECLARE_TUNABLE(IIR_Depth, 4, 2, 8);               // Internal Iterative Reduction depth threshold
DECLARE_TUNABLE(IIR_Reduction, 2, 1, 4);           // Internal Iterative Reduction depth reduction
DECLARE_TUNABLE(Check_Extension, 1, 0, 2);         // Check extension depth increase
