# PZEngine

Meta elements:
0: turn (1=white, 0=black)
1: castling (0b0000: wk, wq, bk, bq)
2: ply count
3: ep square
4: fullmove clock

Zobrist values:
[
    board (64x8)
    ep square (64)
    castling (4)
    turn (1)
]
