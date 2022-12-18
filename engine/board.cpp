#include "board.hpp"

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

Board::Board() {
	this->meta = (char *)malloc(5);
	this->prev = (char *)malloc(8);
}

Board::Board(char board[8][8], const char *meta) {
	this->meta = (char *)malloc(5);
	this->prev = (char *)malloc(8);
	memcpy(this->board, board, 64);
	memcpy(this->meta, meta, 5);
}

Board::~Board() noexcept {
	free(meta);
	free(prev);
}

/// TODO: fix errors
void Board::make_move(const char *move, unsigned long long &hash) {
	strcpy(prev, move);
	// reset halfmove clock when capturing
	if (board[move[2] - 'a'][move[3] - '1'] != 0)
		meta[2] = -1;
	// revoke castling rights if rook move
	if (board[move[0] - 'a'][move[1] - '1'] == 4) { // a white rook
		if (move[0] == 'a') {
			meta[1] &= 0b1011;
		} else if (move[0] == 'h') {
			meta[1] &= 0b0111;
		}
	} else if (board[move[0] - 'a'][move[1] - '1'] == 10) { // a black rook
		if (move[0] == 'a') {
			meta[1] &= 0b1110;
		} else if (move[0] == 'h') {
			meta[1] &= 0b1101;
		}
	}
	if (strlen(move) == 5) { // promotion
		meta[2] = -1;
		board[move[0] - 'a'][move[1] - '1'] = 0;
		if (move[4] == 'q') {
			board[move[2] - 'a'][move[3] - '1'] = meta[0] ? 5 : 11;
		} else if (move[4] == 'r') {
			board[move[2] - 'a'][move[3] - '1'] = meta[0] ? 4 : 10;
		} else if (move[4] == 'b') {
			board[move[2] - 'a'][move[3] - '1'] = meta[0] ? 3 : 9;
		} else if (move[4] == 'n') {
			board[move[2] - 'a'][move[3] - '1'] = meta[0] ? 2 : 8;
		}
	} else if (board[move[0] - 'a'][move[1] - '1'] == 1 || board[move[0] - 'a'][move[1] - '1'] == 7) { // a pawn
		meta[2] = -1;
		// en passant
		if (move[2] - 'a' + (move[3] - '1') * 8 == meta[3]) {
			board[move[2] - 'a'][move[3] - '1'] = board[move[0] - 'a'][move[1] - '1'];
			board[move[0] - 'a'][move[1] - '1'] = 0;
			board[move[2] - 'a'][move[1] - '1'] = 0;
		} else {
			board[move[2] - 'a'][move[3] - '1'] = board[move[0] - 'a'][move[1] - '1'];
			board[move[0] - 'a'][move[1] - '1'] = 0;
		}
	} else if (board[move[0] - 'a'][move[1] - '1'] == 6) { // a white king
		meta[1] &= 0b0011;
		if (move[0] == 'e' && move[2] == 'g') { // kingside castling
			board[0][4] = 0;
			board[0][5] = 4;
			board[0][6] = 6;
			board[0][7] = 0;
		} else if (move[0] == 'e' && move[2] == 'c') { // queenside castling
			board[0][4] = 0;
			board[0][3] = 4;
			board[0][2] = 6;
			board[0][0] = 0;
		} else {
			board[move[2] - 'a'][move[3] - '1'] = board[move[0] - 'a'][move[1] - '1'];
			board[move[0] - 'a'][move[1] - '1'] = 0;
		}
	} else if (board[move[0] - 'a'][move[1] - '1'] == 12) { // a black king
		meta[1] &= 0b1100;
		if (move[0] == 'e' && move[2] == 'g') { // kingside castling
			board[0][4] = 0;
			board[0][5] = 10;
			board[0][6] = 12;
			board[0][7] = 0;
		} else if (move[0] == 'e' && move[2] == 'c') { // queenside castling
			board[0][4] = 0;
			board[0][3] = 10;
			board[0][2] = 12;
			board[0][0] = 0;
		} else {
			board[move[2] - 'a'][move[3] - '1'] = board[move[0] - 'a'][move[1] - '1'];
			board[move[0] - 'a'][move[1] - '1'] = 0;
		}
	} else {
		board[move[2] - 'a'][move[3] - '1'] = board[move[0] - 'a'][move[1] - '1'];
		board[move[0] - 'a'][move[1] - '1'] = 0;
	}
	meta[0] = !meta[0];
	meta[2]++;
}

