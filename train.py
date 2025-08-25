import torch
import torch.nn as nn
from torch.utils.data import DataLoader, TensorDataset
import numpy as np

EPOCHS = 100
HL_SIZE = 32
LR = 0.001
BATCH_SIZE = 16384

class NN(nn.Module):
    def __init__(self, input_size, hidden_size):
        super(NN, self).__init__()
        self.fc1 = nn.Linear(input_size, hidden_size)
        self.relu = nn.ReLU()
        self.fc2 = nn.Linear(hidden_size, 1)  # output logits (no sigmoid)

    def forward(self, x):
        out = self.fc1(x)
        out = self.relu(out)
        out = self.fc2(out)
        return out  # logits

device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
model = NN(input_size=7, hidden_size=HL_SIZE).to(device)

# Load & split into X, y
data = np.loadtxt('data.txt', delimiter=',', dtype=np.float32)
X, y = torch.tensor(data[:, :-1], dtype=torch.float32), torch.tensor(data[:, -1], dtype=torch.float32).unsqueeze(1)

train_loader = DataLoader(TensorDataset(X, y), batch_size=BATCH_SIZE, shuffle=True)

# Handle imbalance
pos = (y == 1).sum().item()
neg = (y == 0).sum().item()
pos_weight = torch.tensor([neg / max(pos, 1)], device=device)

criterion = nn.BCEWithLogitsLoss(pos_weight=pos_weight)
optimizer = torch.optim.Adam(model.parameters(), lr=LR)

# Training loop
model.train()
for epoch in range(EPOCHS):
    loss_sum = 0
    idx = 0
    for inputs, labels in train_loader:
        inputs, labels = inputs.to(device), labels.to(device)
        optimizer.zero_grad()
        logits = model(inputs)  # raw scores
        loss = criterion(logits, labels)
        loss.backward()
        optimizer.step()
        loss_sum += loss.item()
        idx += 1
        if idx % 100 == 0:
            print(f"Epoch [{epoch+1}/{EPOCHS}], Step [{idx}/{len(train_loader)}], Loss: {loss_sum/idx:.4f}")
    print(f"Epoch [{epoch+1}/{EPOCHS}], Loss: {loss_sum/len(train_loader):.4f}")

torch.save(model.state_dict(), 'model.pth')
print("Model saved to model.pth")
