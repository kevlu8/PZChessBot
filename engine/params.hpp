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

// #define TUNING

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
TUNE(qs_see, -15, -100, 100, 10);
TUNE(qsfp_margin, 300, 100, 500, 25);
TUNE(rfp_threshold, 65, 20, 110, 10);
TUNE(rfp_improving, 43, 10, 50, 8);
TUNE(rfp_quad, 217, 1, 320, 16); // quantized to 32
TUNE(rfp_cutnode, 29, 5, 30, 5);
TUNE(rfp_corr, 118, 20, 250, 20);
TUNE(nmp_margin, 218, 50, 400, 20);
TUNE(nmp_depth, 12, 0, 25, 3);
TUNE(razor_margin, 217, 150, 400, 20);
TUNE(probcut_margin, 406, 200, 500, 30);
TUNE(se_base, 65, 1, 160, 16); // quantized to 32
TUNE(dext_base, 22, 10, 50, 5);
TUNE(dext_capt, 8, 0, 100, 5);
TUNE(dext_pv, 37, 0, 100, 5);
TUNE(text_base, 120, 50, 200, 10);
TUNE(text_capt, 32, 0, 200, 10);
TUNE(fp_const, 369, 100, 500, 30);
TUNE(fp_depth, 90, 20, 200, 10);
TUNE(fp_hist, 29, 10, 64, 5);
TUNE(history_margin, 3275, 1000, 4000, 256);
TUNE(see_quad, 14, 0, 40, 4);
TUNE(see_lin, 59, 20, 80, 6);
TUNE(lmr_base, -70, -1024, 1024, 160);
TUNE(lmr_capt, 69, 0, 128, 16);
TUNE(lmr_pv, 1344, 512, 2048, 128);
TUNE(lmr_cutnode, 2480, 0, 3072, 256);
TUNE(lmr_cutnode_nott, -795, -1024, 1024, 256);
TUNE(lmr_cutoffcnt, 709, 0, 2048, 128);
TUNE(lmr_ttpv, 830, 0, 2048, 128);
TUNE(lmr_killer, 1149, 0, 2048, 128);
TUNE(lmr_hist, 11, 1, 40, 4);
TUNE(lmr_ttcapt, 865, 0, 2048, 128);
TUNE(lmr_ttpv_alpha, 362, -1024, 1024, 256);
TUNE(lmr_corr, 219, 0, 1024, 128);
TUNE(dodeeper_margin, 113, 20, 200, 10);
TUNE(doshallower_margin, 28, -64, 64, 8);
TUNE(hist_large_margin, 119, 0, 256, 16);
TUNE(hist_quad, 135, 0, 224, 16); // quantized to 32
TUNE(hist_lin, 141, 64, 256, 16);
TUNE(hist_const, 154, 0, 256, 16);
TUNE(postlmr_quad, 76, 0, 224, 16); // quantized to 32
TUNE(postlmr_lin, 148, 64, 256, 16);
TUNE(postlmr_const, 135, 0, 256, 16);
TUNE(asp_window, 10, 1, 30, 4);
TUNE(corr_ps, 166, 64, 256, 12);
TUNE(corr_np, 171, 64, 256, 12);
TUNE(corr_maj, 71, 32, 128, 8);
TUNE(corr_min, 72, 32, 128, 8);
TUNE(corr_cont, 181, 64, 256, 16);
TUNE(corr_cont2, 159, 64, 256, 16);
TUNE(corr_threat, 89, 64, 256, 8);
TUNE(probcut_see, 57, 0, 200, 10);
TUNE(badnoisy_div, 55, 10, 100, 8);

TUNE(lmr_a, 108, 20, 200, 10);
TUNE(lmr_b, 243, 100, 400, 20);

TUNE(tm_rem, 25, 10, 40, 3);
TUNE(tm_inc, 91, 60, 100, 5);
TUNE(tm_soft, 51, 10, 90, 4);
TUNE(bm_base, 182, 80, 250, 8);
TUNE(bm_mul, 24, 10, 80, 4);
TUNE(node_base, 129, 100, 200, 6);
TUNE(node_mul, 102, 40, 160, 6);
