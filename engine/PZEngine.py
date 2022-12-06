import torch
import torch.nn as nn
import torch.nn.functional as F


class Insert():
    def __call__(self, x: torch.Tensor):
        # concatenate with a tensor of shape (4,) with active color, castling rights, last move, halfmove clock
        # active color w = 1 b = 0
        # castling right is represented bitwise in the order KQkq
        # for example Kkq would be 0b1011 = 11
        # last move is the last move made represented in ICCF numeric notation
        # the halfmove clock is the number of halfmoves since the last capture or pawn advance
        return torch.cat(x, self.metadata)


class PZEval(nn.Module):
    def __init__(self, insert):
        # make a model for a chess eval network
        self.model = nn.Sequential(
            nn.Conv2d(1, 32, 5),
            nn.BatchNorm2d(32),
            nn.ReLU(),
            nn.Conv2d(32, 64, 3),
            nn.BatchNorm2d(64),
            nn.ReLU(),
            nn.Flatten(),
            insert,
            nn.Linear(261, 128),
            nn.ReLU(),
            nn.Linear(128, 64),
            nn.ReLU(),
            nn.Linear(64, 1),  # [eval]
        )

    def forward(self, x):
        return self.model(x)


class PZEngine(nn.Module):
    def __init__(self, insert):
        # make a model for a chess engine
        self.model = nn.Sequential(
            nn.Conv2d(1, 32, 5),
            nn.BatchNorm2d(32),
            nn.ReLU(),
            nn.Conv2d(32, 64, 3),
            nn.BatchNorm2d(64),
            nn.ReLU(),
            nn.Flatten(),
            insert,
            nn.Linear(261, 128),
            nn.ReLU(),
            nn.Linear(128, 64),
            nn.ReLU(),
            nn.Linear(64, 5),  # [file, rank, file, rank, promotion]
        )

    def forward(self, x):
        return self.model(x)
