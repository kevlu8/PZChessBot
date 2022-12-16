#include "moves.hpp"

inline bool is_enemy(char piece, bool side) { return piece != 0 && ((piece > 6 && side) || (piece < 7 && !side)); }

void pawn_control(const bool side, const int i, char *controlled, const int material) {
	if (side) {
		if (i % 8 != 0) {
			controlled[i + 7] += material;
		}
		if (i % 8 != 7) {
			controlled[i + 9] += material;
		}
	} else {
		if (i % 8 != 0) {
			controlled[i - 9] += material;
		}
		if (i % 8 != 7) {
			controlled[i - 7] += material;
		}
	}
}

void knight_control(const int i, char *controlled, const int material) {
	if (i / 8 != 0) {
		if (i % 8 > 1)
			controlled[i - 10] += material;
		if (i % 8 < 6)
			controlled[i - 6] += material;
		if (i / 8 != 1) {
			if (i % 8 != 0)
				controlled[i - 17] += material;
			if (i % 8 != 7)
				controlled[i - 15] += material;
		}
	}
	if (i / 8 != 7) {
		if (i % 8 > 1)
			controlled[i + 6] += material;
		if (i % 8 < 6)
			controlled[i + 10] += material;
		if (i / 8 != 6) {
			if (i % 8 != 0)
				controlled[i + 15] += material;
			if (i % 8 != 7)
				controlled[i + 17] += material;
		}
	}
}

void bishop_control(const char *position, const int i, const bool side, char *controlled, const int material) {
	int j = i;
	while (j / 8 != 0 && j % 8 != 0) {
		j -= 9;
		controlled[j] += material;
		if (is_enemy(position[j], side) || (position[j] != 0 && position[j] != 3 && position[j] != 9 && position[j] != 5 && position[j] != 11))
			break;
	}
	j = i;
	while (j / 8 != 0 && j % 8 != 7) {
		j -= 7;
		controlled[j] += material;
		if (is_enemy(position[j], side) || (position[j] != 0 && position[j] != 3 && position[j] != 9 && position[j] != 5 && position[j] != 11))
			break;
	}
	j = i;
	while (j / 8 != 7 && j % 8 != 0) {
		j += 7;
		controlled[j] += material;
		if (is_enemy(position[j], side) || (position[j] != 0 && position[j] != 3 && position[j] != 9 && position[j] != 5 && position[j] != 11))
			break;
	}
	j = i;
	while (j / 8 != 7 && j % 8 != 7) {
		j += 9;
		controlled[j] += material;
		if (is_enemy(position[j], side) || (position[j] != 0 && position[j] != 3 && position[j] != 9 && position[j] != 5 && position[j] != 11))
			break;
	}
}

void rook_control(const char *position, const int i, const bool side, char *controlled, const int material) {
	int j = i;
	while (j / 8 != 0) {
		j -= 8;
		controlled[j] += material;
		if (is_enemy(position[j], side) || (position[j] != 0 && position[j] != 4 && position[j] != 10 && position[j] != 5 && position[j] != 11))
			break;
	}
	j = i;
	while (j % 8 != 0) {
		j--;
		controlled[j] += material;
		if (is_enemy(position[j], side) || (position[j] != 0 && position[j] != 4 && position[j] != 10 && position[j] != 5 && position[j] != 11))
			break;
	}
	j = i;
	while (j % 8 != 7) {
		j++;
		controlled[j] += material;
		if (is_enemy(position[j], side) || (position[j] != 0 && position[j] != 4 && position[j] != 10 && position[j] != 5 && position[j] != 11))
			break;
	}
	j = i;
	while (j / 8 != 7) {
		j += 8;
		controlled[j] += material;
		if (is_enemy(position[j], side) || (position[j] != 0 && position[j] != 4 && position[j] != 10 && position[j] != 5 && position[j] != 11))
			break;
	}
}

