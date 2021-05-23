import time
from model import Qnet
import torch
from GameMaster import GameMaster
from Communication import networkInit
from Buffer import ReplayBuffer
import torch.optim as optim
import torch.nn.functional as F
import matplotlib.pyplot as plt


device = torch.device('cuda:0' if torch.cuda.is_available() else 'cpu')
print(device)

learning_rate = 0.0005
gamma = 0.98
batch_size = 32

Receive_Buffer = []
Send_Buffer = []

networkInit(Send_Buffer, Receive_Buffer)
env = GameMaster(Send_Buffer, Receive_Buffer)
q = Qnet()
q.to(device)

# q.load_state_dict(torch.load('./params2/q_net200.pth'))

q_target = Qnet()
q_target.load_state_dict(q.state_dict())

memory = ReplayBuffer(buffer_limit=10000)

print_interval = 5
score = 0.0

optimizer = optim.Adam(q.parameters(), lr=learning_rate)


def train(q, q_target, memory, optimizer):
    for i in range(10):
        s, a, r, s_prime, done_mask = memory.sample(batch_size)

        q_out = q(s.to(device))
        q_a = q_out.gather(1, a.to(device))
        max_q_prime = q_target(s_prime).max(1)[0].unsqueeze(1)
        target = r + gamma * max_q_prime * done_mask
        loss = F.smooth_l1_loss(q_a, target.to(device))

        optimizer.zero_grad()
        loss.backward()
        optimizer.step()


rewards = []
cnts = []
for n_epi in range(1000):
    epsilon = max(0.01, 0.08 - 0.01 * (n_epi / 200))  # Linear annealing from 8% to 1%

    s = env.reset()
    done = False
    time.sleep(1)
    cnt = 0

    reward = 0
    while not done:
        a = q.sample_action(torch.from_numpy(s).float().to(device), epsilon)
        s_p, r, done, _ = env.step(a)
        done_mask = 0.0 if done else 1.0
        r = -10.0 if done else r / 2.0
        memory.put([s, a, r, s_p, done_mask])
        s = s_p

        score += r
        reward += r
        cnt += 1
        if done or cnt > 2000:
            cnts.append(cnt)
            rewards.append(reward)
            break

    if memory.size() > 200:
        train(q, q_target, memory, optimizer)
    if n_epi % print_interval == 0:
        q_target.load_state_dict(q.state_dict())
        print("n_episode :{}, score : {:.1f}, n_buffer : {}, eps : {:.1f}%".format(
            n_epi, score / print_interval, memory.size(), epsilon * 100))
        score = 0.0
        plt.plot(range(len(rewards)), rewards, color="blue")
        plt.show()

    if n_epi % 50 == 0:
        PATH = './params6/q_net' + str(n_epi) + '.pth'
        torch.save(q.state_dict(), PATH)


PATH = './q_net6.pth'
torch.save(q.state_dict(), PATH)
