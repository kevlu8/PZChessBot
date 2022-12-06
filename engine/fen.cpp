#include "fen.hpp"

void parse_fen(const std::string fen, char *board, char *metadata, char *extra_metadata) {
	memset(board, 0, 64);
	memset(metadata, 0, 3);
	memset(extra_metadata, 0, 2);
	int currIdx = 0;
	for (int i = 7; i >= 0; i--) {
		for (int j = 0; j < 8; j++) {
			if (fen[currIdx] == '/') {
				currIdx++;
				j--;
				continue;
			}
			if (fen[currIdx] < '9') {
				j += fen[currIdx] - '1';
				currIdx++;
				continue;
			}
			if (fen[currIdx] < 'a') {
				board[i * 8 + j] = fen_to_char[fen[currIdx] - 'B'];
			} else {
				board[i * 8 + j] = fen_to_char[fen[currIdx] - 'b'] + 6;
			}
			currIdx++;
		}
	}
	currIdx++;
	metadata[0] = fen[currIdx] == 'w';
	currIdx += 2;
	if (fen[currIdx] == '-') {
		metadata[1] = 0;
		currIdx++;
	} else {
		if (fen[currIdx] == 'K') {
			metadata[1] ^= 0b1000;
			currIdx++;
		}
		if (fen[currIdx] == 'Q') {
			metadata[1] ^= 0b0100;
			currIdx++;
		}
		if (fen[currIdx] == 'k') {
			metadata[1] ^= 0b0010;
			currIdx++;
		}
		if (fen[currIdx] == 'q') {
			metadata[1] ^= 0b0001;
			currIdx++;
		}
	}
	currIdx++;
	if (fen[currIdx] == '-') {
		extra_metadata[0] = 0;
		currIdx += 2;
	} else {
		extra_metadata[0] = (fen[currIdx] - 'a') * 10 + (fen[currIdx + 1] - '1');
		currIdx += 3;
	}
	if (fen[currIdx + 1] == ' ') {
		metadata[2] = fen[currIdx] - '0';
		currIdx += 2;
	} else {
		metadata[2] = (fen[currIdx] - '0') * 10 + fen[currIdx + 1] - '0';
		currIdx += 3;
	}
	while (currIdx < fen.size()) {
		extra_metadata[1] = extra_metadata[1] * 10 + fen[currIdx] - '0';
		currIdx++;
	}
}

void serialize_fen(const char *board, const char *metadata, const char *extra_metadata, std::string &fen) {
	fen = "";
	int empty = 0;
	for (int i = 7; i >= 0; i--) {
		for (int j = 0; j < 8; j++) {
			if (board[i * 8 + j] == 0) {
				empty++;
			} else {
				if (empty > 0) {
					fen += empty + '0';
					empty = 0;
				}
				if (board[i * 8 + j] <= 6) {
					fen += char_to_fen[board[i * 8 + j]];
				} else {
					fen += char_to_fen[board[i * 8 + j] - 6] + 32;
				}
			}
		}
		if (empty > 0) {
			fen += empty + '0';
			empty = 0;
		}
		if (i) {
			fen += '/';
		}
	}
	fen += ' ';
	fen += metadata[0] ? 'w' : 'b';
	fen += ' ';
	if (metadata[1] == 0) {
		fen += '-';
	} else {
		if (metadata[1] & 0b1000) {
			fen += 'K';
		}
		if (metadata[1] & 0b0100) {
			fen += 'Q';
		}
		if (metadata[1] & 0b0010) {
			fen += 'k';
		}
		if (metadata[1] & 0b0001) {
			fen += 'q';
		}
	}
	fen += ' ';
	if (extra_metadata[0] == 0) {
		fen += '-';
	} else {
		fen += extra_metadata[0] / 10 + 'a';
		fen += extra_metadata[0] % 10 + '1';
	}
	fen += ' ';
	if (metadata[2] >= 10) {
		fen += metadata[2] / 10 + '0';
		fen += metadata[2] % 10 + '0';
	} else {
		fen += metadata[2] + '0';
	}
	fen += ' ';
	if ((unsigned char)extra_metadata[1] >= 100) {
		fen += (unsigned char)extra_metadata[1] / 100 + '0';
		fen += ((unsigned char)extra_metadata[1] / 10) % 10 + '0';
		fen += (unsigned char)extra_metadata[1] % 10 + '0';
	} else if (extra_metadata[1] >= 10) {
		fen += extra_metadata[1] / 10 + '0';
		fen += extra_metadata[1] % 10 + '0';
	} else {
		fen += extra_metadata[1] + '0';
	}
}
