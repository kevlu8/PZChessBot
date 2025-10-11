# PZChessBot

![donate btc](https://img.shields.io/badge/donate%20btc-31pma4U314hJHSxXBECWxYFPBgL7n9BoCC-blue)

A chess engine created by two high school students, started when we were in middle school!

[Play against PZChessBot!](https://lichess.org/@/PZChessBot)

## Strength

|      Version     | CCRL Blitz | CCRL 40/15 | Lichess Rapid |
| ---------------- | ---------- | ---------- | ------------- |
| v20250311T07     |   ~1900    |     -      |     2000      |
| v1.0             |    2712    |     -      |     2500      |
| v20250421T23-dev |   ~3000    |     -      |     2600      |
| v2.0             |    2986    |     -      |     2650      |
| v20250621T09-dev |   ~3100    |     -      |     2700      |
| v20250623T22-dev |   ~3160    |     -      |     2800      |
| v3.0             |    3305    |    3275    |     2850      |
| v20250729T08-dev |   ~3400    |     -      |     2900      |
| v4.0             |    3433    |     -      |     2900      |

## Logistics & Features

PZChessBot is a basic negamax engine.

### Search

- Basic alpha-beta pruning
- Quiescence search
- Principal-Variation Search
- Late-move reductions
- Late-move pruning
- Transposition tables
- Null-move pruning
- Move ordering using MVV+CaptHist, killer moves, history heuristic, and counter moves
- Aspiration windows and iterative deepening
- Check extensions
- Futility and reverse futility pruning
- Razoring
- Singular extensions
- Negative extensions
- History pruning
- Static exchange evaluation

### Moves and board representation

- Hybrid bitboard + mailbox representation
- PEXT bitboards for lightning fast move generation

### Evaluation

- NNUE-type evaluation with king buckets
- Runs a (768x16->512)x2->1x8 model
- Trained from zero-knowledge using self-play games

### Special Thanks

- The [Stockfish Discord Server](https://discord.gg/XUyHyT5ap9), specifically `#engines-dev` for their help!
- The [bullet](https://github.com/jw1912/bullet) NNUE trainer
- The [ChessProgramming Wiki](https://chessprogramming.org/) for their clear albeit outdated explanations
- [OpenBench](https://github.com/AndyGrant/OpenBench) for providing an excellent testing GUI
- [sscg13](https://github.com/sscg13) for sharing an OpenBench instance with me and helping me with a lot of miscellaneous stuff
- Lastly, YOU for checking out this project!
