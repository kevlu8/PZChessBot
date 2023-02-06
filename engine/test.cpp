#include "bitboard.hpp"
#include "search.hpp"

int main() {
	std::string fen, str;
	std::getline(std::cin, fen);
	Board board(fen);
  board.print_board();
  std::pair<int, uint16_t> res;
  std::cin >> str;
  while (str != "") {
    board.make_move(parse_move(board, str));
    board.print_board();
    res = ab_search(board, 18);
    std::cout << "eval: " << res.first << " move: " << stringify_move(res.second) << std::endl;
    board.make_move(res.second);
    board.print_board();
    std::cin >> str;
  }
}