void king_control(const int i, char *controlled, const int material) {
	if (i % 8 != 0) // if going left doesnt fall off the edge
		controlled[i - 1] += material;
	if (i % 8 != 7) // if going right doesnt fall off the edge
		controlled[i + 1] += material;
	if (i / 8 != 0) { // if you can go down
		controlled[i - 8] += material; // go down
		if (i % 8 != 0) // if going down and left doesnt fall off the edge
			controlled[i - 9] += material;
		if (i % 8 != 7) // if going down and right doesnt fall off the edge
			controlled[i - 7] += material;
	}
	if (i / 8 != 7) { // if you can go up
		controlled[i + 8]++;
		if (i % 8 != 0) // if going up and left doesnt fall off the edge
			controlled[i + 7] += material;
		if (i % 8 != 7) // if going up and right doesnt fall off the edge
			controlled[i + 9] += material;
	}
}

void controlled_squares(const char *position, const bool side, char *controlled, const bool material) noexcept {
	std::fill(controlled, controlled + 64, 0);
	for (int i = 0; i < 64; i++) {
		if (position[i] == 0)
			continue;
		if (is_enemy(position[i], side))
			continue;

		switch (position[i]) {
		case 1:
		case 7:
			pawn_control(side, i, controlled, material ? 1 : 1);
			break;
		case 2:
		case 8:
			knight_control(i, controlled, material ? 3 : 1);
			break;
		case 3:
		case 9:
			bishop_control(position, i, side, controlled, material ? 3 : 1);
			break;
		case 4:
		case 10:
			rook_control(position, i, side, controlled, material ? 5 : 1);
			break;
		case 5:
		case 11:
			bishop_control(position, i, side, controlled, material ? 9 : 1);
			rook_control(position, i, side, controlled, material ? 9 : 1);
			break;
		case 6:
		case 12:
			king_control(i, controlled, material ? 0 : 1);
			break;
		}
	}
}

