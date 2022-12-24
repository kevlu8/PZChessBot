#include "board.hpp"

static constexpr inline int __transpose_index(int index) { return (index % 8) << 3 + index / 8; }

// Piece heatmaps
// where do pieces like to be?
constexpr int pawn_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	0,	0,	0,	 0,	  0,   0,	0,	0, // 1
	5,	10, 10,	 -40, -40, 10,	10, 5, // 2
	5,	-5, -10, 0,	  0,   -10, -5, 5, // 3
	0,	0,	0,	 30,  30,  0,	0,	0, // 4
	5,	5,	10,	 40,  40,  10,	5,	5, // 5
	10, 10, 50,	 60,  60,  50,	10, 10, // 6
	80, 80, 80,	 80,  80,  80,	80, 80, // 7
	0,	0,	0,	 0,	  0,   0,	0,	0, // 8
};

constexpr int knight_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	-50, -40, -30, -30, -30, -30, -40, -50, // 1
	-40, -20, 0,   5,	5,	 0,	  -20, -40, // 2
	-30, 5,	  10,  15,	15,	 10,  5,   -30, // 3
	-30, 0,	  15,  20,	20,	 15,  0,   -30, // 4
	-30, 5,	  15,  20,	20,	 15,  5,   -30, // 5
	-30, 0,	  10,  15,	15,	 10,  0,   -30, // 6
	-40, -20, 0,   0,	0,	 0,	  -20, -40, // 7
	-50, -40, -30, -30, -30, -30, -40, -50, // 8
};

constexpr int bishop_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	-20, -10, -10, -10, -10, -10, -10, -20, // 1
	-10, 5,	  0,   0,	0,	 0,	  5,   -10, // 2
	-10, 10,  10,  10,	10,	 10,  10,  -10, // 3
	-10, 0,	  10,  10,	10,	 10,  0,   -10, // 4
	-10, 5,	  5,   10,	10,	 5,	  5,   -10, // 5
	-10, 0,	  5,   10,	10,	 5,	  0,   -10, // 6
	-10, 0,	  0,   0,	0,	 0,	  0,   -10, // 7
	-20, -10, -10, -10, -10, -10, -10, -20, // 8
};

constexpr int rook_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	-10, 0, 0, 10, 10, 5, 0, -10, // 1
	-5,	 0, 0, 0,  0,  0, 0, -5, // 2
	-5,	 0, 0, 0,  0,  0, 0, -5, // 3
	-5,	 0, 0, 0,  0,  0, 0, -5, // 4
	-5,	 0, 0, 0,  0,  0, 0, -5, // 5
	-5,	 0, 0, 0,  0,  0, 0, -5, // 6
	-10, 0, 0, 0,  0,  0, 0, -10, // 7
	0,	 0, 0, 0,  0,  0, 0, 0, // 8
};

constexpr int queen_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	-20, -10, -10, -5, -5, -10, -10, -20, // 1
	-10, 0,	  5,   0,  0,  0,	0,	 -10, // 2
	-10, 5,	  5,   5,  5,  5,	0,	 -10, // 3
	-5,	 0,	  5,   5,  5,  5,	0,	 -5, // 4
	0,	 0,	  5,   5,  5,  5,	0,	 -5, // 5
	-10, 0,	  5,   5,  5,  5,	0,	 -10, // 6
	-10, 0,	  0,   0,  0,  0,	0,	 -10, // 7
	-20, -10, -10, -5, -5, -10, -10, -20, // 8
};

constexpr int king_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	20,	 40,  30,  0,	0,	 10,  40,  20, // 1
	20,	 20,  -5,  -5,	-5,	 -5,  20,  20, // 2
	-10, -20, -20, -20, -20, -20, -20, -10, // 3
	-20, -30, -30, -40, -40, -30, -30, -20, // 4
	-30, -40, -40, -50, -50, -40, -40, -30, // 5
	-30, -40, -40, -50, -50, -40, -40, -30, // 6
	-30, -40, -40, -50, -50, -40, -40, -30, // 7
	-30, -40, -40, -50, -50, -40, -40, -30, // 8
};

constexpr int fen_to_piece[] = {4, -1, -1, -1, -1, -1, -1, -1, -1, 1, -1, -1, 5, -1, 6, 2, 3};
constexpr char piece_to_fen[] = {' ', 'K', 'Q', 'R', 'B', 'N', 'P', 'k', 'q', 'r', 'b', 'n', 'p'};
constexpr int piece_values[] = {0, 100, 320, 330, 500, 900, 20000, -100, -320, -330, -500, -900, -20000};

