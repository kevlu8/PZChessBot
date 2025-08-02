#include "tunable.hpp"
#include "eval.hpp"
#include "search.hpp"
#include "includes.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>

// Global registry for tunable parameters
std::unordered_map<std::string, TunableParam*> tunable_params;

// Define all tunable parameters with reasonable default values and ranges

// Search parameters (from search.hpp constants, converted to centipawns base)
TunableParam RFP_THRESHOLD_PARAM(131, 50, 300, "RFP_THRESHOLD");
TunableParam RFP_IMPROVING_PARAM(30, 10, 100, "RFP_IMPROVING");
TunableParam ASPIRATION_WINDOW_PARAM(32, 10, 100, "ASPIRATION_WINDOW");
TunableParam NMP_R_VALUE_PARAM(4, 2, 6, "NMP_R_VALUE");
TunableParam DELTA_THRESHOLD_PARAM(300, 100, 600, "DELTA_THRESHOLD");
TunableParam FUTILITY_THRESHOLD_PARAM(304, 100, 600, "FUTILITY_THRESHOLD");
TunableParam FUTILITY_THRESHOLD2_PARAM(692, 300, 1200, "FUTILITY_THRESHOLD2");
TunableParam RAZOR_MARGIN_PARAM(241, 100, 500, "RAZOR_MARGIN");
TunableParam HISTORY_MARGIN_PARAM(4000, 2000, 8000, "HISTORY_MARGIN");

// Move ordering parameters
TunableParam COUNTER_MOVE_BONUS_PARAM(1021, 500, 2000, "COUNTER_MOVE_BONUS");
TunableParam KILLER_BONUS_PARAM(1500, 800, 3000, "KILLER_BONUS");
TunableParam SECOND_KILLER_BONUS_PARAM(800, 400, 1600, "SECOND_KILLER_BONUS");

// LMR parameters (scaled by 100 to avoid floats)
TunableParam LMR_BASE_PARAM(77, 40, 120, "LMR_BASE");          // was 0.77, now 77/100
TunableParam LMR_DIVISOR_PARAM(236, 150, 400, "LMR_DIVISOR");  // was 2.36, now 236/100

// PVS reductions (scaled by 512 like in original code)
TunableParam PV_REDUCTION_PARAM(512, 256, 1024, "PV_REDUCTION");        // PV reduction amount
TunableParam CAPTURE_REDUCTION_PARAM(800, 400, 1600, "CAPTURE_REDUCTION"); // Extra reduction for captures

void init_tunable_params() {
    // Register all parameters
    tunable_params["RFP_THRESHOLD"] = &RFP_THRESHOLD_PARAM;
    tunable_params["RFP_IMPROVING"] = &RFP_IMPROVING_PARAM;
    tunable_params["ASPIRATION_WINDOW"] = &ASPIRATION_WINDOW_PARAM;
    tunable_params["NMP_R_VALUE"] = &NMP_R_VALUE_PARAM;
    tunable_params["DELTA_THRESHOLD"] = &DELTA_THRESHOLD_PARAM;
    tunable_params["FUTILITY_THRESHOLD"] = &FUTILITY_THRESHOLD_PARAM;
    tunable_params["FUTILITY_THRESHOLD2"] = &FUTILITY_THRESHOLD2_PARAM;
    tunable_params["RAZOR_MARGIN"] = &RAZOR_MARGIN_PARAM;
    tunable_params["HISTORY_MARGIN"] = &HISTORY_MARGIN_PARAM;
    
    tunable_params["COUNTER_MOVE_BONUS"] = &COUNTER_MOVE_BONUS_PARAM;
    tunable_params["KILLER_BONUS"] = &KILLER_BONUS_PARAM;
    tunable_params["SECOND_KILLER_BONUS"] = &SECOND_KILLER_BONUS_PARAM;
    
    tunable_params["LMR_BASE"] = &LMR_BASE_PARAM;
    tunable_params["LMR_DIVISOR"] = &LMR_DIVISOR_PARAM;
    
    tunable_params["PV_REDUCTION"] = &PV_REDUCTION_PARAM;
    tunable_params["CAPTURE_REDUCTION"] = &CAPTURE_REDUCTION_PARAM;
}

void set_tunable_param(const std::string& name, int value) {
    auto it = tunable_params.find(name);
    if (it != tunable_params.end()) {
        TunableParam* param = it->second;
        if (value >= param->min_value && value <= param->max_value) {
            param->value = value;
            std::cout << "info string Set " << name << " to " << value << std::endl;
            
            // Reinitialize LMR table if LMR parameters changed
            if (name == "LMR_BASE" || name == "LMR_DIVISOR") {
                reinit_lmr_table();
            }
        } else {
            std::cout << "info string Parameter " << name << " value " << value 
                     << " is out of range [" << param->min_value << ", " << param->max_value << "]" << std::endl;
        }
    } else {
        std::cout << "info string Unknown tunable parameter: " << name << std::endl;
    }
}

// Forward declare the reduction table from search.cpp
extern uint16_t reduction[250][MAX_PLY];

void reinit_lmr_table() {
    for (int i = 0; i < 250; i++) {
        for (int d = 0; d < MAX_PLY; d++) {
            if (d <= 1 || i <= 1) reduction[i][d] = 1024;
            else reduction[i][d] = (LMR_BASE / 100.0 + log2(i) * log2(d) / (LMR_DIVISOR / 100.0)) * 1024;
        }
    }
    std::cout << "info string LMR table reinitialized with new parameters" << std::endl;
}

void print_tunable_options() {
    for (const auto& [name, param] : tunable_params) {
        std::cout << "option name " << name << " type spin default " 
                 << param->default_value << " min " << param->min_value 
                 << " max " << param->max_value << std::endl;
    }
}

void reset_tunable_params() {
    for (const auto& [name, param] : tunable_params) {
        param->value = param->default_value;
    }
    std::cout << "info string All tunable parameters reset to defaults" << std::endl;
}
