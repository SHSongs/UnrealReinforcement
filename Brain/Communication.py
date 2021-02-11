from socket import *
import threading
import time
import struct

STOP_FLAG = False
CNT = 0
Receive_Buffer = []
Send_Buffer = []


def send(sock):
    global STOP_FLAG
    global Send_Buffer
    while True:
        if len(Send_Buffer) > 0:
            sock.send(Send_Buffer[0])
            Send_Buffer.pop(0)
        if STOP_FLAG:
            break


def receive(sock):
    global STOP_FLAG
    global CNT
    global Receive_Buffer

    while True:
        recvData = sock.recv(1024)

        if recvData:
            CNT += 1
            Receive_Buffer.append(recvData)
        else:
            print('disconeced')
            STOP_FLAG = True
            break


port = 9999

serverSock = socket(AF_INET, SOCK_STREAM)
serverSock.bind(('', port))
serverSock.listen(1)

print('%d번 포트로 접속 대기중...' % port)

connectionSock, addr = serverSock.accept()

print(str(addr), '에서 접속되었습니다.')

sender = threading.Thread(target=send, args=(connectionSock,))
receiver = threading.Thread(target=receive, args=(connectionSock,))

sender.start()
receiver.start()

URPacket = {'reset': 0, 'step': 1}


class GameMaster:
    def __init__(self):
        self.state = None

    def step(self, a):
        Send_Buffer.append(bytes([URPacket['step']]))

        time.sleep(0.5)

        while len(Receive_Buffer) <= 0:
            time.sleep(0.1)

        state_prime = BytesToState(Receive_Buffer[0])
        self.state = Receive_Buffer.pop(0)

        reward = 0
        done = False
        info = 0

        return state_prime, reward, done, info

    def reset(self):
        Send_Buffer.append(bytes([URPacket['reset']]))
        while len(Receive_Buffer) <= 0:
            print('리셋대기')
            time.sleep(0.1)
        state = BytesToState(Receive_Buffer[0])
        self.state = Receive_Buffer.pop(0)

        return state



def BytesToState(data):
    info = [data[i:i + 4] for i in range(0, len(data), 4)]
    info = [int(struct.unpack('<L', data)[0]) for data in info]
    return info


env = GameMaster()

for n_epi in range(1000):
    epsilon = max(0.01, 0.08 - 0.01 * (n_epi / 200))  # Linear annealing from 8% to 1%

    s = env.reset()
    done = False
    time.sleep(2)
    while not done:
        s_p, r, done, _ = env.step(1)
        s = s_p
        print(s)
        if done:
            break

    if STOP_FLAG:
        break
