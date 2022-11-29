import torch
import torch.nn as nn
import torch.nn.functional as F

class PZEngine(nn.Module):
	def __init__(self):
		# make a model for a chess engine
		self.model = nn.Sequential(
			nn.Conv2d(1, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
			nn.ReLU(),
			nn.Conv2d(32, 32, 3, padding=1),
			nn.BatchNorm2d(32),
		)
		# self.model = nn.Sequential(
		# 	nn.Conv2d(1, 32, 5, 1, 0),
		# 	nn.ReLU(),
		# 	nn.Conv2d(32, 32, 3, 1, 1),
		# 	nn.ReLU(),
		# 	nn.MaxPool2d(2, 2),
		# 	nn.Conv2d(32, 64, 3, 1, 1),
		# )
				
	def forward(self, x):
		return self.model(x)
