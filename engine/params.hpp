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

#ifdef TUNING

#define TUNE(name, val, min, max, step) \
    inline TunableParam &name##Param = add_tunable(#name, val, min, max, step); \
    inline int name() { return name##Param.value; }

#else

#define TUNE(name, val, min, max, step) \
    inline int name() { return val; }

#endif

// Tunables go below
TUNE(qs_see, -16, -100, 100, 10);
TUNE(qsfp_margin, 297, 100, 500, 25);
TUNE(rfp_threshold, 62, 20, 110, 10);
TUNE(rfp_improving, 43, 10, 50, 8);
TUNE(rfp_quad, 216, 1, 320, 16); // quantized to 32
TUNE(rfp_cutnode, 28, 5, 30, 5);
TUNE(rfp_corr, 106, 20, 250, 20);
TUNE(nmp_margin, 217, 50, 400, 20);
TUNE(nmp_depth, 13, 0, 25, 3);
TUNE(razor_margin, 216, 150, 400, 20);
TUNE(probcut_margin, 405, 200, 500, 30);
TUNE(se_base, 69, 1, 160, 16); // quantized to 32
TUNE(dext_base, 22, 10, 50, 5);
TUNE(dext_capt, 7, 0, 100, 5);
TUNE(dext_pv, 40, 0, 100, 5);
TUNE(text_base, 122, 50, 200, 10);
TUNE(text_capt, 37, 0, 200, 10);
TUNE(fp_const, 368, 100, 500, 30);
TUNE(fp_depth, 93, 20, 200, 10);
TUNE(fp_hist, 27, 10, 64, 5);
TUNE(history_margin, 3236, 1000, 4000, 256);
TUNE(see_quad, 15, 0, 40, 4);
TUNE(see_lin, 62, 20, 80, 6);
TUNE(see_hist, 67, 32, 128, 6);
TUNE(lmr_base, -9, -1024, 1024, 160);
TUNE(lmr_capt, 65, 0, 128, 16);
TUNE(lmr_pv, 1325, 512, 2048, 128);
TUNE(lmr_cutnode, 2128, 0, 3072, 256);
TUNE(lmr_cutnode_nott, 1016, 0, 2048, 128);
TUNE(lmr_cutoffcnt, 701, 0, 2048, 128);
TUNE(lmr_ttpv, 871, 0, 2048, 128);
TUNE(lmr_killer, 1177, 0, 2048, 128);
TUNE(lmr_hist, 7, 1, 40, 4);
TUNE(lmr_capt_hist, 10, 1, 30, 3);
TUNE(lmr_ttcapt, 872, 0, 2048, 128);
TUNE(lmr_ttpv_alpha, 333, -1024, 1024, 256);
TUNE(lmr_corr, 233, 0, 1024, 128);
TUNE(dodeeper_margin, 117, 20, 200, 10);
TUNE(doshallower_margin, 28, -64, 64, 8);
TUNE(hist_large_margin, 113, 0, 256, 16);
TUNE(hist_quad, 140, 0, 224, 16); // quantized to 32
TUNE(hist_lin, 141, 64, 256, 16);
TUNE(hist_const, 150, 0, 256, 16);
TUNE(postlmr_quad, 80, 0, 224, 16); // quantized to 32
TUNE(postlmr_lin, 147, 64, 256, 16);
TUNE(postlmr_const, 136, 0, 256, 16);
TUNE(asp_window, 9, 1, 30, 4);
TUNE(corr_ps, 166, 64, 256, 12);
TUNE(corr_np, 173, 64, 256, 12);
TUNE(corr_maj, 73, 32, 128, 8);
TUNE(corr_min, 71, 32, 128, 8);
TUNE(corr_cont, 186, 64, 256, 16);
TUNE(corr_cont2, 169, 64, 256, 16);
TUNE(corr_threat, 86, 64, 256, 8);
TUNE(probcut_see, 58, 0, 200, 10);
TUNE(badnoisy_div, 56, 10, 100, 8);

TUNE(lmr_a, 108, 20, 200, 10);
TUNE(lmr_b, 247, 100, 400, 20);

TUNE(tm_rem, 25, 10, 40, 3);
TUNE(tm_inc, 91, 60, 100, 5);
TUNE(tm_soft, 51, 10, 90, 4);
TUNE(bm_base, 182, 80, 250, 8);
TUNE(bm_mul, 24, 10, 80, 4);
TUNE(node_base, 129, 100, 200, 6);
TUNE(node_mul, 102, 40, 160, 6);
