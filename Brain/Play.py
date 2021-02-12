import time
from model import Qnet
import torch
from GameMaster import GameMaster
from Communication import networkInit
from Buffer import ReplayBuffer
import torch.optim as optim
import torch.nn.functional as F


Receive_Buffer = []
Send_Buffer = []

networkInit(Send_Buffer, Receive_Buffer)
env = GameMaster(Send_Buffer, Receive_Buffer)

q = Qnet()
q.load_state_dict(torch.load('./params/q_net200.pth'))


q_target = Qnet()
q_target.load_state_dict(q.state_dict())


print_interval = 20
score = 0.0

for n_epi in range(1000):

    s = env.reset()
    done = False
    time.sleep(2)
    while not done:
        a = q.sample_action(torch.from_numpy(s).float(), 0)
        s_p, r, done, _ = env.step(a)
        s = s_p
        score += r
        if done:
            print(score)
            score = 0.0
            break


PATH = './q_net.pth'
torch.save(q.state_dict(), PATH)
