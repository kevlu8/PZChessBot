/*
 * PZChessBot, a UCI chess engine
 * Copyright (C) 2026 Kevin Lu and William Ma
 *
 * PZChessBot is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PZChessBot is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PZChessBot. If not, see <https://www.gnu.org/licenses/>.
 */

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
