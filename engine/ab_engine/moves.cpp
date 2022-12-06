#include "moves.hpp"

inline bool is_enemy(char piece, bool turn) { return (piece > 6 && turn) || (piece < 7 && !turn); }

void pawn_control(const bool turn, const int i, char *controlled) {
	if (turn) {
		if (i % 8 != 0) {
			controlled[i + 7]++;
		}
		if (i % 8 != 7) {
			controlled[i + 9]++;
		}
	} else {
		if (i % 8 != 0) {
			controlled[i - 9]++;
		}
		if (i % 8 != 7) {
			controlled[i - 7]++;
		}
	}
}

void knight_control(const int i, char *controlled) {
	if (i / 8 != 0) {
		if (i % 8 > 1)
			controlled[i - 10]++;
		if (i % 8 < 6)
			controlled[i - 6]++;
		if (i / 8 != 1) {
			if (i % 8 != 0)
				controlled[i - 17]++;
			if (i % 8 != 7)
				controlled[i - 15]++;
		}
	}
	if (i / 8 != 7) {
		if (i % 8 > 1)
			controlled[i + 6]++;
		if (i % 8 < 6)
			controlled[i + 10]++;
		if (i / 8 != 6) {
			if (i % 8 != 0)
				controlled[i + 15]++;
			if (i % 8 != 7)
				controlled[i + 17]++;
		}
	}
}

void bishop_control(const char *position, const int i, char *controlled) {
	int j = i;
	while (j / 8 != 0 && j % 8 != 0) {
		j -= 9;
		controlled[j]++;
		if (position[j] != 0)
			break;
	}
	j = i;
	while (j / 8 != 0 && j % 8 != 7) {
		j -= 7;
		controlled[j]++;
		if (position[j] != 0)
			break;
	}
	j = i;
	while (j / 8 != 7 && j % 8 != 0) {
		j += 7;
		controlled[j]++;
		if (position[j] != 0)
			break;
	}
	j = i;
	while (j / 8 != 7 && j % 8 != 7) {
		j += 9;
		controlled[j]++;
		if (position[j] != 0)
			break;
	}
}

void rook_control(const char *position, const int i, char *controlled) {
	int j = i;
	while (j / 8 != 0) {
		j -= 8;
		controlled[j]++;
		if (position[j] != 0)
			break;
	}
	j = i;
	while (j % 8 != 0) {
		j--;
		controlled[j]++;
		if (position[j] != 0)
			break;
	}
	j = i;
	while (j % 8 != 7) {
		j++;
		controlled[j]++;
		if (position[j] != 0)
			break;
	}
	j = i;
	while (j / 8 != 7) {
		j += 8;
		controlled[j]++;
		if (position[j] != 0)
			break;
	}
}

void king_control(const int i, char *controlled) {
	if ((i - 1) / 8 == i / 8)
		controlled[i - 1]++;
	if ((i + 1) / 8 == i / 8)
		controlled[i + 1]++;
	if (i / 8 != 0) {
		controlled[i - 8]++;
		if ((i - 9) / 8 == i / 8 - 1)
			controlled[i - 9]++;
		if ((i - 7) / 8 == i / 8 - 1)
			controlled[i - 7]++;
	}
	if (i / 8 != 7) {
		controlled[i + 8]++;
		if ((i + 7) / 8 == i / 8 + 1)
			controlled[i + 7]++;
		if ((i + 9) / 8 == i / 8 + 1)
			controlled[i + 9]++;
	}
}

char *controlled_squares(const char *position, const bool turn) {
	char *controlled = new char[64];
	std::fill(controlled, controlled + 64, 0);
	for (int i = 0; i < 64; i++) {
		if (position[i] == 0)
			continue;
		if (position[i] == 1 || position[i] == 7) {
			pawn_control(turn, i, controlled);
		} else if (position[i] == 2 || position[i] == 8) {
			knight_control(i, controlled);
		} else if (position[i] == 3 || position[i] == 9) {
			bishop_control(position, i, controlled);
		} else if (position[i] == 4 || position[i] == 10) {
			rook_control(position, i, controlled);
		} else if (position[i] == 5 || position[i] == 11) {
			bishop_control(position, i, controlled);
			rook_control(position, i, controlled);
		} else if (position[i] == 6 || position[i] == 12) {
		}
	}
	return controlled;
}

