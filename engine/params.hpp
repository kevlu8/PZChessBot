#pragma once

#include <iostream>
#include <string>
#include <list>

struct TunableParam {
    std::string name;
    int value, default_value, min, max, step;

    void print_id() {
        std::cout << name << ",int," << default_value << "," << min << "," << max << "," << step << ",0.002" << std::endl;
    }

    void print_uci() {
        std::cout << "option name " << name << " type spin default " << default_value
                  << " min " << min << " max " << max << std::endl;
    }
};

TunableParam &add_tunable(std::string name, int default_value, int min, int max, int step);

void print_uci();
void print_config();
void handle_set(std::string optionname, std::string optionvalue);

#define TUNE(name, val, min, max, step) \
    TunableParam &name##Param = add_tunable(#name, val, min, max, step); \
    int &name = name##Param.value;

// Tunables go below