void pawn_moves(const char *position, const bool side, const int i, const std::string &prev, std::vector<std::string> *moves) noexcept {
	std::string move = "";

	if (side) {
		if (i / 8 == 1) { // second rank
			if (position[i + 8] == 0) {
				// push back in uci
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8) + '3';
				moves->push_back(move);
				move = "";
				if (position[i + 16] == 0) {
					move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8) + '4';
					moves->push_back(move);
					move = "";
				}
			}
			if (i % 8 && is_enemy(position[i + 7], side)) {
				move = move + ((char)('a' + i % 8)) + (char)(i / 8 + 1 + '0') + (char)('a' + i % 8 - 1) + (char)(i / 8 + 2 + '0');
				moves->push_back(move);
				move = "";
			}
			if (i % 8 != 7 && is_enemy(position[i + 9], side)) {
				move = move + ((char)('a' + i % 8)) + (char)(i / 8 + 1 + '0') + (char)('a' + i % 8 + 1) + (char)(i / 8 + 2 + '0');
				moves->push_back(move);
				move = "";
			}
		} else if (i / 8 != 6) {
			if (position[i + 8] == 0) {
				move = move + ((char)('a' + i % 8)) + (char)(i / 8 + 1 + '0') + (char)('a' + i % 8) + (char)(i / 8 + 2 + '0');
				moves->push_back(move);
				move = "";
			}
			if (i % 8 && is_enemy(position[i + 7], side)) {
				move = move + ((char)('a' + i % 8)) + (char)(i / 8 + 1 + '0') + (char)('a' + i % 8 - 1) + (char)(i / 8 + 2 + '0');
				moves->push_back(move);
				move = "";
			}
			if (i % 8 != 7 && is_enemy(position[i + 9], side)) {
				move = move + ((char)('a' + i % 8)) + (char)(i / 8 + 1 + '0') + (char)('a' + i % 8 + 1) + (char)(i / 8 + 2 + '0');
				moves->push_back(move);
				move = "";
			}
		} else {
			if (position[i + 8] == 0) {
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8) + '8' + 'q';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8) + '8' + 'r';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8) + '8' + 'b';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8) + '8' + 'n';
				moves->push_back(move);
				move = "";
			}
			if (i % 8 != 0 && is_enemy(position[i + 7], side)) {
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8 - 1) + '8' + 'q';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8 - 1) + '8' + 'r';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8 - 1) + '8' + 'b';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8 - 1) + '8' + 'n';
				moves->push_back(move);
				move = "";
			}
			if (i % 8 != 7 && is_enemy(position[i + 9], side)) {
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8 + 1) + '8' + 'q';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8 + 1) + '8' + 'r';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8 + 1) + '8' + 'b';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8 + 1) + '8' + 'n';
				moves->push_back(move);
				move = "";
			}
		}
		if (i / 8 == 4 && prev[3] == '5' && prev[1] == '7') { // en passant
			if (abs(prev[2] - 'a' - i % 8) == 1) {
				move = move + ((char)('a' + i % 8)) + '5' + prev[0] + '6';
				moves->push_back(move);
				move = "";
			}
		}
	} else {
		if (i / 8 + 1 == 7) { // seventh rank
			if (position[i - 8] == 0) {
				// push back in uci
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8) + '6';
				moves->push_back(move);
				move = "";
				if (position[i - 16] == 0) {
					move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8) + '5';
					moves->push_back(move);
					move = "";
				}
			}
			if (i % 8 && is_enemy(position[i - 9], side)) {
				move = move + ((char)('a' + i % 8)) + (char)(i / 8 + 1 + '0') + (char)('a' + i % 8 - 1) + (char)(i / 8 + '0');
				moves->push_back(move);
				move = "";
			}
			if (i % 8 != 7 && is_enemy(position[i - 7], side)) {
				move = move + ((char)('a' + i % 8)) + (char)(i / 8 + 1 + '0') + (char)('a' + i % 8 + 1) + (char)(i / 8 + '0');
				moves->push_back(move);
				move = "";
			}
		} else if (i / 8 != 1) {
			if (position[i - 8] == 0) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8) + std::to_string(i / 8);
				moves->push_back(move);
				move = "";
			}
			if (i % 8 && is_enemy(position[i - 9], side)) {
				move = move + ((char)('a' + i % 8)) + (char)(i / 8 + 1 + '0') + (char)('a' + i % 8 - 1) + (char)(i / 8 + '0');
				moves->push_back(move);
				move = "";
			}
			if (i % 8 != 7 && is_enemy(position[i - 7], side)) {
				move = move + ((char)('a' + i % 8)) + (char)(i / 8 + 1 + '0') + (char)('a' + i % 8 + 1) + (char)(i / 8 + '0');
				moves->push_back(move);
				move = "";
			}
		} else {
			if (position[i - 8] == 0) {
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8) + '1' + 'q';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8) + '1' + 'r';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8) + '1' + 'b';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8) + '1' + 'n';
				moves->push_back(move);
				move = "";
			}
			if (i % 8 && is_enemy(position[i - 9], side)) {
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8 - 1) + '1' + 'q';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8 - 1) + '1' + 'r';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8 - 1) + '1' + 'b';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8 - 1) + '1' + 'n';
				moves->push_back(move);
				move = "";
			}
			if (i % 8 != 7 && is_enemy(position[i - 7], side)) {
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8 + 1) + '1' + 'q';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8 + 1) + '1' + 'r';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8 + 1) + '1' + 'b';
				moves->push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8 + 1) + '1' + 'n';
				moves->push_back(move);
				move = "";
			}
		}
		if (i / 8 == 3 && prev[3] == '4' && prev[1] == '2') { // en passant
			if (abs(prev[2] - 'a' - i % 8) == 1) {
				move = move + ((char)('a' + i % 8)) + '4' + prev[0] + '3';
				moves->push_back(move);
				move = "";
			}
		}
	}
}

