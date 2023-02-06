#include "bitboard.hpp"

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

constexpr int endgame_heatmap[64] = {
	//  a  b  c  d  e  f  g  h
	1, 2,  4,  8,  8,  4,  2,  1, // 1
	2, 4,  8,  16, 16, 8,  4,  2, // 2
	4, 8,  16, 32, 32, 16, 8,  4, // 3
	8, 16, 32, 64, 64, 32, 16, 8, // 4
	8, 16, 32, 64, 64, 32, 16, 8, // 5
	4, 8,  16, 32, 32, 16, 8,  4, // 6
	2, 4,  8,  16, 16, 8,  4,  2, // 7
	1, 2,  4,  8,  8,  4,  2,  1, // 8
};

constexpr const int *heatmaps[] = {nullptr, queen_heatmap, rook_heatmap, bishop_heatmap, knight_heatmap, pawn_heatmap};

int Board::eval() {
	if (meta[3] == 100)
		return 0;
	int material, positioning, mobility, king_safety, controlledsquares;
	material = positioning = mobility = king_safety = controlledsquares = 0;

	for (int i = 1; i < 6; i++) {
		// material
		material += (_popcnt64(pieces[i] & pieces[6]) - _popcnt64(pieces[i] & pieces[7])) * piece_values[i];
		// positioning
		U64 tmpw = pieces[i] & pieces[6];
		U64 tmpb = pieces[i] & pieces[7];
		while (tmpw) {
			positioning += heatmaps[i][__builtin_ctzll(tmpw)];
			tmpw &= tmpw - 1;
		}
		while (tmpb) {
			positioning -= heatmaps[i][0b111000 - (__builtin_ctzll(tmpb) & 0b111000) | (__builtin_ctzll(tmpb) & 0b111)];
			tmpb &= tmpb - 1;
		}
	}

	// decide if endgame
	if (_popcnt64(pieces[6] | pieces[7]) < 10) {
		positioning += 2 * ((_popcnt64(pieces[7]) * endgame_heatmap[__builtin_ctzll(pieces[0] & pieces[6])]) - (_popcnt64(pieces[6]) * endgame_heatmap[__builtin_ctzll(pieces[0] & pieces[7])])) / _popcnt64(pieces[6] | pieces[7]);
	} else {
		positioning += king_heatmap[__builtin_ctzll(pieces[0] & pieces[6])] - king_heatmap[0b111000 - (__builtin_ctzll(pieces[0] & pieces[7]) & 0b111000) | (__builtin_ctzll(pieces[0] & pieces[7]) & 0b111)];
	}

	return ((3 * material) + (positioning)) / 4;

	// // mobility
	// // count the number of pseudolegal moves for each side
	// std::unordered_set<uint16_t> moves = std::unordered_set<uint16_t>();
	// legal_moves(moves);
	// if (meta[0])
	// 	mobility += moves.size();
	// else
	// 	mobility -= moves.size();
	// meta[0] = !meta[0];
	// legal_moves(moves);
	// if (meta[0])
	// 	mobility += moves.size();
	// else
	// 	mobility -= moves.size();
	// meta[0] = !meta[0];

	// // king safety
	// // count control around king and also sliding piece attack chances

	// // control
	// // count control over the board
	// controlled_squares(true);
	// controlled_squares(false);
	// controlledsquares = _popcnt64(control[1]) - _popcnt64(control[0]);

	// // return (material + mobility + king_safety + controlledsquares) / 4;
	// return ((4 * material) + (3 * positioning) + (controlledsquares)) / 8;
}
