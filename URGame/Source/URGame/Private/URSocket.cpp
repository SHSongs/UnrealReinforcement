// Fill out your copyright notice in the Description page of Project Settings.


#include "URSocket.h"


void AURSocket::BeginPlay()
{
	ConnectToGameServer();
}

void AURSocket::ConnectToGameServer()
{
	if (isConnected(connectionIdGameServer))
	{
		UE_LOG(LogTemp, Warning, TEXT("Log: Can't connect SECOND time. We're already connected!"));
		return;
	}
	FTcpSocketDisconnectDelegate disconnectDelegate;
	disconnectDelegate.BindDynamic(this, &AURSocket::OnDisconnected);
	FTcpSocketConnectDelegate connectDelegate;
	connectDelegate.BindDynamic(this, &AURSocket::OnConnected);
	FTcpSocketReceivedMessageDelegate receivedDelegate;
	receivedDelegate.BindDynamic(this, &AURSocket::OnMessageReceived);
	Connect("127.0.0.1", 9999, disconnectDelegate, connectDelegate, receivedDelegate, connectionIdGameServer);
}

void AURSocket::SendState(const TArray<uint8> data)
{
	SendData(connectionIdGameServer, data);
}


void AURSocket::OnConnected(int32 ConId)
{
	UE_LOG(LogTemp, Warning, TEXT("Log: Connected to server."));
}

void AURSocket::OnDisconnected(int32 ConId)
{
	UE_LOG(LogTemp, Warning, TEXT("Log: OnDisconnected."));
}

void AURSocket::OnMessageReceived(int32 ConId, TArray<uint8>& Message)
{
	UE_LOG(LogTemp, Log, TEXT("Log: Received message."));

	if (Message.Num() != 0)
	{
		const URPacket upk = static_cast<URPacket>(Message[0]);
		Message.RemoveAt(0);
		ReceiveDelegate.Broadcast(upk, Message);
	}
}