void Board::unmake_move() noexcept {}

int Board::eval() noexcept {
	int material = 0, mobility = 0, king_safety = 0, pawn_structure = 0, position = 0;

	// King safety
	// count material worth of attackers and defenders on squares around the king (do not count kings)
	// weighted on a bell curve (closer to king is more important)
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			// Mobility
			// count number of squares controlled by each side
			mobility += w_control[i][j] - b_control[i][j];
			switch (board[i][j]) {
			case 0:
				continue;
			case 1:
				material += 100;
				position += pawn_heatmap[i * 8 + j];
				break;
			case 2:
				material += 320;
				position += knight_heatmap[i * 8 + j];
				break;
			case 3:
				material += 330;
				position += bishop_heatmap[i * 8 + j];
				break;
			case 4:
				material += 500;
				position += rook_heatmap[i * 8 + j];
				break;
			case 5:
				material += 900;
				position += queen_heatmap[i * 8 + j];
				break;
			case 6:
				if (w_king_pos != -1)
					w_king_pos = i;
				position += king_heatmap[i * 8 + j];
				break;
			case 7:
				material -= 100;
				position -= pawn_heatmap[56 - i + (i % 8) * 2];
				break;
			case 8:
				material -= 320;
				position -= knight_heatmap[56 - i + (i % 8) * 2];
				break;
			case 9:
				material -= 330;
				position -= bishop_heatmap[56 - i + (i % 8) * 2];
				break;
			case 10:
				material -= 500;
				position -= rook_heatmap[56 - i + (i % 8) * 2];
				break;
			case 11:
				material -= 900;
				position -= queen_heatmap[56 - i + (i % 8) * 2];
				break;
			case 12:
				if (b_king_pos != -1)
					b_king_pos = i;
				position -= king_heatmap[56 - i + (i % 8) * 2];
				break;
			}
		}
	}

	for (int y = -2; y < 3; y++) {
		for (int x = -2; x < 3; x++) {
			if (x == 0 && y == 0)
				continue;
			int coeff = 1 << (4 - abs(x) - abs(y));
			// if adding the x doesnt overflow the row and adding the y doesnt overflow the column
			if ((w_king_pos % 8) >= -x && (w_king_pos % 8) + x < 8 && (w_king_pos / 8) + y < 8 && (w_king_pos / 8) >= -y) {
				king_safety += (w_control[w_king_pos + x + y * 8] - b_control[w_king_pos + x + y * 8]) * coeff;
			}
			if ((b_king_pos % 8) >= -x && (b_king_pos % 8) + x < 8 && (b_king_pos / 8) + y < 8 && (b_king_pos / 8) >= -y) {
				king_safety += (w_control[b_king_pos + x + y * 8] - b_control[b_king_pos + x + y * 8]) * coeff;
			}
		}
	}

	/*
		Pawn structure
		judge isolated pawns (their safety and potential exposure to attack)
			no pawns on the surrounding files
			IQP (isolated queen pawn) is a special case as it is a weakness but also a potential strength
		judge doubled pawns
			a pair of pawns on the same file
		judge passed pawns
			a pawn that is not blocked by another pawn and there are no pawns on adjacent files in front of it
	*/

	return ((4 * material) + (mobility) + (2 * king_safety) + (3 * position)) / 10;
	// weight in order:  material, position, king safety, mobility, pawn structure
}

bool Board::is_checkmate() noexcept {
	// check if the king is in check and if there are any legal moves
	if (is_check()) {
		// check if king has moves that get out of check
	}
	return false;
}