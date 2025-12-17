// Copyright Epic Games, Inc. All Rights Reserved.

#include "AmongUsCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "AmongUs/RewindSubsystem.h"
#include "InputActionValue.h"
#include "AmongUs.h"
#include "AmongUsGameMode.h"
#include "AmongUsPlayerController.h"
#include "Bouton.h"

AAmongUsCharacter::AAmongUsCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	RewindCapsule = CreateDefaultSubobject<URewindableComponent>(TEXT("RewindCapsule"));
	RewindCapsule->SetupAttachment(GetCapsuleComponent()); 
	RewindCapsule->SetCapsuleRadius(GetCapsuleComponent()->GetScaledCapsuleRadius());
	RewindCapsule->SetCapsuleHalfHeight(GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AAmongUsCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	AAmongUsPlayerState* PS = GetPlayerState<AAmongUsPlayerState>();
	if (PS)
	{
		ApplyColorToSkin(PS->PlayerColor);
		UE_LOG(LogTemp, Warning, TEXT("Client : PlayerState reçu ! Couleur appliquée : %d"), (int32)PS->PlayerColor);
	}
}

void AAmongUsCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AAmongUsPlayerState* PS = GetPlayerState<AAmongUsPlayerState>();
	if (PS)
	{
		ApplyColorToSkin(PS->PlayerColor);
	}
}

void AAmongUsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAmongUsCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AAmongUsCharacter::Look);

		//Firing
		PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AAmongUsCharacter::Fire);
		
		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAmongUsCharacter::Look);

	}
}

void AAmongUsCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void AAmongUsCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AAmongUsCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AAmongUsCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AAmongUsCharacter::Fire()
{
    UWorld* World = GetWorld();
    if (!World || !IsLocallyControlled()) return;

    FVector StartLocation = FollowCamera->GetComponentLocation();
    FRotator CameraRotation = FollowCamera->GetComponentRotation();
    FVector EndLocation = StartLocation + (CameraRotation.Vector() * 100000.f);

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bHit = World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Pawn, Params);

    AAmongUsPlayerState* TargetPS = nullptr;
    if (bHit)
    {
        if (AAmongUsCharacter* HitCharacter = Cast<AAmongUsCharacter>(HitResult.GetActor()))
        	TargetPS = HitCharacter->GetPlayerState<AAmongUsPlayerState>();
        
    }
    if (TargetPS)
    {
        AAmongUsPlayerController* PC = Cast<AAmongUsPlayerController>(GetController());
        float EstimatedServerTime = PC ? PC->GetServerWorldTime() : World->GetTimeSeconds();
        
        ServerConfirmHit(EstimatedServerTime, StartLocation, CameraRotation, TargetPS);
        UE_LOG(LogTemp, Warning, TEXT("Client Fire: Hit %s, Sending RPC @ %.3f"), 
            *TargetPS->GetPlayerName(), EstimatedServerTime);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Client Fire: Missed."));
    }
}

void AAmongUsCharacter::ServerConfirmHit_Implementation(
    float ClientTimestamp,
    const FVector& StartLocation,
    const FRotator& Rotation,
    APlayerState* TargetPlayerState)
{
    UWorld* World = GetWorld();
    if (!World) return;
    
	float ServerTime = World->GetTimeSeconds();
	float TimeDifference = FMath::Abs(ServerTime - ClientTimestamp);
    
    const float MaxAcceptableTimeDiff = 0.5f;
    if (TimeDifference > MaxAcceptableTimeDiff)
    {
        UE_LOG(LogTemp, Warning, TEXT("Server: Time difference too large (%.3fs), rejecting shot"), TimeDifference);
        return;
    }
    float RewindTime = ClientTimestamp;

    if (!TargetPlayerState)
    {
        UE_LOG(LogTemp, Warning, TEXT("Server: Rewind Hit Denied! No TargetPlayerState"));
        return;
    }

    AAmongUsPlayerState* ShooterPS = GetPlayerState<AAmongUsPlayerState>();
    if (!ShooterPS)return;
    

    UE_LOG(LogTemp, Warning, TEXT("Server: Tireur=%s vise Target=%s à t=%.3f (Server t=%.3f, diff=%.3fms)"), 
        *ShooterPS->GetPlayerName(), 
        *TargetPlayerState->GetPlayerName(), 
        RewindTime,
        ServerTime,
        TimeDifference * 1000.f);

    AAmongUsPlayerState* TargetAmongUsPS = Cast<AAmongUsPlayerState>(TargetPlayerState);
    if (!TargetAmongUsPS)return;
	
    AAmongUsCharacter* TargetCharacter = Cast<AAmongUsCharacter>(TargetAmongUsPS->GetPawn());
    if (!TargetCharacter || !TargetCharacter->RewindCapsule) return;
	
    if (URewindSubsystem* RewindSubsystem = World->GetSubsystem<URewindSubsystem>())
    {
        if (RewindSubsystem->VerifyHit(RewindTime, StartLocation, Rotation, TargetAmongUsPS, ShooterPS))
        {
            UE_LOG(LogTemp, Warning, TEXT("✅ Server: Rewind Hit CONFIRMED! %s a touché %s"), 
                *ShooterPS->GetPlayerName(), *TargetAmongUsPS->GetPlayerName());
            
        	TargetCharacter->MulticastTriggerDeath();
        	TargetAmongUsPS->SetPlayerRole(EPlayerRole::Mort);

        	if (AAmongUsGameMode* GM = Cast<AAmongUsGameMode>(GetWorld()->GetAuthGameMode()))
        	{
        		GM->CheckWinCondition();
        	}
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ Server: Rewind Hit DENIED!"));
        }
    }
}


