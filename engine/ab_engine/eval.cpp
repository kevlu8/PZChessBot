#include "eval.hpp"

int eval(const char *board, const char *metadata, const std::string prev) {
	// metadata[0] = 1 for white, 0 for black
	// metadata[1] = castling rights
	// metadata[2] = halfmove clock
	// extra_metadata[0] = en passant square
	// extra_metadata[1] = fullmove number
	int material = 0, mobility = 0, king_safety = 0, pawn_structure = 0, space = 0;

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

	std::cout << "material: " << material << '\n';

	// Mobility
	// count number of legal moves for each side (unless in check, where we count the number of legal moves if we weren't in check)
	char *meta = (char *)malloc(3 * sizeof(char));
	memcpy(meta, metadata, 3);
	meta[0] = 1;
	int w_legal_moves = find_legal_moves(board, prev, meta).size();
	meta[0] = 0;
	int b_legal_moves = find_legal_moves(board, prev, meta).size();
	mobility += w_legal_moves * 10;
	mobility -= b_legal_moves * 10;
	free(meta);

	std::cout << "white legal moves: " << w_legal_moves << '\n';
	std::cout << "black legal moves: " << b_legal_moves << '\n';

	// King safety
	// count material worth of attackers and defenders on squares around the king (do not count kings)
	// weighted on a bell curve (closer to king is more important)
	char w_control[64];
	char b_control[64];
	controlled_squares(board, true, w_control, true);
	controlled_squares(board, false, b_control, true);
	int w_king_pos = -1;
	int b_king_pos = -1;
	for (int i = 0; i < 64; i++) {
		if (board[i] == 6) w_king_pos = i;
		if (board[i] == 12) b_king_pos = i;
	}
	// first check for mate
	if (check_mate(board, w_king_pos, b_control)) {
		return INT32_MIN;
	}
	if (check_mate(board, b_king_pos, w_control)) {
		return INT32_MAX;
	}
	for (int y = -2; y < 3; y++) {
		for (int x = -2; x < 3; x++) {
			if (x == 0 && y == 0) continue;
			int coeff = powf(2, 4 - abs(x) - abs(y));
			// if adding the x doesnt overflow the row and adding the y doesnt overflow the column
			if (w_king_pos >= -x && w_king_pos + x < 8 && w_king_pos + y * 8 < 8 && w_king_pos >= -y * 8) {
				king_safety += (w_control[w_king_pos + x + y * 8] - b_control[w_king_pos + x + y * 8]) * coeff;
			}
			if (b_king_pos >= -x && b_king_pos + x < 8 && b_king_pos + y * 8 < 8 && b_king_pos >= -y * 8) {
				king_safety += (w_control[b_king_pos + x + y * 8] - b_control[b_king_pos + x + y * 8]) * coeff;
			}
		}
		std::cout << std::endl;
	}
	king_safety = king_safety * 100 / 84;

	std::cout << "white control: " << '\n';
	for (int i = 0; i < 64; i++) {
		std::cout << std::setw(3) << (int)w_control[(7 - i / 8) * 8 + i % 8];
		if (i % 8 == 7) {
			std::cout << '\n';
		}
	}
	
	std::cout << "black control: " << '\n';
	for (int i = 0; i < 64; i++) {
		std::cout << std::setw(3) << (int)b_control[(7 - i / 8) * 8 + i % 8];
		if (i % 8 == 7) {
			std::cout << '\n';
		}
	}

	std::cout << "king safety: " << king_safety << '\n';

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

	return (material + mobility + king_safety) / 3;
	// weight in order:  material, king safety, mobility, space, pawn structure
}
