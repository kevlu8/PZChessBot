# PZChessBot

A chess engine created by two high school students, started when we were in middle school!

[Play against PZChessBot!](https://lichess.org/@/PZChessBot)

## Logistics & Features

PZChessBot is a basic negamax engine.

### Search

- Basic alpha-beta pruning
- Quiescence search
- Principal-Variation Search
- Late-move reductions
- Transposition tables
- Null-move pruning

### Moves and board representation

- Hybrid bitboard + mailbox representation
- PEXT bitboards for lightning fast move generation

### Evaluation

- Simple HCE-type evaluation
- Weighs material and position (piece-square tables)
- Roughly counts king safety though it is not very accurate

## Up next

- NNUE?