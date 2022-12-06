#include "eval.hpp"

int eval(const char *board, const char *metadata, const char *extra_metadata, std::string prev) {
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

	// Mobility
	// count number of legal moves for each side (unless in check, where we count the number of legal moves if we weren't in check)
	// subtract black from white
	mobility += find_legal_moves(board, prev, 1, metadata).size();
	mobility -= find_legal_moves(board, prev, 0, metadata).size();

	/*

		King safety
		count material worth of attackers and defenders on these squares (do not count kings)
		weighted on a bell curve (closer to king is more important)

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

	return (mobility);
}