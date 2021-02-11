import time
from model import Qnet
import torch
from GameMaster import GameMaster
from Communication import networkInit


Receive_Buffer = []
Send_Buffer = []

networkInit(Send_Buffer, Receive_Buffer)
env = GameMaster(Send_Buffer, Receive_Buffer)
q = Qnet()

for n_epi in range(1000):
    epsilon = max(0.01, 0.08 - 0.01 * (n_epi / 200))  # Linear annealing from 8% to 1%

    s = env.reset()
    done = False
    time.sleep(2)
    while not done:
        a = q.sample_action(torch.from_numpy(s).float(), epsilon)
        s_p, r, done, _ = env.step(a)
        s = s_p
        print(s)
        if done:
            print('done')
            break
