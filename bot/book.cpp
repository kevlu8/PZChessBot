// #include "book.hpp"

// std::map<std::string, std::string> book;

// void init_book() {
// 	std::fstream file("book.db");
// 	book[""] = "e2e4"; // default opening
// 	std::string line, move;
// 	while (file >> line >> move) {
// 		book[line] = move;
// 	}
// 	file.close();
// }

// std::pair<Move, bool> book_move(std::vector<std::string> &moves, Board &board) {
// 	std::string cur_moves = "";
// 	for (int i = 0; i < moves.size(); i++) {
// 		cur_moves += moves[i] + ",";
// 	}
// 	if (!cur_moves.empty()) cur_moves.pop_back();
// 	if (book.find(cur_moves) != book.end()) {
// 		return { Move::from_string(book[cur_moves], &board), true };
// 	}
// 	std::string req = "https://explorer.lichess.ovh/masters?play=" + cur_moves;
// 	cpr::Response res = cpr::Get(cpr::Url{req});
// 	if (res.status_code != 200) {
// 		std::cerr << "error: " << res.status_code << ' ' << res.text << std::endl;
// 		return { NullMove, false };
// 	}
// 	json data = json::parse(res.text);
// 	if (data["moves"].size() == 0)
// 		return { NullMove, false };
// 	std::string move = data["moves"][0]["uci"];
// 	book[cur_moves] = move;
// 	std::fstream file("book.db", std::ios::app);
// 	file << cur_moves << " " << move << std::endl;
// 	file.close();
// 	return { Move::from_string(move, &board), true };
// }