#include "params.hpp"

std::list<TunableParam> &tunables() {
    static std::list<TunableParam> tunables;
    return tunables;
}

TunableParam &add_tunable(std::string name, int default_value, int min, int max, int step) {
    tunables().push_back({name, default_value, default_value, min, max, step});
    return tunables().back();
}

void print_uci() {
    for (auto &tp : tunables()) {
        tp.print_uci();
    }
}

void print_config() {
    for (auto &tp : tunables()) {
        tp.print_id();
    }
}

void handle_set(std::string optionname, std::string optionvalue) {
    for (auto &tp : tunables()) {
        if (tp.name == optionname) {
            int val = std::stoi(optionvalue);
            if (val < tp.min || val > tp.max) {
                std::cerr << "Invalid value for " << optionname << ": " << val << std::endl;
                return;
            }
            tp.value = val;
            return;
        }
    }
    std::cerr << "Unknown option: " << optionname << std::endl;
}
