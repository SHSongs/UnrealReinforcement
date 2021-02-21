// Copyright Epic Games, Inc. All Rights Reserved.

#include "TP_VehicleGameMode.h"
#include "TP_VehiclePawn.h"
#include "TP_VehicleHud.h"

ATP_VehicleGameMode::ATP_VehicleGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> PawnClass(TEXT("/Game/VehicleBP/Sedan/Sedan"));
	if (PawnClass.Succeeded())
		DefaultPawnClass = PawnClass.Class;

	HUDClass = ATP_VehicleHud::StaticClass();
}
