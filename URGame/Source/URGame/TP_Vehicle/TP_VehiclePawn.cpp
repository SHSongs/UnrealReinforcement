// Copyright Epic Games, Inc. All Rights Reserved.

#include "TP_VehiclePawn.h"
#include "TP_VehicleWheelFront.h"
#include "TP_VehicleWheelRear.h"
#include "TP_VehicleHud.h"
#include "URSocket.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "WheeledVehicleMovementComponent4W.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/TextRenderComponent.h"
#include "Materials/Material.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

const FName ATP_VehiclePawn::LookUpBinding("LookUp");
const FName ATP_VehiclePawn::LookRightBinding("LookRight");

#define LOCTEXT_NAMESPACE "VehiclePawn"

#include "TcpSocketConnection.h"
#include "DrawDebugHelpers.h"

ATP_VehiclePawn::ATP_VehiclePawn()
{
	// Car mesh
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CarMesh(
		TEXT("/Game/Vehicle/Sedan/Sedan_SkelMesh.Sedan_SkelMesh"));
	GetMesh()->SetSkeletalMesh(CarMesh.Object);

	static ConstructorHelpers::FClassFinder<UObject> AnimBPClass(TEXT("/Game/Vehicle/Sedan/Sedan_AnimBP"));
	GetMesh()->SetAnimInstanceClass(AnimBPClass.Class);

	// Simulation
	UWheeledVehicleMovementComponent4W* Vehicle4W = CastChecked<UWheeledVehicleMovementComponent4W>(
		GetVehicleMovement());

	check(Vehicle4W->WheelSetups.Num() == 4);

	Vehicle4W->WheelSetups[0].WheelClass = UTP_VehicleWheelFront::StaticClass();
	Vehicle4W->WheelSetups[0].BoneName = FName("Wheel_Front_Left");
	Vehicle4W->WheelSetups[0].AdditionalOffset = FVector(0.f, -12.f, 0.f);

	Vehicle4W->WheelSetups[1].WheelClass = UTP_VehicleWheelFront::StaticClass();
	Vehicle4W->WheelSetups[1].BoneName = FName("Wheel_Front_Right");
	Vehicle4W->WheelSetups[1].AdditionalOffset = FVector(0.f, 12.f, 0.f);

	Vehicle4W->WheelSetups[2].WheelClass = UTP_VehicleWheelRear::StaticClass();
	Vehicle4W->WheelSetups[2].BoneName = FName("Wheel_Rear_Left");
	Vehicle4W->WheelSetups[2].AdditionalOffset = FVector(0.f, -12.f, 0.f);

	Vehicle4W->WheelSetups[3].WheelClass = UTP_VehicleWheelRear::StaticClass();
	Vehicle4W->WheelSetups[3].BoneName = FName("Wheel_Rear_Right");
	Vehicle4W->WheelSetups[3].AdditionalOffset = FVector(0.f, 12.f, 0.f);

	// Create a spring arm component
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm0"));
	SpringArm->TargetOffset = FVector(0.f, 0.f, 200.f);
	SpringArm->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 600.0f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 7.f;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll = false;

	// Create camera component 
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera0"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->FieldOfView = 90.f;

	// Create In-Car camera component 
	InternalCameraOrigin = FVector(0.0f, -40.0f, 120.0f);

	InternalCameraBase = CreateDefaultSubobject<USceneComponent>(TEXT("InternalCameraBase"));
	InternalCameraBase->SetRelativeLocation(InternalCameraOrigin);
	InternalCameraBase->SetupAttachment(GetMesh());

	InternalCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("InternalCamera"));
	InternalCamera->bUsePawnControlRotation = false;
	InternalCamera->FieldOfView = 90.f;
	InternalCamera->SetupAttachment(InternalCameraBase);

	//Setup TextRenderMaterial
	static ConstructorHelpers::FObjectFinder<UMaterial> TextMaterial(TEXT(
		"Material'/Engine/EngineMaterials/AntiAliasedTextMaterialTranslucent.AntiAliasedTextMaterialTranslucent'"));

	UMaterialInterface* Material = TextMaterial.Object;

	// Create text render component for in car speed display
	InCarSpeed = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IncarSpeed"));
	InCarSpeed->SetTextMaterial(Material);
	InCarSpeed->SetRelativeLocation(FVector(70.0f, -75.0f, 99.0f));
	InCarSpeed->SetRelativeRotation(FRotator(18.0f, 180.0f, 0.0f));
	InCarSpeed->SetupAttachment(GetMesh());
	InCarSpeed->SetRelativeScale3D(FVector(1.0f, 0.4f, 0.4f));

	// Create text render component for in car gear display
	InCarGear = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IncarGear"));
	InCarGear->SetTextMaterial(Material);
	InCarGear->SetRelativeLocation(FVector(66.0f, -9.0f, 95.0f));
	InCarGear->SetRelativeRotation(FRotator(25.0f, 180.0f, 0.0f));
	InCarGear->SetRelativeScale3D(FVector(1.0f, 0.4f, 0.4f));
	InCarGear->SetupAttachment(GetMesh());

	// Colors for the incar gear display. One for normal one for reverse
	GearDisplayReverseColor = FColor(255, 0, 0, 255);
	GearDisplayColor = FColor(255, 255, 255, 255);

	// Colors for the in-car gear display. One for normal one for reverse
	GearDisplayReverseColor = FColor(255, 0, 0, 255);
	GearDisplayColor = FColor(255, 255, 255, 255);

	bInReverseGear = false;
}