Board::Board() { load_fen(STARTING_POSITION); }

Board::Board(const std::string &fen) { load_fen(fen); }

void Board::load_fen(const std::string &fen) {
	int currIdx = 0;
	for (int j = 7; j >= 0; j--) {
		for (int i = 0; i < 8; i++) {
			if (fen[currIdx] == '/') {
				currIdx++;
				i--;
				continue;
			}
			if (fen[currIdx] < '9') {
				for (int k = 0; k < fen[currIdx] - '0'; k++)
					data[i + k][j] = 0;
				i += fen[currIdx] - '1';
				currIdx++;
				continue;
			}
			if (fen[currIdx] < 'a') {
				data[i][j] = fen_to_piece[fen[currIdx] - 'B'];
			} else {
				data[i][j] = fen_to_piece[fen[currIdx] - 'b'] + 6;
			}
			currIdx++;
		}
	}
	currIdx++;
	meta[0] = (fen[currIdx] == 'w');
	currIdx += 2;
	if (fen[currIdx] == '-') {
		meta[1] = 0;
		currIdx++;
	} else {
		if (fen[currIdx] == 'K') {
			meta[1] ^= 0b1000;
			currIdx++;
		}
		if (fen[currIdx] == 'Q') {
			meta[1] ^= 0b0100;
			currIdx++;
		}
		if (fen[currIdx] == 'k') {
			meta[1] ^= 0b0010;
			currIdx++;
		}
		if (fen[currIdx] == 'q') {
			meta[1] ^= 0b0001;
			currIdx++;
		}
	}
	currIdx++;
	if (fen[currIdx] == '-') {
		meta[3] = -1;
		currIdx += 2;
	} else {
		meta[3] = (fen[currIdx] - 'a') << 3 + (fen[currIdx + 1] - '1');
		currIdx += 3;
	}
	meta[2] = std::stoi(fen.substr(currIdx));
	meta[4] = std::stoi(fen.substr(currIdx + 2 + (fen[currIdx + 1] != ' ')));
}

void Board::print_board() {
	for (int j = 7; j >= 0; j--) {
		for (int i = 0; i < 8; i++) {
			std::cout << piece_to_fen[data[i][j]] << ' ';
		}
		std::cout << std::endl;
	}
}