void knight_moves(const char *position, const bool side, const int i, std::vector<std::string> *moves) noexcept {
	// man i hate knights
	std::string move;

	if (i % 8 >= 1) { // can move 1 left
		if (i / 8 >= 2) { // can move 2 down
			if (position[i - 17] == 0 || is_enemy(position[i - 17], side)) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - 1) + std::to_string(i / 8 - 1);
				moves->push_back(move);
				move = "";
			}
		}
		if (i / 8 <= 5) { // can move 2 up
			if (position[i + 15] == 0 || is_enemy(position[i + 15], side)) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - 1) + std::to_string(i / 8 + 3);
				moves->push_back(move);
				move = "";
			}
		}
		if (i % 8 >= 2) { // can move 2 left
			if (i / 8 >= 1) { // can move 1 down
				if (position[i - 10] == 0 || is_enemy(position[i - 10], side)) {
					move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - 2) + std::to_string(i / 8);
					moves->push_back(move);
					move = "";
				}
			}
			if (i / 8 <= 6) { // can move 1 up
				if (position[i + 6] == 0 || is_enemy(position[i + 6], side)) {
					move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - 2) + std::to_string(i / 8 + 2);
					moves->push_back(move);
					move = "";
				}
			}
		}
	}
	if (i % 8 <= 6) { // can move 1 right
		if (i / 8 >= 2) { // can move 2 down
			if (position[i - 15] == 0 || is_enemy(position[i - 15], side)) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + 1) + std::to_string(i / 8 - 1);
				moves->push_back(move);
				move = "";
			}
		}
		if (i / 8 <= 5) { // can move 2 up
			if (position[i + 17] == 0 || is_enemy(position[i + 17], side)) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + 1) + std::to_string(i / 8 + 3);
				moves->push_back(move);
				move = "";
			}
		}
		if (i % 8 <= 5) { // can move 2 right
			if (i / 8 >= 1) { // can move 1 down
				if (position[i - 6] == 0 || is_enemy(position[i - 6], side)) {
					move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + 2) + std::to_string(i / 8);
					moves->push_back(move);
					move = "";
				}
			}
			if (i / 8 <= 6) { // can move 1 up
				if (position[i + 10] == 0 || is_enemy(position[i + 10], side)) {
					move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + 2) + std::to_string(i / 8 + 2);
					moves->push_back(move);
					move = "";
				}
			}
		}
	}
}