void ATP_VehiclePawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ATP_VehiclePawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATP_VehiclePawn::MoveRight);
	PlayerInputComponent->BindAxis("LookUp");
	PlayerInputComponent->BindAxis("LookRight");

	PlayerInputComponent->BindAction("Handbrake", IE_Pressed, this, &ATP_VehiclePawn::OnHandbrakePressed);
	PlayerInputComponent->BindAction("Handbrake", IE_Released, this, &ATP_VehiclePawn::OnHandbrakeReleased);
	PlayerInputComponent->BindAction("SwitchCamera", IE_Pressed, this, &ATP_VehiclePawn::OnToggleCamera);
}

void ATP_VehiclePawn::MoveForward(float Val)
{
	GetVehicleMovementComponent()->SetThrottleInput(Val);
}

void ATP_VehiclePawn::MoveRight(float Val)
{
	GetVehicleMovementComponent()->SetSteeringInput(Val);
}

void ATP_VehiclePawn::OnHandbrakePressed()
{
	GetVehicleMovementComponent()->SetHandbrakeInput(true);
}

void ATP_VehiclePawn::OnHandbrakeReleased()
{
	GetVehicleMovementComponent()->SetHandbrakeInput(false);
}

void ATP_VehiclePawn::OnToggleCamera()
{
	EnableIncarView(!bInCarCameraActive);
}

void ATP_VehiclePawn::EnableIncarView(const bool bState, const bool bForce)
{
	if ((bState != bInCarCameraActive) || (bForce == true))
	{
		bInCarCameraActive = bState;

		if (bState == true)
		{
			Camera->Deactivate();
			InternalCamera->Activate();
		}
		else
		{
			InternalCamera->Deactivate();
			Camera->Activate();
		}

		InCarSpeed->SetVisibility(bInCarCameraActive);
		InCarGear->SetVisibility(bInCarCameraActive);
	}
}


void ATP_VehiclePawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// Setup the flag to say we are in reverse gear
	bInReverseGear = GetVehicleMovement()->GetCurrentGear() < 0;

	// Update the strings used in the hud (incar and onscreen)
	UpdateHUDStrings();

	// Set the string in the incar hud
	SetupInCarHUD();

	const bool bHMDActive = false;

	if (bHMDActive == false)
	{
		if ((InputComponent) && (bInCarCameraActive == true))
		{
			FRotator HeadRotation = InternalCamera->GetRelativeRotation();
			HeadRotation.Pitch += InputComponent->GetAxisValue(LookUpBinding);
			HeadRotation.Yaw += InputComponent->GetAxisValue(LookRightBinding);
			InternalCamera->SetRelativeRotation(HeadRotation);
		}
	}

	StreeringMove();
	GetVehicleMovement()->SetThrottleInput(ForwardAxis);
}

void ATP_VehiclePawn::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> FoundActors;

	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(TEXT("socket")), FoundActors);
	if (FoundActors.Num() > 0)
	{
		URSocket = Cast<AURSocket>(FoundActors[0]);
		URSocket->ReceiveDelegate.AddDynamic(this, &ATP_VehiclePawn::Agent);
	}
	else
	UE_LOG(LogTemp, Error, TEXT("Not Found URSocket"));

	FoundActors.Empty();
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(TEXT("StartPoint")), FoundActors);
	if (FoundActors.Num() > 0)
	{
		StartPoint = FoundActors[0];
	}
	else
	UE_LOG(LogTemp, Error, TEXT("Not Found StartPoint"));
}


