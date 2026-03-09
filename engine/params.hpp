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
    inline TunableParam &name##Param = add_tunable(#name, val, min, max, step); \
    inline int name() { return name##Param.value; }

// Tunables go below
TUNE(qs_see, -12, -100, 100, 10);
TUNE(qsfp_see, 168, 50, 600, 50);
TUNE(qsfp_margin, 355, 100, 500, 25);
TUNE(rfp_threshold, 60, 20, 110, 10);
TUNE(rfp_improving, 31, 10, 50, 8);
TUNE(rfp_quad, 5, 1, 8, 1);
TUNE(rfp_cutnode, 18, 5, 30, 5);
TUNE(nmp_margin, 200, 50, 400, 25);
TUNE(nmp_depth, 20, 0, 40, 5);
TUNE(razor_margin, 244, 150, 400, 20);
TUNE(probcut_margin, 370, 200, 500, 30);
TUNE(dext_base, 20, 10, 50, 5);
TUNE(dext_capt, 0, -100, 100, 10);
TUNE(dext_pv, 0, -100, 100, 10);
TUNE(dext_improving, 0, -50, 50, 10);
TUNE(text_base, 100, 50, 200, 10);
TUNE(text_capt, 0, -200, 200, 20);
TUNE(fp_const, 307, 100, 500, 30);
TUNE(fp_depth, 100, 20, 200, 10);
TUNE(fp_hist, 28, 10, 64, 5);
TUNE(history_margin, 2753, 1000, 4000, 256);
TUNE(see_quad, 23, 10, 40, 4);
TUNE(see_lin, 55, 20, 80, 6);
TUNE(lmr_base, 270, -1024, 1024, 160);
TUNE(lmr_pv, 1095, 512, 2048, 128);
TUNE(lmr_cutnode, 1673, 0, 3072, 256);
TUNE(lmr_cutnode_nott, -265, -1024, 1024, 256);
TUNE(lmr_cutoffcnt, 798, 0, 2048, 128);
TUNE(lmr_ttpv, 869, 0, 2048, 128);
TUNE(lmr_killer, 816, 0, 2048, 128);
TUNE(lmr_hist, 10, 1, 16, 2);
TUNE(lmr_ttcapt, 1024, 0, 2048, 128);
TUNE(lmr_ttpv_alpha, 0, -1024, 1024, 256);
TUNE(dodeeper_margin, 100, 20, 200, 10);
TUNE(doshallower_margin, -3, -64, 64, 8);
TUNE(hist_quad, 2, 0, 6, 1);
TUNE(hist_lin, 123, 64, 256, 15);
TUNE(hist_const, 130, 0, 256, 16);
TUNE(asp_window, 16, 1, 30, 4);
TUNE(bm_base, 180, 80, 250, 15);
TUNE(bm_mul, 40, 10, 80, 10);
TUNE(corr_ps, 125, 64, 256, 12);
TUNE(corr_np, 138, 64, 256, 12);
TUNE(corr_maj, 72, 32, 128, 8);
TUNE(corr_min, 70, 32, 128, 8);
TUNE(corr_cont, 134, 64, 256, 16);
TUNE(corr_cont2, 134, 64, 256, 16);
TUNE(probcut_see, 97, 50, 200, 10);
TUNE(badnoisy_div, 46, 10, 100, 8);