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
TUNE(qs_see, -10, -100, 100, 10);
TUNE(qsfp_see, 153, 50, 600, 50);
TUNE(qsfp_margin, 346, 100, 500, 25);
TUNE(rfp_threshold, 58, 20, 110, 10);
TUNE(rfp_improving, 31, 10, 50, 8);
TUNE(rfp_quad, 5, 1, 8, 1);
TUNE(rfp_cutnode, 21, 5, 30, 5);
TUNE(nmp_margin, 202, 50, 400, 25);
TUNE(nmp_depth, 17, 0, 40, 5);
TUNE(razor_margin, 228, 150, 400, 20);
TUNE(probcut_margin, 349, 200, 500, 30);
TUNE(dext_base, 22, 10, 50, 5);
TUNE(dext_capt, 2, -100, 100, 10);
TUNE(dext_pv, 11, -100, 100, 10);
TUNE(dext_improving, -2, -50, 50, 10);
TUNE(text_base, 92, 50, 200, 10);
TUNE(text_capt, -5, -200, 200, 20);
TUNE(fp_const, 284, 100, 500, 30);
TUNE(fp_depth, 96, 20, 200, 10);
TUNE(fp_hist, 32, 10, 64, 5);
TUNE(history_margin, 2990, 1000, 4000, 256);
TUNE(see_quad, 25, 10, 40, 4);
TUNE(see_lin, 58, 20, 80, 6);
TUNE(lmr_base, 163, -1024, 1024, 160);
TUNE(lmr_pv, 1055, 512, 2048, 128);
TUNE(lmr_cutnode, 1940, 0, 3072, 256);
TUNE(lmr_cutnode_nott, -303, -1024, 1024, 256);
TUNE(lmr_cutoffcnt, 714, 0, 2048, 128);
TUNE(lmr_ttpv, 959, 0, 2048, 128);
TUNE(lmr_killer, 717, 0, 2048, 128);
TUNE(lmr_hist, 12, 1, 16, 2);
TUNE(lmr_ttcapt, 1019, 0, 2048, 128);
TUNE(lmr_ttpv_alpha, -84, -1024, 1024, 256);
TUNE(dodeeper_margin, 91, 20, 200, 10);
TUNE(doshallower_margin, -8, -64, 64, 8);
TUNE(hist_large_margin, 100, 0, 256, 16);
TUNE(hist_quad, 3, 0, 6, 1);
TUNE(hist_lin, 122, 64, 256, 15);
TUNE(hist_const, 120, 0, 256, 16);
TUNE(asp_window, 14, 1, 30, 4);
TUNE(bm_base, 182, 80, 250, 15);
TUNE(bm_mul, 33, 10, 80, 10);
TUNE(corr_ps, 133, 64, 256, 12);
TUNE(corr_np, 139, 64, 256, 12);
TUNE(corr_maj, 62, 32, 128, 8);
TUNE(corr_min, 73, 32, 128, 8);
TUNE(corr_cont, 132, 64, 256, 16);
TUNE(corr_cont2, 157, 64, 256, 16);
TUNE(probcut_see, 102, 50, 200, 10);
TUNE(badnoisy_div, 51, 10, 100, 8);