void AAmongUsCharacter::DoJumpStart()
{
	Jump();
}

void AAmongUsCharacter::DoJumpEnd()
{
	StopJumping();
}

void AAmongUsCharacter::TryInteract()
{
	FVector PlayerLocation = GetActorLocation();
	TArray<FHitResult> Hits;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(InteractionDistance);

	bool bHit = GetWorld()->SweepMultiByChannel(Hits, PlayerLocation, PlayerLocation, FQuat::Identity, ECC_Pawn, Sphere);

	if (bHit)
	{
		for (auto& Hit : Hits)
		{
			ABouton* Btn = Cast<ABouton>(Hit.GetActor());
			if (Btn)
			{
				ServerInteractWithButton(Btn);
				break;
			}
		}
	}
}

void AAmongUsCharacter::ServerInteractWithButton_Implementation(ABouton* Btn)
{
	if (Btn)
	{
		AAmongUsPlayerState* MyPS = GetPlayerState<AAmongUsPlayerState>();
		if (MyPS)
		{
			Btn->IncrementTaskServerOnly(MyPS); 
		}
	}
}

void AAmongUsCharacter::MulticastTriggerDeath_Implementation()
{
	// 1. Désactiver le contrôle de mouvement (très important pour le client local)
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->SetComponentTickEnabled(false);

	// 2. Désactiver la capsule de collision (pour ne pas que le corps rebondisse dessus)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3. Activer le Ragdoll sur le Mesh
	if (GetMesh())
	{
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);
        
		// Petit boost pour faire tomber le corps de façon plus naturelle
		GetMesh()->AddImpulse(FVector(0, 0, -100.f), NAME_None, true); 
	}
}
void AAmongUsCharacter::ApplyColorToSkin(EPlayerColor Color)
{
	USkeletalMeshComponent* MyMesh = GetMesh();
	if (!MyMesh) return;

	const int32 MaterialIndex = 0; 

	// Créer un Material Instance Dynamic (MID) si ce n'est pas déjà fait
	// Cela permet de changer les paramètres sans changer le fichier asset d'origine
	UMaterialInterface* CurrentMat = MyMesh->GetMaterial(MaterialIndex);
	UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(CurrentMat);

	if (!DynMat)
	{
		DynMat = MyMesh->CreateDynamicMaterialInstance(MaterialIndex);
	}

	if (DynMat)
	{
		FLinearColor SelectedColor = GetLinearColorFromEnum(Color);
		// "PlayerColor" doit être EXACTEMENT le nom du paramètre dans M_PlayerColor
		DynMat->SetVectorParameterValue(FName("PlayerColor"), SelectedColor);
	}
}

FLinearColor AAmongUsCharacter::GetLinearColorFromEnum(EPlayerColor Color)
{
	switch (Color)
	{
	case EPlayerColor::Red: return FLinearColor::Red;
	case EPlayerColor::Blue: return FLinearColor::Blue;
	case EPlayerColor::Green: return FLinearColor::Green;
	case EPlayerColor::Yellow: return FLinearColor::Yellow;
	case EPlayerColor::Purple: return FLinearColor(0.5f, 0.0f, 0.5f);
	case EPlayerColor::Cyan: return FLinearColor(0.0f, 1.0f, 1.0f);
	case EPlayerColor::Orange: return FLinearColor(1.0f, 0.5f, 0.0f);
	case EPlayerColor::Pink: return FLinearColor(1.0f, 0.07f, 0.57f);
	default: return FLinearColor::White;
	}
}
