import torch
import torch.nn as nn
import torch.nn.functional as F

# nn based chess engine......
class PZEngine(object):
	def __init__(self, model, device):
		self.model = model
		self.device = device

	def predict(self, board):
		# convert board to tensor
		board_tensor = torch.tensor(board).float().to(self.device)
		# forward pass
		with torch.no_grad():
			policy, value = self.model(board_tensor)
		# convert policy to numpy array
		policy = policy.cpu().numpy()
		# convert value to scalar
		value = value.item()
		# return policy and value
		return policy, value