void pawn_moves(const char *position, const bool turn, const int i, const std::string prev, std::vector<std::string> &moves) {
	std::string move = "";

	if (turn) {
		if (i / 8 == 1) { // second rank
			if (position[i + 8] == 0) {
				// push back in uci
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8) + '3';
				moves.push_back(move);
				move = "";
				if (position[i + 16] == 0) {
					move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8) + '4';
					moves.push_back(move);
					move = "";
				}
			}
		} else if (i / 8 != 6) {
			if (position[i + 8] == 0) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8) + std::to_string(i / 8 + 2);
				moves.push_back(move);
				move = "";
			}
		} else {
			if (position[i + 8] == 0) {
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8) + '8' + 'q';
				moves.push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8) + '8' + 'r';
				moves.push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8) + '8' + 'b';
				moves.push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8) + '8' + 'n';
				moves.push_back(move);
				move = "";
			}
		}
		if (i / 8 == 4 && prev[3] == '5' && prev[1] == '7') { // en passant
			if (abs(prev[2] - 'a' - i % 8) == 1) {
				move = move + ((char)('a' + i % 8)) + '5' + prev[0] + '6';
				moves.push_back(move);
				move = "";
			}
		}
	} else {
		if (i / 8 == 6) { // seventh rank
			if (position[i - 8] == 0) {
				// push back in uci
				move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8) + '6';
				moves.push_back(move);
				move = "";
				if (position[i + 16] == 0) {
					move = move + ((char)('a' + i % 8)) + '7' + (char)('a' + i % 8) + '5';
					moves.push_back(move);
					move = "";
				}
			}
		} else if (i / 8 != 1) {
			if (position[i - 8] == 0) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8) + std::to_string(i / 8 + 2);
				moves.push_back(move);
				move = "";
			}
		} else {
			if (position[i - 8] == 0) {
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8) + '1' + 'q';
				moves.push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8) + '1' + 'r';
				moves.push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8) + '1' + 'b';
				moves.push_back(move);
				move = "";
				move = move + ((char)('a' + i % 8)) + '2' + (char)('a' + i % 8) + '1' + 'n';
				moves.push_back(move);
				move = "";
			}
		}
		if (i / 8 == 3 && prev[3] == '4' && prev[1] == '2') { // en passant
			if (abs(prev[2] - 'a' - i % 8) == 1) {
				move = move + ((char)('a' + i % 8)) + '4' + prev[0] + '3';
				moves.push_back(move);
				move = "";
			}
		}
	}
}

void knight_moves(const char *position, const bool turn, const int i, std::vector<std::string> &moves) {
	// man i hate knights
	std::string move;

	if (i % 8 >= 1) { // can move 1 left
		if (i / 8 >= 2) { // can move 2 down
			if (position[i - 17] == 0 || is_enemy(position[i - 17], turn)) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - 1) + std::to_string(i / 8 - 1);
				moves.push_back(move);
				move = "";
			}
		}
		if (i / 8 <= 5) { // can move 2 up
			if (position[i + 15] == 0 || is_enemy(position[i + 15], turn)) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - 1) + std::to_string(i / 8 + 3);
				moves.push_back(move);
				move = "";
			}
		}
		if (i % 8 >= 2) { // can move 2 left
			if (i / 8 >= 1) { // can move 1 down
				if (position[i - 10] == 0 || is_enemy(position[i - 10], turn)) {
					move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - 2) + std::to_string(i / 8);
				}
				move = "";
			}
			if (i / 6 <= 6) { // can move 1 up
				if (position[i + 6] == 0 || is_enemy(position[i + 6], turn)) {
					move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 - 2) + std::to_string(i / 8 + 2);
				}
				move = "";
			}
		}
	}
	if (i % 8 <= 6) { // can move 1 right
		if (i / 8 >= 2) { // can move 2 down
			if (position[i - 15] == 0 || is_enemy(position[i - 15], turn)) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + 1) + std::to_string(i / 8 - 1);
				moves.push_back(move);
				move = "";
			}
		}
		if (i / 8 <= 5) { // can move 2 up
			if (position[i + 17] == 0 || is_enemy(position[i + 17], turn)) {
				move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + 1) + std::to_string(i / 8 + 3);
				moves.push_back(move);
				move = "";
			}
		}
		if (i % 8 <= 5) { // can move 2 right
			if (i / 8 >= 1) { // can move 1 down
				if (position[i - 6] == 0 || is_enemy(position[i - 6], turn)) {
					move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + 2) + std::to_string(i / 8);
					moves.push_back(move);
					move = "";
				}
			}
			if (i / 8 <= 6) { // can move 1 up
				if (position[i + 10] == 0 || is_enemy(position[i + 10], turn)) {
					move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + (char)('a' + i % 8 + 2) + std::to_string(i / 8 + 2);
					moves.push_back(move);
					move = "";
				}
			}
		}
	}
}

