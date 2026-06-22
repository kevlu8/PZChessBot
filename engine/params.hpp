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
TUNE(qs_see, -29, -100, 100, 10);
TUNE(qsfp_margin, 241, 100, 500, 25);
TUNE(rfp_threshold, 71, 20, 110, 10);
TUNE(rfp_improving, 45, 10, 50, 8);
TUNE(rfp_quad, 7, 1, 8, 1);
TUNE(rfp_cutnode, 30, 5, 30, 5);
TUNE(rfp_corr, 112, 20, 250, 20);
TUNE(nmp_margin, 184, 50, 400, 25);
TUNE(nmp_depth, 8, 0, 40, 5);
TUNE(razor_margin, 231, 150, 400, 20);
TUNE(probcut_margin, 409, 200, 500, 30);
TUNE(se_base, 6, 1, 10, 1);
TUNE(dext_base, 26, 10, 50, 5);
TUNE(dext_capt, 10, -100, 100, 10);
TUNE(dext_pv, 35, -100, 100, 10);
TUNE(dext_improving, 7, -50, 50, 10);
TUNE(text_base, 118, 50, 200, 10);
TUNE(text_capt, -61, -200, 200, 20);
TUNE(fp_const, 297, 100, 500, 30);
TUNE(fp_depth, 72, 20, 200, 10);
TUNE(fp_hist, 32, 10, 64, 5);
TUNE(history_margin, 2957, 1000, 4000, 256);
TUNE(see_quad, 17, 10, 40, 4);
TUNE(see_lin, 58, 20, 80, 6);
TUNE(lmr_base, -72, -1024, 1024, 160);
TUNE(lmr_capt, 57, 0, 128, 16);
TUNE(lmr_pv, 1316, 512, 2048, 128);
TUNE(lmr_cutnode, 2531, 0, 3072, 256);
TUNE(lmr_cutnode_nott, -653, -1024, 1024, 256);
TUNE(lmr_cutoffcnt, 561, 0, 2048, 128);
TUNE(lmr_ttpv, 834, 0, 2048, 128);
TUNE(lmr_killer, 974, 0, 2048, 128);
TUNE(lmr_hist, 12, 1, 40, 2);
TUNE(lmr_ttcapt, 754, 0, 2048, 128);
TUNE(lmr_ttpv_alpha, 70, -1024, 1024, 256);
TUNE(lmr_corr, 456, 0, 1024, 128);
TUNE(dodeeper_margin, 109, 20, 200, 10);
TUNE(doshallower_margin, 25, -64, 64, 8);
TUNE(hist_large_margin, 101, 0, 256, 16);
TUNE(hist_quad, 4, 0, 6, 1);
TUNE(hist_lin, 139, 64, 256, 15);
TUNE(hist_const, 124, 0, 256, 16);
TUNE(postlmr_quad, 3, 0, 6, 1);
TUNE(postlmr_lin, 126, 64, 256, 15);
TUNE(postlmr_const, 112, 0, 256, 16);
TUNE(asp_window, 13, 1, 30, 4);
TUNE(corr_ps, 157, 64, 256, 12);
TUNE(corr_np, 155, 64, 256, 12);
TUNE(corr_maj, 74, 32, 128, 8);
TUNE(corr_min, 78, 32, 128, 8);
TUNE(corr_cont, 150, 64, 256, 16);
TUNE(corr_cont2, 126, 64, 256, 16);
TUNE(corr_threat, 90, 64, 256, 16);
TUNE(probcut_see, 62, 50, 200, 10);
TUNE(badnoisy_div, 47, 10, 100, 8);

TUNE(lmr_a, 99, 20, 200, 10);
TUNE(lmr_b, 260, 100, 400, 20);

TUNE(tm_soft, 53, 10, 90, 8);
TUNE(cmplx_div, 192, 100, 400, 40);
TUNE(cmplx_base, 64, 20, 120, 10);
TUNE(cmplx_mul, 88, 40, 160, 10);
TUNE(bm_base, 180, 80, 250, 15);
TUNE(bm_mul, 20, 10, 80, 10);
TUNE(node_base, 134, 100, 200, 10);
TUNE(node_mul, 92, 40, 160, 10);
