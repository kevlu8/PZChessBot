# PZChessBot

A chess engine created by two high school students, started when we were in middle school!

[Play against PZChessBot!](https://lichess.org/@/PZChessBot)

## Strength

|   Version    | CCRL Blitz | CCRL 40/15 | Lichess Blitz |
| ------------ | ---------- | ---------- | ------------- |
| v20250311T07 |   ~1900    |	   -     |     2000      |
| v1.0         |    2728    |      -     |     2500      |
| v20250421T23-dev |   ~3000    |      -     |     2600      |

## Logistics & Features

PZChessBot is a basic negamax engine.

### Search

- Basic alpha-beta pruning
- Quiescence search
- Principal-Variation Search
- Late-move reductions
- Transposition tables
- Null-move pruning
- Move ordering using MVV-LVA, killer moves, history heuristic, and counter moves
- Aspiration windows and iterative deepening
- Check extensions

### Moves and board representation

- Hybrid bitboard + mailbox representation
- PEXT bitboards for lightning fast move generation

### Evaluation

- Simple NNUE-type evaluation
- Runs a (768->256)x2->8 model
- Trained on a mix of Stockfish and LC0 Data