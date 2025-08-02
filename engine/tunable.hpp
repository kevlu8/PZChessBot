#pragma once

#include <string>
#include <unordered_map>

/**
 * SPSA Tunable Parameters System
 * 
 * This system allows tuning of search parameters via UCI setoption commands.
 * Parameters marked as tunable can be adjusted during engine testing.
 */

struct TunableParam {
    int value;
    int default_value;
    int min_value;
    int max_value;
    std::string name;
    
    TunableParam(int def, int min, int max, const std::string& n) 
        : value(def), default_value(def), min_value(min), max_value(max), name(n) {}
};

// Global tunable parameters registry
extern std::unordered_map<std::string, TunableParam*> tunable_params;

// Macro to define a tunable parameter
#define TUNABLE_PARAM(name, default_val, min_val, max_val) \
    TunableParam name(default_val, min_val, max_val, #name); \
    static int __register_##name = (tunable_params[#name] = &name, 0);

// Function declarations
void init_tunable_params();
void set_tunable_param(const std::string& name, int value);
void print_tunable_options();
void reset_tunable_params();
void reinit_lmr_table(); // Reinitialize LMR table when parameters change

// Search parameters
extern TunableParam RFP_THRESHOLD_PARAM;
extern TunableParam RFP_IMPROVING_PARAM;
extern TunableParam ASPIRATION_WINDOW_PARAM;
extern TunableParam NMP_R_VALUE_PARAM;
extern TunableParam DELTA_THRESHOLD_PARAM;
extern TunableParam FUTILITY_THRESHOLD_PARAM;
extern TunableParam FUTILITY_THRESHOLD2_PARAM;
extern TunableParam RAZOR_MARGIN_PARAM;
extern TunableParam HISTORY_MARGIN_PARAM;

// Move ordering parameters
extern TunableParam COUNTER_MOVE_BONUS_PARAM;
extern TunableParam KILLER_BONUS_PARAM;
extern TunableParam SECOND_KILLER_BONUS_PARAM;

// Late move reductions
extern TunableParam LMR_BASE_PARAM;
extern TunableParam LMR_DIVISOR_PARAM;

// PVS reductions
extern TunableParam PV_REDUCTION_PARAM;
extern TunableParam CAPTURE_REDUCTION_PARAM;

// Macros for easy access to parameter values
#define RFP_THRESHOLD (RFP_THRESHOLD_PARAM.value * CP_SCALE_FACTOR)
#define RFP_IMPROVING (RFP_IMPROVING_PARAM.value * CP_SCALE_FACTOR)
#define ASPIRATION_WINDOW (ASPIRATION_WINDOW_PARAM.value * CP_SCALE_FACTOR)
#define NMP_R_VALUE (NMP_R_VALUE_PARAM.value)
#define DELTA_THRESHOLD (DELTA_THRESHOLD_PARAM.value * CP_SCALE_FACTOR)
#define FUTILITY_THRESHOLD (FUTILITY_THRESHOLD_PARAM.value * CP_SCALE_FACTOR)
#define FUTILITY_THRESHOLD2 (FUTILITY_THRESHOLD2_PARAM.value * CP_SCALE_FACTOR)
#define RAZOR_MARGIN (RAZOR_MARGIN_PARAM.value * CP_SCALE_FACTOR)
#define HISTORY_MARGIN (HISTORY_MARGIN_PARAM.value * CP_SCALE_FACTOR)

#define COUNTER_MOVE_BONUS (COUNTER_MOVE_BONUS_PARAM.value)
#define KILLER_BONUS (KILLER_BONUS_PARAM.value)
#define SECOND_KILLER_BONUS (SECOND_KILLER_BONUS_PARAM.value)

#define LMR_BASE (LMR_BASE_PARAM.value)
#define LMR_DIVISOR (LMR_DIVISOR_PARAM.value)

#define PV_REDUCTION (PV_REDUCTION_PARAM.value)
#define CAPTURE_REDUCTION (CAPTURE_REDUCTION_PARAM.value)