void bishop_moves(const char *position, const bool side, const int i, std::vector<std::string> *moves) noexcept {
	std::string move;

	for (int j = 1; j <= std::min(i % 8, i / 8); j++) {
		if (position[i - 9 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - j) + std::to_string(i / 8 + 1 - j);
			moves->push_back(move);
			move = "";
		} else if (is_enemy(position[i - 9 * j], side)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - j) + std::to_string(i / 8 + 1 - j);
			moves->push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
	for (int j = 1; j <= std::min(7 - i % 8, i / 8); j++) {
		if (position[i - 7 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + j) + std::to_string(i / 8 + 1 - j);
			moves->push_back(move);
			move = "";
		} else if (is_enemy(position[i - 7 * j], side)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + j) + std::to_string(i / 8 + 1 - j);
			moves->push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
	for (int j = 1; j <= std::min(7 - i % 8, 7 - i / 8); j++) {
		if (position[i + 9 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + j) + std::to_string(i / 8 + 1 + j);
			moves->push_back(move);
			move = "";
		} else if (is_enemy(position[i + 9 * j], side)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + j) + std::to_string(i / 8 + 1 + j);
			moves->push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
	for (int j = 1; j <= std::min(i % 8, 7 - i / 8); j++) {
		if (position[i + 7 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - j) + std::to_string(i / 8 + 1 + j);
			moves->push_back(move);
			move = "";
		} else if (is_enemy(position[i + 7 * j], side)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - j) + std::to_string(i / 8 + 1 + j);
			moves->push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
}

void rook_moves(const char *position, const bool side, const int i, std::vector<std::string> *moves) noexcept {
	std::string move;

	for (int j = 1; j <= i % 8; j++) {
		if (position[i - j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - j) + std::to_string(i / 8 + 1);
			moves->push_back(move);
			move = "";
		} else if (is_enemy(position[i - 1 * j], side)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - j) + std::to_string(i / 8 + 1);
			moves->push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
	for (int j = 1; j <= 7 - i % 8; j++) {
		if (position[i + j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + j) + std::to_string(i / 8 + 1);
			moves->push_back(move);
			move = "";
		} else if (is_enemy(position[i + 1 * j], side)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + j) + std::to_string(i / 8 + 1);
			moves->push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
	for (int j = 1; j <= i / 8; j++) {
		if (position[i - 8 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8) + std::to_string(i / 8 + 1 - j);
			moves->push_back(move);
			move = "";
		} else if (is_enemy(position[i - 8 * j], side)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8) + std::to_string(i / 8 + 1 - j);
			moves->push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
	for (int j = 1; j <= 7 - i / 8; j++) {
		if (position[i + 8 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8) + std::to_string(i / 8 + 1 + j);
			moves->push_back(move);
			move = "";
		} else if (is_enemy(position[i + 8 * j], side)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8) + std::to_string(i / 8 + 1 + j);
			moves->push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
}

void king_moves(const char *position, const bool side, const int i, const char castling, std::vector<std::string> *moves) noexcept {
	std::string move = "";
	// can't move into check
	// can't take a piece that is defended
	// can't castle through or into check
	// can't castle if king or rook has moved

	char controlled[64];
	controlled_squares(position, !side, controlled, false); 

	if (i % 8) { // there is space on the left
		if ((position[i - 1] == 0 || is_enemy(position[i - 1], side)) && controlled[i - 1] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - 1) + std::to_string(i / 8 + 1);
			moves->push_back(move);
			move = "";
		}
		if (i / 8 > 0) { // there is space on the bottom
			if ((position[i - 9] == 0 || is_enemy(position[i - 9], side)) && controlled[i - 9] == 0) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - 1) + std::to_string(i / 8);
				moves->push_back(move);
				move = "";
			}
		}
		if (i / 8 < 7) { // there is space on the top
			if ((position[i + 7] == 0 || is_enemy(position[i + 7], side)) && controlled[i + 7] == 0) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - 1) + std::to_string(i / 8 + 2);
				moves->push_back(move);
				move = "";
			}
		}
	}
	if (i % 8 < 7) { // if there is space on the right
		if ((position[i + 1] == 0 || is_enemy(position[i + 1], side)) && controlled[i + 1] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + 1) + std::to_string(i / 8 + 1);
			moves->push_back(move);
			move = "";
		}
		if (i / 8 > 0) { // there is space on the bottom
			if ((position[i - 7] == 0 || is_enemy(position[i - 7], side)) && controlled[i - 7] == 0) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + 1) + std::to_string(i / 8);
				moves->push_back(move);
				move = "";
			}
		}
		if (i / 8 < 7) { // there is space on the top
			if ((position[i + 9] == 0 || is_enemy(position[i + 9], side)) && controlled[i + 9] == 0) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + 1) + std::to_string(i / 8 + 2);
				moves->push_back(move);
				move = "";
			}
		}
	}
	if (i / 8 > 0) { // if there is space on the bottom
		if ((position[i - 8] == 0 || is_enemy(position[i - 8], side)) && controlled[i - 8] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8) + std::to_string(i / 8);
			moves->push_back(move);
			move = "";
		}
	}
	if (i / 8 < 7) { // if there is space on the top
		if ((position[i + 8] == 0 || is_enemy(position[i + 8], side)) && controlled[i + 8] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8) + std::to_string(i / 8 + 2);
			moves->push_back(move);
			move = "";
		}
	}

	// castling
	if (side) {
		if (castling & 0b1000) { // white kingside
			if (position[7] == 4) { // rook is still there
				if (controlled[4] + controlled[5] + controlled[6] + position[5] + position[6] == 0) { // not moving through check or other pieces
					move = "e1g1";
					moves->push_back(move);
					move = "";
				}
			}
		}
		if (castling & 0b0100) { // white queenside
			if (position[0] == 4) { // rook is still there
				if (controlled[4] + controlled[3] + controlled[2] + position[3] + position[2] + position[1] == 0) { // not moving through check or other pieces
					move = "e1c1";
					moves->push_back(move);
					move = "";
				}
			}
		}
	} else {
		if (castling & 0b0010) { // black kingside
			if (position[63] == 10) { // rook is still there
				if (controlled[60] + controlled[61] + controlled[62] + position[61] + position[62] == 0) { // not moving through check or other pieces
					move = "e8g8";
					moves->push_back(move);
					move = "";
				}
			}
		}
		if (castling & 0b0001) { // black queenside
			if (position[56] == 10) { // rook is still there
				if (controlled[60] + controlled[59] + controlled[58] + position[59] + position[58] + position[57] == 0) { // not moving through check or other pieces
					move = "e8c8";
					moves->push_back(move);
					move = "";
				}
			}
		}
	}
}

