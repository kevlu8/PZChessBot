import PZEngine


def trainEval():
    # open training data
    # training data will be a json file with the following format:
    # [
    # 	[
    # 		[64 numbers representing the board (a->h, 1->8)],
    # 		[5 numbers representing metadata],
    # 		eval
    # 	],
    #	...
    # ]
    # each element in the board array will be a number from 0 to 12
    # 0 = empty
    # 1 = white pawn
    # 2 = white knight
    # 3 = white bishop
    # 4 = white rook
    # 5 = white queen
    # 6 = white king
    # 7 = black pawn
    # 8 = black knight
    # 9 = black bishop
    # 10 = black rook
    # 11 = black queen
    # 12 = black king
    # the metadata array will be a 4 element array with the following format:
    # [active color, castling rights, halfmove clock, last move]
    pass


if __name__ == "__main__":
    trainEval()
