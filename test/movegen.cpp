#include "test.hpp"

#include "bitboard.hpp"
#include "search.hpp"

extern Bitboard rook_blockers[64][64];
extern Bitboard bishop_blockers[64][64];

TEST_SUITE("Legality checking") {
	TEST_CASE("Rook blockers") {
		// Make sure a -> b is the same as b -> a
		SUBCASE("Symmetry") {
			bool passed = true;
			for (Square src = SQ_A1; src <= SQ_H8; src++) {
				for (Square dst = Square(src + 1); dst <= SQ_H8; dst++) {
					if (src == dst)
						continue;

					CAPTURE(to_string(src));
					CAPTURE(to_string(dst));
					SHORT_EQ(passed, rook_blockers[src][dst], rook_blockers[dst][src]);
				}
			}
			REQUIRE(passed);
		}

		// Make sure the source square is included in all illegal moves but not in any legal moves
		SUBCASE("Legality") {
			bool passed = true;
			Bitboard rank = 0x00000000000000ff;
			Bitboard file = 0x0101010101010101;
			for (Square src = SQ_A1; src <= SQ_H8; src++) {
				Bitboard rankray = rank << (src & 0b111000);
				Bitboard fileray = file << (src & 0b111);
				for (Square dst = Square(src + 1); dst <= SQ_H8; dst++) {
					bool legal = false;
					if (src != dst)
						legal = square_bits(dst) & (rankray | fileray);

					CAPTURE(to_string(src));
					CAPTURE(to_string(dst));
					if (legal) {
						SHORT_EQ(passed, rook_blockers[src][dst] & square_bits(src), 0);
					} else {
						SHORT_NE(passed, rook_blockers[src][dst] & square_bits(src), 0);
					}
				}
			}
			REQUIRE(passed);
		}

		// Check for correctness
		SUBCASE("Validity") {
			// Rows
			for (Rank r1 = RANK_1; r1 <= RANK_8; r1++) {
				for (File f1 = FILE_A; f1 < FILE_H; f1++) {
					Bitboard mask = 0;
					for (File f2 = File(f1 + 1); f2 <= FILE_H; f2++) {
						Square src = make_square(f1, r1);
						Square dst = make_square(f2, r1);
						CHECK_EQ(rook_blockers[src][dst], mask);
						mask |= square_bits(dst);
					}
				}
			}
			// Cols
			for (Rank r1 = RANK_1; r1 < RANK_8; r1++) {
				for (File f1 = FILE_A; f1 <= FILE_H; f1++) {
					Bitboard mask = 0;
					for (Rank r2 = Rank(r1 + 1); r2 <= RANK_8; r2++) {
						Square src = make_square(f1, r1);
						Square dst = make_square(f1, r2);
						CHECK_EQ(rook_blockers[src][dst], mask);
						mask |= square_bits(dst);
					}
				}
			}
		}
	}

	TEST_CASE("Bishop blockers") {
		// Make sure a -> b is the same as b -> a
		SUBCASE("Symmetry") {
			bool passed = true;
			for (Square src = SQ_A1; src <= SQ_H8; src++) {
				for (Square dst = Square(src + 1); dst <= SQ_H8; dst++) {
					if (src == dst)
						continue;

					CAPTURE(to_string(src));
					CAPTURE(to_string(dst));
					SHORT_EQ(passed, bishop_blockers[src][dst], bishop_blockers[dst][src]);
				}
			}
			REQUIRE(passed);
		}

		// Make sure the source square is included in all illegal moves but not in any legal moves
		SUBCASE("Legality") {
			bool passed = true;
			Bitboard diag = 0x8040201008040201;
			Bitboard anti = 0x0102040810204080;
			for (Square src = SQ_A1; src <= SQ_H8; src++) {
				int shift = (src & 0b111) - (src >> 3);
				Bitboard diagray;
				if (shift >= 0)
					diagray = (diag >> (shift * 8));
				else
					diagray = (diag << (-shift * 8));
				Bitboard antiray;
				shift = 7 - (src & 0b111) - (src >> 3);
				if (shift >= 0)
					antiray = (anti >> (shift * 8));
				else
					antiray = (anti << (-shift * 8));
				for (Square dst = Square(src + 1); dst <= SQ_H8; dst++) {
					bool legal = false;
					if (src != dst)
						legal = square_bits(dst) & (diagray | antiray);

					CAPTURE(to_string(src));
					CAPTURE(to_string(dst));
					if (legal) {
						SHORT_EQ(passed, bishop_blockers[src][dst] & square_bits(src), 0);
					} else {
						SHORT_NE(passed, bishop_blockers[src][dst] & square_bits(src), 0);
					}
				}
			}
			REQUIRE(passed);
		}

		// Check for correctness
		SUBCASE("Validity") {
			// Diagonals
			for (Rank r1 = RANK_2; r1 <= RANK_8; r1++) {
				for (File f1 = FILE_B; f1 <= FILE_H; f1++) {
					Bitboard mask = 0;
					for (int off = 1; off <= std::min((int)r1, (int)f1); off++) {
						Square src = make_square(f1, r1);
						Square dst = make_square(File(f1 - off), Rank(r1 - off));
						CAPTURE(to_string(src));
						CAPTURE(to_string(dst));
						CHECK_EQ(bishop_blockers[src][dst], mask);
						mask |= square_bits(dst);
					}
				}
			}
			// Antidiagonals
			for (Rank r1 = RANK_2; r1 <= RANK_8; r1++) {
				for (File f1 = FILE_A; f1 < FILE_H; f1++) {
					Bitboard mask = 0;
					for (int off = 1; off <= std::min((int)r1, FILE_H - f1); off++) {
						Square src = make_square(f1, r1);
						Square dst = make_square(File(f1 + off), Rank(r1 - off));
						CAPTURE(to_string(src));
						CAPTURE(to_string(dst));
						CHECK_EQ(bishop_blockers[src][dst], mask);
						mask |= square_bits(dst);
					}
				}
			}
		}
	}

	TEST_CASE("is_pseudolegal") {
		Board board;
		pzstd::vector<Move> moves;
		std::vector<Move> special, normal;

		// Create a list of all possible special moves
		special.push_back(Move::make<CASTLING>(SQ_E1, SQ_G1));
		special.push_back(Move::make<CASTLING>(SQ_E1, SQ_C1));
		special.push_back(Move::make<CASTLING>(SQ_E8, SQ_G8));
		special.push_back(Move::make<CASTLING>(SQ_E8, SQ_C8));
		for (int i = 0; i < 8; i++) {
			// Straight promotions
			special.push_back(Move::make<PROMOTION>(make_square((File)i, RANK_7), make_square((File)i, RANK_8), QUEEN));
			special.push_back(Move::make<PROMOTION>(make_square((File)i, RANK_2), make_square((File)i, RANK_1), QUEEN));
			if (i != 0) {
				// Capture promotions
				special.push_back(Move::make<PROMOTION>(make_square(File(i - 1), RANK_7), make_square(File(i), RANK_8), QUEEN));
				special.push_back(Move::make<PROMOTION>(make_square(File(i), RANK_7), make_square(File(i - 1), RANK_8), QUEEN));
				special.push_back(Move::make<PROMOTION>(make_square(File(i - 1), RANK_2), make_square(File(i), RANK_1), QUEEN));
				special.push_back(Move::make<PROMOTION>(make_square(File(i), RANK_2), make_square(File(i - 1), RANK_1), QUEEN));

				special.push_back(Move::make<EN_PASSANT>(make_square(File(i - 1), RANK_5), make_square(File(i), RANK_6)));
				special.push_back(Move::make<EN_PASSANT>(make_square(File(i), RANK_5), make_square(File(i - 1), RANK_6)));
				special.push_back(Move::make<EN_PASSANT>(make_square(File(i - 1), RANK_4), make_square(File(i), RANK_3)));
				special.push_back(Move::make<EN_PASSANT>(make_square(File(i), RANK_4), make_square(File(i - 1), RANK_3)));
			}
		}

		// Create list of all non-special moves
		for (Square src = SQ_A1; src <= SQ_H8; src++) {
			int cnt = 0;
			for (Square dst = SQ_A1; dst <= SQ_H8; dst++) {
				if (src == dst)
					continue; // Skip same square

				Move move = Move(src, dst);
				bool valid = true;
				for (const Move &special_move : special) {
					if (src == special_move.src() && dst == special_move.dst()) {
						valid = false;
						break;
					}
				}
				if (!valid)
					continue;
				normal.push_back(move);
				cnt++;
			}
		}

		SUBCASE("Startpos") {
			board.legal_moves(moves);
			REQUIRE_EQ(moves.size(), 20);

			// Check all valid moves and see if they match the expected moves
			for (const Move &move : special) {
				CAPTURE(move.to_string());
				CHECK_EQ(board.is_pseudolegal(move), moves.count(move) > 0);
			}
			for (const Move &move : normal) {
				CAPTURE(move.to_string());
				CHECK_EQ(board.is_pseudolegal(move), moves.count(move) > 0);
			}
		}

		SUBCASE("Opening") {
			board.reset("rnbqkb1r/1p2pppp/p2p1n2/8/3NP3/2N5/PPP2PPP/R1BQKB1R w KQkq - 0 6");
			const int MAX = 20;
			bool good = true;
			for (int i = 0; i < MAX; i++) {
				// Play best move
				auto res = search_nodes(board, 15000, true);
				Move move = res.first;
				board.make_move(move);

				// Check correctness
				moves.clear();
				board.legal_moves(moves);
				for (const Move &move : special) {
					CAPTURE(move.to_string());
					SHORT_EQ(good, board.is_pseudolegal(move), moves.count(move) > 0);
				}
				for (const Move &move : normal) {
					CAPTURE(move.to_string());
					SHORT_EQ(good, board.is_pseudolegal(move), moves.count(move) > 0);
				}

				if (!good) {
					MESSAGE(move.to_string(), ": \x1b[31mFAILED\x1b[0m ", i + 1, "/20");
					MESSAGE(board.get_fen(), board.print_board());
					break;
				}
				MESSAGE(move.to_string(), ": \x1b[32mPASSED\x1b[0m ", i + 1, "/", MAX);

				board.reset(board.get_fen());
			}
			if (good)
				MESSAGE(board.get_fen(), board.print_board());
		}

		SUBCASE("Endgame") {
			board.reset("3k4/8/3K4/3P4/8/8/8/8 b - - 0 1");
			const int MAX = 27;
			bool good = true;
			for (int i = 0; i < MAX; i++) {
				// Play best move
				auto res = search_nodes(board, 15000, true);
				Move move = res.first;
				board.make_move(move);

				// Check correctness
				moves.clear();
				board.legal_moves(moves);
				for (const Move &move : special) {
					CAPTURE(move.to_string());
					SHORT_EQ(good, board.is_pseudolegal(move), moves.count(move) > 0);
				}
				for (const Move &move : normal) {
					CAPTURE(move.to_string());
					SHORT_EQ(good, board.is_pseudolegal(move), moves.count(move) > 0);
				}

				if (!good) {
					MESSAGE(move.to_string(), ": \x1b[31mFAILED\x1b[0m ", i + 1, "/20");
					MESSAGE(board.get_fen(), board.print_board());
					break;
				}
				MESSAGE(move.to_string(), ": \x1b[32mPASSED\x1b[0m ", i + 1, "/", MAX);

				board.reset(board.get_fen());
			}
			if (good)
				MESSAGE(board.get_fen(), board.print_board());
		}

		SUBCASE("Stress test") {
			board.reset("1r1r1r1r/P1P1P1P1/p2k4/1p1Pp1p1/P5pP/8/8/R3K2R w KQ e6 0 1");
			moves.clear();
			board.legal_moves(moves);

			// Check all valid moves
			for (const Move &move : special) {
				CAPTURE(move.to_string());
				CHECK_EQ(board.is_pseudolegal(move), moves.count(move) > 0);
			}
			for (const Move &move : normal) {
				CAPTURE(move.to_string());
				CHECK_EQ(board.is_pseudolegal(move), moves.count(move) > 0);
			}

			board.reset("r3k2r/8/8/P6p/p2pP1P1/1P1K3P/1p1p1p1p/R1R1R1R1 b kq e3 0 1");
			moves.clear();
			board.legal_moves(moves);

			// Check all valid moves
			for (const Move &move : special) {
				CAPTURE(move.to_string());
				CHECK_EQ(board.is_pseudolegal(move), moves.count(move) > 0);
			}
			for (const Move &move : normal) {
				CAPTURE(move.to_string());
				CHECK_EQ(board.is_pseudolegal(move), moves.count(move) > 0);
			}
		}
	}
}
