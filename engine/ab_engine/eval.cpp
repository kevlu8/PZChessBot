#include "eval.hpp"

int eval(const char *board, const char *metadata, const std::string prev) noexcept {
	// metadata[0] = 1 for white, 0 for black
	// metadata[1] = castling rights
	// metadata[2] = halfmove clock
	// extra_metadata[0] = en passant square
	// extra_metadata[1] = fullmove number
	int material = 0, mobility = 0, king_safety = 0, pawn_structure = 0, space = 0, position = 0;

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
		switch (board[i]) {
		case 1:
			material += 100;
			break;
		case 2:
			material += 300;
			break;
		case 3:
			material += 300;
			break;
		case 4:
			material += 500;
			break;
		case 5:
			material += 900;
			break;
		case 7:
			material -= 100;
			break;
		case 8:
			material -= 300;
			break;
		case 9:
			material -= 300;
			break;
		case 10:
			material -= 500;
			break;
		case 11:
			material -= 900;
			break;
		}
	}

	// King safety
	// count material worth of attackers and defenders on squares around the king (do not count kings)
	// weighted on a bell curve (closer to king is more important)
	char w_control[64];
	char b_control[64];
	controlled_squares(board, true, w_control, true);
	controlled_squares(board, false, b_control, true);
	// std::cout << "w_control: " << '\n';
	// for (int i = 0; i < 64; i++) {
	// 	std::cout << std::setw(3) << (int)w_control[(7 - i / 8) * 8 + i % 8];
	// 	if (i % 8 == 7) {
	// 		std::cout << '\n';
	// 	}
	// }
	// std::cout << std::endl;
	int w_king_pos = -1;
	int b_king_pos = -1;
	for (int i = 0; i < 64; i++) {
		if (board[i] == 6)
			w_king_pos = i;
		else if (board[i] == 12)
			b_king_pos = i;
	}
	// first check for mate
	if (check_mate(board, w_king_pos, prev, metadata)) {
		return -1e9;
	}
	if (check_mate(board, b_king_pos, prev, metadata)) {
		return 1e9;
	}
	for (int y = -2; y < 3; y++) {
		for (int x = -2; x < 3; x++) {
			if (x == 0 && y == 0)
				continue;
			int coeff = powf(2, 4 - abs(x) - abs(y));
			// if adding the x doesnt overflow the row and adding the y doesnt overflow the column
			if ((w_king_pos % 8) >= -x && (w_king_pos % 8) + x < 8 && (w_king_pos / 8) + y < 8 && (w_king_pos / 8) >= -y) {
				king_safety += (w_control[w_king_pos + x + y * 8] - b_control[w_king_pos + x + y * 8]) * coeff;
			}
			if ((b_king_pos % 8) >= -x && (b_king_pos % 8) + x < 8 && (b_king_pos / 8) + y < 8 && (b_king_pos / 8) >= -y) {
				king_safety += (w_control[b_king_pos + x + y * 8] - b_control[b_king_pos + x + y * 8]) * coeff;
			}
		}
		// std::cout << std::endl;
	}
	king_safety = king_safety * 100 / 84;

	controlled_squares(board, true, w_control, false);
	controlled_squares(board, false, b_control, false);
	// Mobility
	// count number of squares controlled by each side
	for (int i = 0; i < 64; i++) 
		mobility += w_control[i] - b_control[i];

	// Piece heatmaps
	// where do pieces like to be?
	int pawn_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
		0, 0, 0, 0, 0, 0, 0, 0, // 1
		1, 2, 2, 1, 1, 2, 2, 1, // 2
		1, 2, 3, 5, 5, 1, 2, 3, // 3
		1, 2, 4, 8, 8, 4, 1, 2, // 4
		1, 1, 2, 5, 5, 1, 1, 1, // 5
		2, 2, 2, 2, 2, 2, 2, 2, // 6
		6, 6, 6, 6, 6, 6, 6, 6, // 7
		9, 9, 9, 9, 9, 9, 9, 9, // 8
	}; // invert for black (heatmap[ - i])

	int knight_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
		1, 1, 1, 1, 1, 1, 1, 1, // 1
		2, 1, 1, 4, 4, 2, 1, 2, // 2
		2, 2, 5, 3, 3, 5, 3, 2, // 3
		1, 1, 4, 5, 5, 4, 1, 1, // 4
		1, 3, 4, 5, 5, 4, 3, 1, // 5
		2, 2, 2, 2, 2, 2, 2, 2, // 6
		1, 1, 3, 1, 1, 3, 1, 1, // 7
		1, 1, 1, 1, 1, 1, 1, 1, // 8
	};

	int bishop_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
		1, 1, 1, 1, 1, 1, 1, 1, // 1
		1, 4, 1, 3, 3, 1, 4, 1, // 2
		2, 2, 1, 1, 1, 1, 2, 2, // 3
		1, 1, 5, 1, 1, 1, 1, 2, // 4
		1, 4, 1, 1, 1, 1, 5, 1, // 5
		1, 1, 1, 1, 1, 1, 1, 1, // 6
		1, 2, 1, 1, 1, 1, 2, 1, // 7
		1, 1, 1, 1, 1, 1, 1, 1, // 8
	};

	int rook_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
		1, 1, 4, 5, 5, 3, 1, 3, // 1
		1, 1, 1, 1, 1, 1, 1, 1, // 2
		1, 1, 1, 1, 1, 1, 1, 1, // 3
		1, 1, 1, 1, 1, 1, 1, 1, // 4
		1, 1, 1, 1, 1, 1, 1, 1, // 5
		1, 1, 1, 1, 1, 1, 1, 1, // 6
		5, 5, 5, 5, 5, 5, 5, 5, // 7
		3, 3, 3, 3, 3, 3, 3, 3, // 8
	};

	int queen_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
		1, 1, 1, 5, 1, 1, 1, 1, // 1
		1, 1, 7, 1, 1, 1, 1, 1, // 2
		1, 7, 1, 1, 1, 3, 1, 1, // 3
		1, 1, 1, 1, 1, 1, 1, 1, // 4
		1, 1, 1, 1, 1, 1, 1, 1, // 5
		1, 1, 1, 1, 1, 1, 1, 1, // 6
		1, 1, 1, 1, 1, 1, 1, 1, // 7
		1, 1, 1, 1, 1, 1, 1, 1, // 8
	};

	int king_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
		2, 4, 5, 2, 3, 2, 5, 4, // 1
		2, 2, 2, 1, 1, 1, 2, 2, // 2
		1, 1, 1, 1, 1, 1, 1, 1, // 3
		1, 1, 1, 1, 1, 1, 1, 1, // 4
		1, 1, 1, 1, 1, 1, 1, 1, // 5
		1, 1, 1, 1, 1, 1, 1, 1, // 6
		1, 1, 1, 1, 1, 1, 1, 1, // 7
		1, 1, 1, 1, 1, 1, 1, 1, // 8
	};

	for (int i = 0; i < 64; i++) {
		if (board[i] == 1) {
			position += pawn_heatmap[i];
		}
		else if (board[i] == 2) {
			position += knight_heatmap[i];
		}
		else if (board[i] == 3) {
			position += bishop_heatmap[i];
		}
		else if (board[i] == 4) {
			position += rook_heatmap[i];
		}
		else if (board[i] == 5) {
			position += queen_heatmap[i];
		}
		else if (board[i] == 6) {
			position += king_heatmap[i];
		}
		else if (board[i] == 7) {
			position -= pawn_heatmap[56 - i + (i % 8) * 2];
		}
		else if (board[i] == 8) {
			position -= knight_heatmap[56 - i + (i % 8) * 2];
		}
		else if (board[i] == 9) {
			position -= bishop_heatmap[56 - i + (i % 8) * 2];
		}
		else if (board[i] == 10) {
			position -= rook_heatmap[56 - i + (i % 8) * 2];
		}
		else if (board[i] == 11) {
			position -= queen_heatmap[56 - i + (i % 8) * 2];
		}
		else if (board[i] == 12) {
			position -= king_heatmap[56 - i + (i % 8) * 2];
		}
	}
	position *= 10;

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

	return ((4 * material) + (mobility) + (king_safety) + (4 * position)) / 10;
	// weight in order:  material, position, king safety, mobility, space, pawn structure
	return 0;
}
