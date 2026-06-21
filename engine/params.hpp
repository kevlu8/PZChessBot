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

#pragma once

#include <iostream>
#include <string>
#include <list>

#define TUNING

struct TunableParam {
    std::string name;
    int value, default_value, min, max, step;

    void print_id() {
        #ifdef TUNING
        std::cout << name << ",int," << default_value << "," << min << "," << max << "," << step << ",0.002" << std::endl;
        #endif
    }

    void print_uci() {
        #ifdef TUNING
        std::cout << "option name " << name << " type spin default " << default_value
                  << " min " << min << " max " << max << std::endl;
        #endif
    }
};

TunableParam &add_tunable(std::string name, int default_value, int min, int max, int step);

void print_uci();
void print_config();
void handle_set(std::string optionname, std::string optionvalue);

#define TUNE(name, val, min, max, step) \
    inline TunableParam &name##Param = add_tunable(#name, val, min, max, step); \
    inline int name() { return name##Param.value; }

// Tunables go below
TUNE(qs_see, -14, -100, 100, 10);
TUNE(qsfp_margin, 205, 100, 500, 25);
TUNE(rfp_threshold, 59, 20, 110, 10);
TUNE(rfp_improving, 31, 10, 50, 8);
TUNE(rfp_quad, 5, 1, 8, 1);
TUNE(rfp_cutnode, 15, 5, 30, 5);
TUNE(rfp_corr, 100, 20, 250, 20);
TUNE(nmp_margin, 214, 50, 400, 25);
TUNE(nmp_depth, 13, 0, 40, 5);
TUNE(razor_margin, 227, 150, 400, 20);
TUNE(probcut_margin, 377, 200, 500, 30);
TUNE(se_base, 6, 1, 10, 1);
TUNE(dext_base, 24, 10, 50, 5);
TUNE(dext_capt, -2, -100, 100, 10);
TUNE(dext_pv, 16, -100, 100, 10);
TUNE(dext_improving, 12, -50, 50, 10);
TUNE(text_base, 111, 50, 200, 10);
TUNE(text_capt, -11, -200, 200, 20);
TUNE(fp_const, 282, 100, 500, 30);
TUNE(fp_depth, 92, 20, 200, 10);
TUNE(fp_hist, 30, 10, 64, 5);
TUNE(history_margin, 3014, 1000, 4000, 256);
TUNE(see_quad, 17, 10, 40, 4);
TUNE(see_lin, 60, 20, 80, 6);
TUNE(lmr_base, 223, -1024, 1024, 160);
TUNE(lmr_capt, 64, 0, 128, 16);
TUNE(lmr_pv, 1109, 512, 2048, 128);
TUNE(lmr_cutnode, 2286, 0, 3072, 256);
TUNE(lmr_cutnode_nott, -643, -1024, 1024, 256);
TUNE(lmr_cutoffcnt, 835, 0, 2048, 128);
TUNE(lmr_ttpv, 896, 0, 2048, 128);
TUNE(lmr_killer, 835, 0, 2048, 128);
TUNE(lmr_hist, 12, 1, 40, 2);
TUNE(lmr_ttcapt, 914, 0, 2048, 128);
TUNE(lmr_ttpv_alpha, -34, -1024, 1024, 256);
TUNE(lmr_corr, 400, 0, 1024, 128);
TUNE(dodeeper_margin, 109, 20, 200, 10);
TUNE(doshallower_margin, -3, -64, 64, 8);
TUNE(hist_large_margin, 96, 0, 256, 16);
TUNE(hist_quad, 3, 0, 6, 1);
TUNE(hist_lin, 131, 64, 256, 15);
TUNE(hist_const, 144, 0, 256, 16);
TUNE(postlmr_quad, 3, 0, 6, 1);
TUNE(postlmr_lin, 131, 64, 256, 15);
TUNE(postlmr_const, 144, 0, 256, 16);
TUNE(asp_window, 14, 1, 30, 4);
TUNE(bm_base, 180, 80, 250, 15);
TUNE(bm_mul, 20, 10, 80, 10);
TUNE(corr_ps, 128, 64, 256, 12);
TUNE(corr_np, 141, 64, 256, 12);
TUNE(corr_maj, 63, 32, 128, 8);
TUNE(corr_min, 84, 32, 128, 8);
TUNE(corr_cont, 137, 64, 256, 16);
TUNE(corr_cont2, 143, 64, 256, 16);
TUNE(corr_threat, 103, 64, 256, 16);
TUNE(probcut_see, 84, 50, 200, 10);
TUNE(badnoisy_div, 48, 10, 100, 8);

TUNE(lmr_a, 74, 20, 200, 10);
TUNE(lmr_b, 233, 100, 400, 20);