void bishop_moves(const char *position, const bool turn, const int i, std::vector<std::string> &moves) {
	std::string move;

	for (int j = 0; j < std::min(i % 8, i / 8); j++) {
		if (position[i - 9 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8 - j) + std::to_string(i / 8 + 1 - j);
			moves.push_back(move);
			move = "";
		} else if (is_enemy(position[i - 9 * j], turn)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8 - j) + std::to_string(i / 8 + 1 - j);
			moves.push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
	for (int j = 0; j < std::min(7 - i % 8, i / 8); j++) {
		if (position[i - 7 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8 + j) + std::to_string(i / 8 + 1 - j);
			moves.push_back(move);
			move = "";
		} else if (is_enemy(position[i - 7 * j], turn)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8 + j) + std::to_string(i / 8 + 1 - j);
			moves.push_back(move);
			move = "";
			break;
		} else {
			break;
		}
		if (turn) {
		}
	}
	for (int j = 0; j < std::min(7 - i % 8, 7 - i / 8); j++) {
		if (position[i + 9 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8 + j) + std::to_string(i / 8 + 1 + j);
			moves.push_back(move);
			move = "";
		} else if (is_enemy(position[i + 9 * j], turn)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8 + j) + std::to_string(i / 8 + 1 + j);
			moves.push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
	for (int j = 0; j < std::min(i % 8, 7 - i / 8); j++) {
		if (position[i + 7 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8 - j) + std::to_string(i / 8 + 1 + j);
			moves.push_back(move);
			move = "";
		} else if (is_enemy(position[i + 7 * j], turn)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8 - j) + std::to_string(i / 8 + 1 + j);
			moves.push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
}

void rook_moves(const char *position, const bool turn, const int i, std::vector<std::string> &moves) {
	std::string move;

	for (int j = 0; j < i % 8; j++) {
		if (position[i - 1 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8 - j) + std::to_string(i / 8 + 1);
			moves.push_back(move);
			move = "";
		} else if (is_enemy(position[i - 1 * j], turn)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8 - j) + std::to_string(i / 8 + 1);
			moves.push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
	for (int j = 0; j < 7 - i % 8; j++) {
		if (position[i + 1 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8 + j) + std::to_string(i / 8 + 1);
			moves.push_back(move);
			move = "";
		} else if (is_enemy(position[i + 1 * j], turn)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8 + j) + std::to_string(i / 8 + 1);
			moves.push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
	for (int j = 0; j < i / 8; j++) {
		if (position[i - 8 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8) + std::to_string(i / 8 + 1 - j);
			moves.push_back(move);
			move = "";
		} else if (is_enemy(position[i - 8 * j], turn)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8) + std::to_string(i / 8 + 1 - j);
			moves.push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
	for (int j = 0; j < 7 - i / 8; j++) {
		if (position[i + 8 * j] == 0) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8) + std::to_string(i / 8 + 1 + j);
			moves.push_back(move);
			move = "";
		} else if (is_enemy(position[i + 8 * j], turn)) {
			move = move + ((char)('a' + i % 8)) + std::to_string(i / 8 + 1) + ("" + 'a' + i % 8) + std::to_string(i / 8 + 1 + j);
			moves.push_back(move);
			move = "";
			break;
		} else {
			break;
		}
	}
}

void king_moves(const char *position, const bool turn, const int i, std::vector<std::string> &moves) {
	// castling is pain
	std::string move;

	if (i % 8) { // there is space on the left
	}
}

std::vector<std::string> find_legal_moves(const char *position, const std::string prev, const bool turn, const char *metadata) {
	std::vector<std::string> moves;
	std::vector<std::string> temp;
	moves.reserve(300);

	for (int i = 0; i < 64; i++) {
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
		std::string move;
		if (position[i] == 1 || position[i] == 7) {
			pawn_moves(position, turn, i, prev, moves);
		} else if (position[i] == 2 || position[i] == 8) {
			knight_moves(position, turn, i, moves);
		} else if (position[i] == 3 || position[i] == 9) {
			bishop_moves(position, turn, i, moves);
		} else if (position[i] == 4 || position[i] == 10) {
			rook_moves(position, turn, i, moves);
		} else if (position[i] == 5 || position[i] == 11) {
			bishop_moves(position, turn, i, moves);
			rook_moves(position, turn, i, moves);
		}
	}
	return moves;
}
