from socket import *
import threading
import time

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
            print('상대방 :', recvData.decode('utf-8'))
        else:
            print('disconeced')
            STOP_FLAG = True
            break
        time.sleep(0.5)


port = 9999

serverSock = socket(AF_INET, SOCK_STREAM)
serverSock.bind(('', port))
serverSock.listen(1)

print('%d번 포트로 접속 대기중...'%port)

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