void ATP_VehiclePawn::UpdateHUDStrings()
{
	float KPH = FMath::Abs(GetVehicleMovement()->GetForwardSpeed()) * 0.036f;
	int32 KPH_int = FMath::FloorToInt(KPH);

	// Using FText because this is display text that should be localizable
	SpeedDisplayString = FText::Format(LOCTEXT("SpeedFormat", "{0} km/h"), FText::AsNumber(KPH_int));

	if (bInReverseGear == true)
	{
		GearDisplayString = FText(LOCTEXT("ReverseGear", "R"));
	}
	else
	{
		int32 Gear = GetVehicleMovement()->GetCurrentGear();
		GearDisplayString = (Gear == 0) ? LOCTEXT("N", "N") : FText::AsNumber(Gear);
	}
}


void ATP_VehiclePawn::SetupInCarHUD()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if ((PlayerController != nullptr) && (InCarSpeed != nullptr) && (InCarGear != nullptr))
	{
		// Setup the text render component strings
		InCarSpeed->SetText(SpeedDisplayString);
		InCarGear->SetText(GearDisplayString);

		if (bInReverseGear == false)
		{
			InCarGear->SetTextRenderColor(GearDisplayColor);
		}
		else
		{
			InCarGear->SetTextRenderColor(GearDisplayReverseColor);
		}
	}
}


void ATP_VehiclePawn::StreeringMove()
{
	GetVehicleMovement()->SetSteeringInput(RightAxis);
}

TArray<int32> ATP_VehiclePawn::LineTrace()
{
	TArray<int32> Distances;
	for (int i = -4; i < 5; i++)
	{
		FHitResult out;
		FVector ALoc = GetMesh()->GetComponentLocation();
		FRotator ARot = GetMesh()->GetComponentRotation();

		FVector Start = ALoc + ARot.RotateVector(FVector(260.f, 0, 60.f));
		FVector End = Start + ARot.RotateVector(FVector(1500.f, i * 250.f, 60.f));

		FCollisionQueryParams Params;
		const bool bResult = GetWorld()->LineTraceSingleByChannel(out, Start, End, ECC_Visibility, Params);

		float dis = (out.Location - Start).Size();
		Distances.Add(static_cast<int32>(dis));

#if ENABLE_DRAW_DEBUG

		FColor DrawColor = bResult ? FColor::Green : FColor::Red;

		DrawDebugLine(GetWorld(),
		              Start,
		              End,
		              DrawColor,
		              false,
		              0.1f);
#endif
	}
	return Distances;
}

TArray<uint8> ATP_VehiclePawn::Conv_IntArrToBytes(const TArray<int32> IntArr)
{
	TArray<uint8> Data;
	for (int32 i : IntArr)
	{
		Data.Append(ATcpSocketConnection::Conv_IntToBytes(i));
	}
	return Data;
}

void ATP_VehiclePawn::Agent(URPacket UrPacket, const TArray<uint8>& Byte_command)
{
	int Action = 0;
	switch (UrPacket)
	{
	case URPacket::Reset:
		SetActorLocationAndRotation(StartPoint->GetActorLocation(), FRotator(0.f, -90.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		GetVehicleMovement()->SetHandbrakeInput(true);

		URSocket->SendState(Conv_IntArrToBytes(LineTrace()));
		ForwardAxis = 0.f;
		RightAxis = 0.f;
		{
			FTimerHandle WaitHandle;
			float WaitTime = 0.5f;
			GetWorld()->GetTimerManager().SetTimer(WaitHandle, FTimerDelegate::CreateLambda([&]()
	        {
	            ForwardAxis = 1.f;
	            GetVehicleMovement()->SetHandbrakeInput(false);
	        }), WaitTime, false);
		}
		break;
	case URPacket::Step:
		Action = static_cast<int32>(Byte_command[0]);
		switch (Action)
		{
		case 1:
			RightAxis = 0.f;
			break;
		case 2:
			RightAxis = 0.5f;
			break;
		case 3:
			RightAxis = -0.5f;
			break;
		default:
			UE_LOG(LogTemp, Error, TEXT("Action Error"));
		}
		
		{
			FTimerHandle WaitHandle;
			float WaitTime = 0.1f;
			GetWorld()->GetTimerManager().SetTimer(WaitHandle, FTimerDelegate::CreateLambda([&]()
			{
				TArray<uint8> Data;

                if (Done == 1)
                {
	                Reward = -10;
                	Data.Append(DoneState);
                }
				else
				{
					Reward = 1;
					Data.Append(Conv_IntArrToBytes(LineTrace()));
				}
				Data.Append(ATcpSocketConnection::Conv_IntToBytes(Reward));
				Data.Append(ATcpSocketConnection::Conv_IntToBytes(Done));

				URSocket->SendState(Data);

				Reward = 0;
				Done = 0;
				
	        }), WaitTime, false);
		}
		
		break;
	default: ;
	}
}

#undef LOCTEXT_NAMESPACE
