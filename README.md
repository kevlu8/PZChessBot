# PZChessBot

A chess engine created by two high school students, started when we were in middle school!

[Play against PZChessBot!](https://lichess.org/@/PZChessBot)

## Logistics & Features

PZChessBot is a basic negamax engine.

### Search

- Basic alpha-beta pruning
- Principal-Variation Search
- Late-move reductions
- Transposition tables

### Moves and board representation

- Hybrid bitboard + mailbox representation
- Magic bitboards for lightning fast move generation

### Evaluation

- Simple HCE-type evaluation
- Weighs material and position (piece-square tables)

## Up next

- Null-move pruning once we have stuff fixed
- Deeper evaluation