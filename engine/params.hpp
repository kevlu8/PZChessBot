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
TUNE(qs_see, -2, -50, 50, 8);
TUNE(delta, 348, 0, 500, 25);
TUNE(qsfp_see, 4754, 1024, 8192, 256);
TUNE(rfp, 62, 0, 200, 5);
TUNE(rfp_improving, 36, 0, 100, 5);
TUNE(rfp_quad, 5, 0, 10, 1);
TUNE(rfp_cutnode, 15, 0, 50, 3);
TUNE(razor, 267, 0, 500, 25);
TUNE(probcut, 300, 0, 1000, 50);
TUNE(dext, 26, 0, 50, 4);
TUNE(fut, 300, 0, 600, 40);
TUNE(fut_depth, 100, 0, 250, 20);
TUNE(hist_marg, 2633, 0, 5000, 250);
TUNE(see_quad, 25, 0, 100, 5);
TUNE(see_lin, 67, 0, 200, 10);
TUNE(lmr_a, 77, 50, 150, 5);
TUNE(lmr_b, 236, 100, 350, 10);
TUNE(lmr_base, 0, -1024, 1024, 256);
TUNE(lmr_pv, 1024, 0, 2048, 256);
TUNE(lmr_cutnode, 1024, 0, 2048, 256);
TUNE(lmr_cutoff, 1024, 0, 2048, 256);
TUNE(lmr_ttpv, 1024, 0, 2048, 256);
TUNE(lmr_killer, 1024, 0, 2048, 256);
TUNE(lmr_hist, 8, 1, 16, 1);
TUNE(asp, 24, 1, 100, 5);
TUNE(pawn_corr, 117, 0, 256, 16);
TUNE(nonpawn_corr, 130, 0, 256, 16);
TUNE(major_corr, 64, 0, 128, 8);
TUNE(minor_corr, 64, 0, 128, 8);
TUNE(cont_corr, 128, 0, 256, 16);