std::vector<std::string> *find_legal_moves(const char *position, const std::string &prev, const char *metadata) noexcept {
	std::vector<std::string> *moves = new std::vector<std::string>;
	moves->reserve(300);

	for (int i = 0; i < 64; i++) {
		if (position[i] == 0)
			continue;
		// each element in the board array will be a number from 0 to 12
		// 0 = empty
		// 1 = white pawn
		// 2 = white knight
		// 3 = white bishop
		// 4 = white rook
		// 5 = white queen
		// 6 = white king
		// 7 = black pawn
		// 8 = black knight
		// 9 = black bishop
		// 10 = black rook
		// 11 = black queen
		// 12 = black king

		if (metadata[0]) {
			if (position[i] == 1) {
				pawn_moves(position, metadata[0], i, prev, moves);
			} else if (position[i] == 2) {
				knight_moves(position, metadata[0], i, moves);
			} else if (position[i] == 3) {
				bishop_moves(position, metadata[0], i, moves);
			} else if (position[i] == 4) {
				rook_moves(position, metadata[0], i, moves);
			} else if (position[i] == 5) {
				bishop_moves(position, metadata[0], i, moves);
				rook_moves(position, metadata[0], i, moves);
			} else if (position[i] == 6) {
				king_moves(position, metadata[0], i, metadata[1], moves);
			}
		} else {
			if (position[i] == 7) {
				pawn_moves(position, metadata[0], i, prev, moves);
			} else if (position[i] == 8) {
				knight_moves(position, metadata[0], i, moves);
			} else if (position[i] == 9) {
				bishop_moves(position, metadata[0], i, moves);
			} else if (position[i] == 10) {
				rook_moves(position, metadata[0], i, moves);
			} else if (position[i] == 11) {
				bishop_moves(position, metadata[0], i, moves);
				rook_moves(position, metadata[0], i, moves);
			} else if (position[i] == 12) {
				king_moves(position, metadata[0], i, metadata[1], moves);
			}
		}
	}
	return moves;
}

