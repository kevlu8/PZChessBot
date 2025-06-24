# PZChessBot

A chess engine created by two high school students, started when we were in middle school!

[Play against PZChessBot!](https://lichess.org/@/PZChessBot)

## Strength

|   Version    | CCRL Blitz | CCRL 40/15 | Lichess Blitz |
| ------------ | ---------- | ---------- | ------------- |
| v20250311T07 |   ~1900    |	   -     |     2000      |
| v1.0         |    2728    |      -     |     2500      |
| v20250421T23-dev |   ~3000    |      -     |     2600      |
| v2.0         |   3012     |      -	 |     2650      |
| v20250621T09-dev |   ~3100   |      -     |     2700      |
| v20250623T22-dev |   ~3160   |      -     |     -      |

## Logistics & Features

PZChessBot is a basic negamax engine.

### Search

- Basic alpha-beta pruning
- Quiescence search
- Principal-Variation Search
- Late-move reductions
- Transposition tables
- Null-move pruning
- Move ordering using MVV+CaptHist, killer moves, history heuristic, and counter moves
- Aspiration windows and iterative deepening
- Check extensions
- Futility and reverse futility pruning

### Moves and board representation

- Hybrid bitboard + mailbox representation
- PEXT bitboards for lightning fast move generation

### Evaluation

- NNUE-type evaluation with king buckets
- Runs a (24576->256)x2->8 model
- Trained on a mix of Stockfish and LC0 Data