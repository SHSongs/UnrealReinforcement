import random
import torch.nn as nn
import torch.nn.functional as F

class Qnet(nn.Module):
    def __init__(self):
        super(Qnet, self).__init__()
        self.fc1 = nn.Linear(36, 128)
        self.fc2 = nn.Linear(128, 10000)
        self.fc3 = nn.Linear(10000, 10000)
        self.fc4 = nn.Linear(10000, 128)
        self.fc5 = nn.Linear(128, 3)

    def forward(self, x):
        x = F.relu(self.fc1(x))
        x = F.relu(self.fc2(x))
        x = F.relu(self.fc3(x))
        x = F.relu(self.fc4(x))
        x = self.fc5(x)
        return x

    def sample_action(self, obs, epsilon):
        out = self.forward(obs)
        coin = random.random()
        if coin < epsilon:
            return random.randint(0, 2)
        else:
            return out.argmax().item()