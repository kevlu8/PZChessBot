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
- Move ordering using static eval in search and MVV-LVA in QSearch
- Reverse futility pruning
- Aspiration windows and iterative deepening
- Check extensions

### Moves and board representation

- Hybrid bitboard + mailbox representation
- PEXT bitboards for lightning fast move generation

### Evaluation

- Simple NNUE-type evaluation
- Runs a 768->128x2->1 model
- Trained on a mix of Stockfish and LC0 Data