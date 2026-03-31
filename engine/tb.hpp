#pragma once

#include "includes.hpp"

#include "bitboard.hpp"

#include "../Pyrrhic/tbprobe.h"

struct TBManager {
	bool initialized = false;
	int min_depth = 1;
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
	 * 
	 * Not yet implemented.
	 */
	// std::unordered_set<Move> probe_moves(Position &pos);
};

extern TBManager tbman;