void Board::make_move(const char *move) {
	if (move[4] == 0) {
		memcpy(prev, move, 4);
		prev[4] = data[move[2] - 'a'][move[3] - '1'] ? piece_to_fen[data[move[2] - 'a'][move[3] - '1']] : '-';
		prev[5] = 0;
		prev[6] = 0;
	} else {
		memcpy(prev, move, 5);
		prev[5] = data[move[2] - 'a'][move[3] - '1'] ? piece_to_fen[data[move[2] - 'a'][move[3] - '1']] : '-';
		prev[6] = 0;
	}
	if (move[4] != 0) { // promotion
		meta[2] = -1;
		data[move[0] - 'a'][move[1] - '1'] = 0;
		switch (move[4]) {
		case 'n':
			data[move[2] - 'a'][move[3] - '1'] = 5;
			break;
		case 'b':
			data[move[2] - 'a'][move[3] - '1'] = 4;
			break;
		case 'r':
			data[move[2] - 'a'][move[3] - '1'] = 3;
			break;
		case 'q':
			data[move[2] - 'a'][move[3] - '1'] = 2;
			break;
		}
		data[move[2] - 'a'][move[3] - '1'] += !meta[0] * 6;
	} else if ((move[2] - 'a') << 3 + move[3] - '1' == meta[3]) { // en passant
		meta[2] = -1;
		data[move[2] - 'a'][move[3] - '1'] = data[move[2] - 'a'][move[1] - '1'];
		data[move[0] - 'a'][move[1] - '1'] = 0;
		data[move[2] - 'a'][move[1] - '1'] = 0;
		prev[4] = meta[0] ? 'e' : 'E';
	} else if (strncmp(move, "e1g1", 4) == 0) { // white kingside castle
		meta[1] &= 0b0011;
		data[4][0] = 0;
		data[5][0] = 3;
		data[6][0] = 1;
		data[7][0] = 0;
	} else if (strncmp(move, "e1c1", 4) == 0) { // white queenside castle
		meta[1] &= 0b0011;
		data[0][0] = 0;
		data[2][0] = 1;
		data[3][0] = 3;
		data[4][0] = 0;
	} else if (strncmp(move, "e8g8", 4) == 0) { // black kingside castle
		meta[1] &= 0b1100;
		data[4][7] = 0;
		data[5][7] = 9;
		data[6][7] = 7;
		data[7][7] = 0;
	} else if (strncmp(move, "e8c8", 4) == 0) { // black queenside castle
		meta[1] &= 0b1100;
		data[0][7] = 0;
		data[2][7] = 7;
		data[3][7] = 9;
		data[4][7] = 0;
	} else { // normal move
		if (!strncmp(move, "e1", 2) && data[move[0] - 'a'][move[1] - '1'] == 1) // white king
			meta[1] &= 0b0011;
		else if (!strncmp(move, "e8", 2) && data[move[0] - 'a'][move[1] - '1'] == 7) // black king
			meta[1] &= 0b1100;
		else if (!strncmp(move, "a1", 2) && data[move[0] - 'a'][move[1] - '1'] == 3) // white queen rook
			meta[1] &= 0b1011;
		else if (!strncmp(move, "h1", 2) && data[move[0] - 'a'][move[1] - '1'] == 3) // white king rook
			meta[1] &= 0b0111;
		else if (!strncmp(move, "a8", 2) && data[move[0] - 'a'][move[1] - '1'] == 9) // black queen rook
			meta[1] &= 0b1110;
		else if (!strncmp(move, "h8", 2) && data[move[0] - 'a'][move[1] - '1'] == 9) // black king rook
			meta[1] &= 0b1101;
		else if (data[move[0] - 'a'][move[1] - '1'] == 6 || data[move[0] - 'a'][move[1] - '1'] == 12) { // pawn move
			meta[2] = -1;
			if (move[0] == move[2] && abs(move[1] - move[3]) == 2) // double pawn move
				meta[3] = move[2] << 3 + (move[3] + move[1]) / 2;
			else
				meta[3] = -1;
		}
		data[move[2] - 'a'][move[3] - '1'] = data[move[0] - 'a'][move[1] - '1'];
		data[move[0] - 'a'][move[1] - '1'] = 0;
	}
	meta[0] = !meta[0];
	meta[2]++;
}

void Board::unmake_move() {
	memcpy(meta, prevmeta, 5);
	if (strncmp(prev, "e1g1", 4) == 0) { // white kingside castle
		data[4][0] = 1;
		data[5][0] = 0;
		data[6][0] = 0;
		data[7][0] = 3;
	} else if (strncmp(prev, "e1c1", 4) == 0) { // white queenside castle
		data[0][0] = 3;
		data[2][0] = 0;
		data[3][0] = 0;
		data[4][0] = 1;
	} else if (strncmp(prev, "e8g8", 4) == 0) { // black kingside castle
		data[4][7] = 7;
		data[5][7] = 0;
		data[6][7] = 0;
		data[7][7] = 9;
	} else if (strncmp(prev, "e8c8", 4) == 0) { // black queenside castle
		data[0][7] = 9;
		data[2][7] = 0;
		data[3][7] = 0;
		data[4][7] = 7;
	} else if (prev[5] == 0) { // no promotion
		data[prev[0] - 'a'][prev[1] - '1'] = data[prev[2] - 'a'][prev[3] - '1']; // set origin to destination
		if (prev[4] == 'e') // en passant
			data[prev[2] - 'a'][prev[1] - '1'] = 12; // replace taken pawn
		else if (prev[4] == 'E') // en passant
			data[prev[2] - 'a'][prev[1] - '1'] = 6; // replace taken pawn
		else
			data[prev[2] - 'a'][prev[3] - '1'] = prev[4] == '-' ? 0 : prev[4] < 'a' ? fen_to_piece[prev[4] - 'B'] : fen_to_piece[prev[4] - 'b'] + 6; // set destination to captured piece
	} else { // promotion
		data[prev[0] - 'a'][prev[1] - '1'] = (data[prev[2] - 'a'][prev[3] - '1'] / 6 + 1) * 6; // set origin to destination colored pawn
		data[prev[2] - 'a'][prev[3] - '1'] = prev[5] == '-' ? 0 : prev[5] < 'a' ? fen_to_piece[prev[5] - 'B'] : fen_to_piece[prev[5] - 'b'] + 6; // replace piece on destination square
	}
}