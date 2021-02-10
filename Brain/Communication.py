from socket import *
import threading
import time
import struct

STOP_FLAG = False
CNT = 0


def send(sock):
    global STOP_FLAG

    while True:
        sendData = '1'
        sock.send(sendData.encode('utf-8'))
        if STOP_FLAG:
            break

        time.sleep(0.5)


def receive(sock):
    global STOP_FLAG
    global CNT
    while True:
        recvData = sock.recv(1024)

        if recvData:
            CNT += 1

            info = [recvData[i:i+4] for i in range(0, len(recvData), 4)]
            info = [int(struct.unpack('<f', data)[0]) for data in info]
            print(info)
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

while True:
    time.sleep(0.5)
    print(CNT)
    if STOP_FLAG:
        break
