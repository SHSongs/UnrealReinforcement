import struct
import time
import numpy as np
import collections

URPacket = {'reset': 0, 'step': 1}


class GameMaster:
    def __init__(self, Send_Buffer, Receive_Buffer):
        self.Send_Buffer = Send_Buffer
        self.Receive_Buffer = Receive_Buffer
        self.stack_state_cnt = 4
        self.state_buffer = collections.deque(maxlen=self.stack_state_cnt)

    def step(self, a):
        data = bytes([URPacket['step']])
        data += bytes([a])
        self.Send_Buffer.append(data)

        time.sleep(0.1)

        while len(self.Receive_Buffer) <= 0:
            time.sleep(0.1)

        packet = BytesToPacket(self.Receive_Buffer[0])
        self.Receive_Buffer.pop(0)

        info = 0
        state_prime = np.array(packet[0:9])
        reward = packet[9]
        done = False if packet[10] == 0 else True

        mean = 500
        std = 1000
        state_prime = (state_prime - mean) / std

        self.state_buffer.append(state_prime)
        process_state = np.array([])

        for i in range(self.stack_state_cnt):
            process_state = np.concatenate((process_state, self.state_buffer[i]), axis=None)

        return process_state, reward, done, info

    def reset(self):
        self.Send_Buffer.append(bytes([URPacket['reset']]))
        while len(self.Receive_Buffer) <= 0:
            time.sleep(0.1)
        state = BytesToPacket(self.Receive_Buffer[0])
        self.Receive_Buffer.pop(0)
        state = np.array(state)
        stack_state = np.array([])

        mean = 500
        std = 1000
        state = (state - mean) / std
        for i in range(self.stack_state_cnt):
            self.state_buffer.append(state)
            stack_state = np.concatenate((stack_state, state), axis=None)
        return stack_state


def BytesToPacket(data):
    info = [data[i:i + 4] for i in range(0, len(data), 4)]
    info = [int(struct.unpack('<i', data)[0]) for data in info]
    return info
