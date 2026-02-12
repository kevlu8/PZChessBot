#pragma once

#include "includes.hpp"

// A move needs 16 bits to be stored
// bits 0-5: Destination square (from 0 to 63)
// bits 6-11: Origin (from 0 to 63)
// bits 12-14: promotion piece type - 1 (from KNIGHT-1 to QUEEN-1)
// bits 14-15: special move flag: promotion (1), en passant (2), king promo (3)
struct Move {
	uint16_t data;
	constexpr Move() : data(0) {}
	constexpr explicit Move(uint16_t d) : data(d) {}
	constexpr Move(int from, int to) : data((from << 6) | to) {}
	template <MoveType T> static constexpr Move make(int from, int to, PieceType pt = KNIGHT) {
		if (pt != KING) return Move(T | ((pt - KNIGHT) << 12) | (from << 6) | to);
		else return Move(T | (from << 6) | to | (3 << 14));
	}
	constexpr Square src() const {
		return (Square)(data >> 6 & 0x3f);
	};
	constexpr Square dst() const {
		return (Square)(data & 0x3f);
	};
	constexpr PieceType promotion() const {
		return (PieceType)((data >> 12) & 0b11);
	};
	constexpr MoveType type() const {
		return (MoveType)(data & 0xc000);
	};
	constexpr bool operator==(const Move &m) const {
		return data == m.data;
	};
	constexpr bool operator!=(const Move &m) const {
		return data != m.data;
	};
	std::string to_string() const;
	static Move from_string(const std::string &, const void *);
};

static constexpr Move NullMove = Move(0);
