#include "eval.hpp"

// Piece heatmaps
// where do pieces like to be?
const int pawn_heatmap[64] = {
//  a  b  c  d  e  f  g  h
	0, 0, 0, 0, 0, 0, 0, 0, // 1
	5, 10, 10, -20, -20, 10, 10, 5, // 2
	5, -5, -10, 0, 0, -10, -5, 5, // 3
	0, 0, 0, 20, 20, 0, 0, 0, // 4
	5, 5, 10, 25, 25, 10, 5, 5, // 5
	10, 10, 20, 30, 30, 20, 10, 10, // 6
	50, 50, 50, 50, 50, 50, 50, 50, // 7
	0, 0, 0, 0, 0, 0, 0, 0, // 8
}; // invert for black (heatmap[56 - i + i % 8])

const int knight_heatmap[64] = {
//  a  b  c  d  e  f  g  h
	-50,-40,-30,-30,-30,-30,-40,-50, // 1
	-40,-20,  0,  5,  5,  0,-20,-40, // 2
	-30,  5, 10, 15, 15, 10,  5,-30, // 3
	-30,  0, 15, 20, 20, 15,  0,-30, // 4
	-30,  5, 15, 20, 20, 15,  5,-30, // 5
	-30,  0, 10, 15, 15, 10,  0,-30, // 6
	-40,-20,  0,  0,  0,  0,-20,-40, // 7
	-50,-40,-30,-30,-30,-30,-40,-50, // 8
};

const int bishop_heatmap[64] = {
//  a  b  c  d  e  f  g  h
	-20,-10,-10,-10,-10,-10,-10,-20, // 1
	-10,  5,  0,  0,  0,  0,  5,-10, // 2
	-10, 10, 10, 10, 10, 10, 10,-10, // 3
	-10,  0, 10, 10, 10, 10,  0,-10, // 4
	-10,  5,  5, 10, 10,  5,  5,-10, // 5
	-10,  0,  5, 10, 10,  5,  0,-10, // 6
	-10,  0,  0,  0,  0,  0,  0,-10, // 7
	-20,-10,-10,-10,-10,-10,-10,-20, // 8
};

const int rook_heatmap[64] = {
//  a  b  c  d  e  f  g  h
	0,  0,  0,  5,  5,  0,  0,  0, // 1
	-5,  0,  0,  0,  0,  0,  0, -5, // 2
	-5,  0,  0,  0,  0,  0,  0, -5, // 3
	-5,  0,  0,  0,  0,  0,  0, -5, // 4
	-5,  0,  0,  0,  0,  0,  0, -5, // 5
	-5,  0,  0,  0,  0,  0,  0, -5, // 6
	-10,  0,  0,  0,  0,  0,  0, -10, // 7
	0,  0,  0,  0,  0,  0,  0,  0, // 8
};

const int queen_heatmap[64] = {
//  a  b  c  d  e  f  g  h
	-20,-10,-10, -5, -5,-10,-10,-20, // 1
	-10,  0,  5,  0,  0,  0,  0,-10, // 2
	-10,  5,  5,  5,  5,  5,  0,-10, // 3
	-5,  0,  5,  5,  5,  5,  0, -5, // 4
	0,  0,  5,  5,  5,  5,  0, -5, // 5
	-10,  0,  5,  5,  5,  5,  0,-10, // 6
	-10,  0,  0,  0,  0,  0,  0,-10, // 7
	-20,-10,-10, -5, -5,-10,-10,-20, // 8
};

const int king_heatmap[64] = {
//  a  b  c  d  e  f  g  h
	20, 30, 10,  0,  0, 10, 30, 20, // 1
	20, 20,  0,  0,  0,  0, 20, 20, // 2
	-10,-20,-20,-20,-20,-20,-20,-10, // 3
	-20,-30,-30,-40,-40,-30,-30,-20, // 4
	-30,-40,-40,-50,-50,-40,-40,-30, // 5
	-30,-40,-40,-50,-50,-40,-40,-30, // 6
	-30,-40,-40,-50,-50,-40,-40,-30, // 7
	-30,-40,-40,-50,-50,-40,-40,-30, // 8
};

int eval(const char *board, const char *metadata, const std::string prev, const char *w_control, const char *b_control, int &w_king_pos, int &b_king_pos) noexcept {
	// metadata[0] = 1 for white, 0 for black
	// metadata[1] = castling rights
	// metadata[2] = halfmove clock
	// extra_metadata[0] = en passant square
	// extra_metadata[1] = fullmove number
	int material = 0, mobility = 0, king_safety = 0, pawn_structure = 0, space = 0, position = 0;

	// King safety
	// count material worth of attackers and defenders on squares around the king (do not count kings)
	// weighted on a bell curve (closer to king is more important)

	// Material
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
	for (int i = 0; i < 64; i++) {
		// Mobility
		// count number of squares controlled by each side
		mobility += w_control[i] - b_control[i];
		switch (board[i]) {
		case 1:
			material += 100;
			position += pawn_heatmap[i];
			break;
		case 2:
			material += 320;
			position += knight_heatmap[i];
			break;
		case 3:
			material += 330;
			position += bishop_heatmap[i];
			break;
		case 4:
			material += 500;
			position += rook_heatmap[i];
			break;
		case 5:
			material += 900;
			position += queen_heatmap[i];
			break;
		case 6:
			w_king_pos = i;
			position += king_heatmap[i];
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
			b_king_pos = i;
			position -= king_heatmap[56 - i + (i % 8) * 2];
			break;
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

		Space
			count number of squares controlled by each side
			subtract black from white
	*/

	return ((4 * material) + (mobility) + (2 * king_safety) + (3 * position)) / 10;
	// weight in order:  material, position, king safety, mobility, space, pawn structure
}
