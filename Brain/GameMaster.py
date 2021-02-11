import struct
import time
import numpy as np

URPacket = {'reset': 0, 'step': 1}


class GameMaster:
    def __init__(self, Send_Buffer, Receive_Buffer):
        self.Send_Buffer = Send_Buffer
        self.Receive_Buffer = Receive_Buffer

    def step(self, a):
        data = bytes([URPacket['step']])
        data += bytes([a])
        self.Send_Buffer.append(data)

        time.sleep(0.5)

        while len(self.Receive_Buffer) <= 0:
            time.sleep(0.1)

        packet = BytesToPacket(self.Receive_Buffer[0])
        self.Receive_Buffer.pop(0)

        info = 0
        state_prime = np.array(packet[0:9])
        reward = packet[9]
        done = False if packet[10] == 0 else True

        return state_prime, reward, done, info

    def reset(self):
        self.Send_Buffer.append(bytes([URPacket['reset']]))
        while len(self.Receive_Buffer) <= 0:
            print('리셋대기')
            time.sleep(0.1)
        state = BytesToPacket(self.Receive_Buffer[0])
        self.Receive_Buffer.pop(0)
        state = np.array(state)
        return state


def BytesToPacket(data):
    info = [data[i:i + 4] for i in range(0, len(data), 4)]
    info = [int(struct.unpack('<L', data)[0]) for data in info]
    return info
