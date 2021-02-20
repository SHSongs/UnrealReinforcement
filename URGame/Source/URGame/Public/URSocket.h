// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TcpSocketConnection.h"
#include "URSocket.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReceiveDelegate, uint8, URPacket, const TArray<uint8>&, Message);



UCLASS()
class URGAME_API AURSocket : public ATcpSocketConnection
{
	GENERATED_BODY()

private:
	virtual void BeginPlay() override;
	
public:
	UPROPERTY(BlueprintAssignable)
	FReceiveDelegate ReceiveDelegate;
	
	UFUNCTION()
    void OnConnected(int32 ConnectionId);

	UFUNCTION()
    void OnDisconnected(int32 ConId);

	UFUNCTION()
    void OnMessageReceived(int32 ConId, TArray<uint8>& Message);
  
	UFUNCTION(BlueprintCallable)
  void ConnectToGameServer();
	
	UPROPERTY()
	int32 connectionIdGameServer;

};
