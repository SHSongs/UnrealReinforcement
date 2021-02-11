from socket import *
import threading

STOP_FLAG = False
CNT = 0

Receive_Buffer = None
Send_Buffer = None


def networkInit(Send, Receive):
    global Send_Buffer
    global Receive_Buffer

    Send_Buffer = Send
    Receive_Buffer = Receive

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