void make_move(const std::string &move, const std::string &prev, char *board, char *meta) noexcept {
	if (move.size() == 5) { // promotion
		board[move[0] - 'a' + (move[1] - '1') * 8] = 0;
		if (move[4] == 'q') {
			if (meta[0]) {
				board[move[2] - 'a' + (move[3] - '1') * 8] = 5;
			} else {
				board[move[2] - 'a' + (move[3] - '1') * 8] = 11;
			}
		} else if (move[4] == 'r') {
			if (meta[0]) {
				board[move[2] - 'a' + (move[3] - '1') * 8] = 4;
			} else {
				board[move[2] - 'a' + (move[3] - '1') * 8] = 10;
			}
		} else if (move[4] == 'b') {
			if (meta[0]) {
				board[move[2] - 'a' + (move[3] - '1') * 8] = 3;
			} else {
				board[move[2] - 'a' + (move[3] - '1') * 8] = 9;
			}
		} else if (move[4] == 'n') {
			if (meta[0]) {
				board[move[2] - 'a' + (move[3] - '1') * 8] = 2;
			} else {
				board[move[2] - 'a' + (move[3] - '1') * 8] = 8;
			}
		}
	} else if (board[move[0] - 'a' + (move[1] - '1') * 8] == 1 || board[move[0] - 'a' + (move[1] - '1') * 8] == 7) { // a pawn
		if (abs(prev[1] - prev[3]) == 2 && (board[prev[0] - 'a' + (move[1] - '1') * 8] == 1 || board[prev[0] - 'a' + (move[1] - '1') * 8] == 7)) { // previous move was also a pawn
			if (abs(prev[0] - move[0]) == 1 && prev[2] == move[2] && (prev[3] - prev[1]) / 2 == prev[3] - move[3]) { // en passant
				board[move[0] - 'a' + (move[1] - '1') * 8] = 0;
				board[move[0] - 'a' + (move[3] - '1') * 8] = board[prev[2] - 'a' + (move[3] - '1') * 8];
				board[prev[2] - 'a' + (prev[3] - '1') * 8] = 0;
			} else {
				board[move[2] - 'a' + (move[3] - '1') * 8] = board[move[0] - 'a' + (move[1] - '1') * 8];
				board[move[0] - 'a' + (move[1] - '1') * 8] = 0;
			}
		} else {
			board[move[2] - 'a' + (move[3] - '1') * 8] = board[move[0] - 'a' + (move[1] - '1') * 8];
			board[move[0] - 'a' + (move[1] - '1') * 8] = 0;
		}
	} else if (board[move[0] - 'a' + (move[1] - '1') * 8] == 6) { // a white king
		meta[1] &= 0b0011;
		if (move[0] == 'e' && move[2] == 'g') { // kingside castling
			board[4] = 0;
			board[5] = 4;
			board[6] = 6;
			board[7] = 0;
		} else if (move[0] == 'e' && move[2] == 'c') { // queenside castling
			board[4] = 0;
			board[3] = 4;
			board[2] = 6;
			board[0] = 0;
		} else {
			board[move[2] - 'a' + (move[3] - '1') * 8] = board[move[0] - 'a' + (move[1] - '1') * 8];
			board[move[0] - 'a' + (move[1] - '1') * 8] = 0;
		}
	} else if (board[move[0] - 'a' + (move[1] - '1') * 8] == 12) { // a black king
		meta[1] &= 0b1100;
		if (move[0] == 'e' && move[2] == 'g') { // kingside castling
			board[60] = 0;
			board[61] = 10;
			board[62] = 12;
			board[63] = 0;
		} else if (move[0] == 'e' && move[2] == 'c') { // queenside castling
			board[60] = 0;
			board[59] = 10;
			board[58] = 12;
			board[56] = 0;
		} else {
			board[move[2] - 'a' + (move[3] - '1') * 8] = board[move[0] - 'a' + (move[1] - '1') * 8];
			board[move[0] - 'a' + (move[1] - '1') * 8] = 0;
		}
	} else {
		board[move[2] - 'a' + (move[3] - '1') * 8] = board[move[0] - 'a' + (move[1] - '1') * 8];
		board[move[0] - 'a' + (move[1] - '1') * 8] = 0;
	}
	// revoke castling rights when rook moves
	if (board[move[0] - 'a' + (move[1] - '1') * 8] == 4) { // a white rook
		if (move[0] == 'a') {
			meta[1] &= 0b1011;
		} else if (move[0] == 'h') {
			meta[1] &= 0b0111;
		}
	} else if (board[move[0] - 'a' + (move[1] - '1') * 8] == 10) { // a black rook
		if (move[0] == 'a') {
			meta[1] &= 0b1110;
		} else if (move[0] == 'h') {
			meta[1] &= 0b1101;
		}
	}
	meta[0] = !meta[0];
	meta[2]++;
}
