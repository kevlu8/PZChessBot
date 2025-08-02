#include "test.hpp"

#include "bitboard.hpp"
#include "search.hpp"

TEST_SUITE("Legality checking") {
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
			board.load_fen("r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 3");
			for (int i = 0; i < 20; i++) {
				// Play best move
				auto res = search(board, 100, false);
				Move move = res.first;
				board.make_move(move);

				// Check correctness
				moves.clear();
				board.legal_moves(moves);
				bool good = true;
				for (const Move &move : special) {
					CAPTURE(move.to_string());
					if (board.is_pseudolegal(move) != (moves.count(move) > 0))
						good = false;
					CHECK_EQ(board.is_pseudolegal(move), moves.count(move) > 0);
				}
				for (const Move &move : normal) {
					CAPTURE(move.to_string());
					if (board.is_pseudolegal(move) != (moves.count(move) > 0))
						good = false;
					CHECK_EQ(board.is_pseudolegal(move), moves.count(move) > 0);
				}

				if (!good) {
					MESSAGE("Test failed after ", i + 1, " moves");
					MESSAGE("Last move: ", move.to_string());
					MESSAGE(board.get_fen(), board.print_board());
					break;
				}
			}
		}
	}
}
