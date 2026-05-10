#pragma once

#include "includes.hpp"

#include "bitboard.hpp"

#include "../Pyrrhic/tbprobe.h"

struct TBManager {
	bool initialized = false;
	int min_depth = 1;
	int max_pieces = 7;
	std::string path = "";

	TBManager() = default;
	TBManager(std::string path) {
		init(path);
	}

	void init(std::string path) {
		if (tb_init(path.c_str())) {
			initialized = true;
			this->path = path;
		} else {
			std::cerr << "Failed to init tablebase with path: " << path << std::endl;
		}
	}

	void destroy() {
		if (initialized) {
			tb_free();
			initialized = false;
			min_depth = 1;
			max_pieces = 7;
			path = "";
		}
	}

	/**
	 * Probes the tablebase for a given position.
	 * 
	 * 1 is a POV win, -1 is a POV loss, and 0 is a draw
	 */
	std::optional<int> probe_pos(Position &pos);

	/**
	 * Probes the tablebase for a given position and returns the viable moves
	 * (i.e. the ones that are not different than the optimal result).
	 */
	std::unordered_set<uint16_t> probe_moves(Position &pos, bool rep = false);
};

extern TBManager tbman;
