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
#include "Bouton.h"

AAmongUsCharacter::AAmongUsCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// ** Ajout du Rewindable Component **
	RewindCapsule = CreateDefaultSubobject<URewindableComponent>(TEXT("RewindCapsule"));
	// Attacher au RootComponent (CapsuleComponent du Character)
	RewindCapsule->SetupAttachment(GetCapsuleComponent()); 
	// S'assurer qu'il a la même taille que la capsule principale
	RewindCapsule->SetCapsuleRadius(GetCapsuleComponent()->GetScaledCapsuleRadius());
	RewindCapsule->SetCapsuleHalfHeight(GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

	// Camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
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
		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AAmongUsCharacter::Fire);
		}
		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAmongUsCharacter::Look);

	}
	else
	{
		UE_LOG(LogAmongUs, Error, TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
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

	// Utiliser la caméra comme point de départ pour le line trace
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
		{
			TargetPS = HitCharacter->GetPlayerState<AAmongUsPlayerState>();
		}
	}
	
	// 7. Envoi de la RPC au serveur
	if (TargetPS)
	{
		float ClientTimestamp = World->GetTimeSeconds();
		ServerConfirmHit(ClientTimestamp, StartLocation, CameraRotation, TargetPS);
		UE_LOG(LogTemp, Warning, TEXT("Client Fire: Hit %s, Sending RPC @ %.3f"), *TargetPS->GetPlayerName(), ClientTimestamp);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Client Fire: Missed."));
	}
}

void AAmongUsCharacter::ServerConfirmHit_Implementation(float ClientTimestamp, const FVector& StartLocation, const FRotator& Rotation, APlayerState* TargetPlayerState)
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 8. Demander au RewindSubsystem de vérifier le tir
	if (URewindSubsystem* RewindSubsystem = World->GetSubsystem<URewindSubsystem>())
	{
		// On caste le PlayerState reçu en AAmongUsPlayerState pour la compatibilité avec la Map du Subsystem
		if (AAmongUsPlayerState* TargetAmongUsPS = Cast<AAmongUsPlayerState>(TargetPlayerState))
		{
			if (RewindSubsystem->VerifyHit(ClientTimestamp, StartLocation, Rotation, TargetAmongUsPS))
			{
				// Tir confirmé après compensation de latence !
				UE_LOG(LogTemp, Warning, TEXT("Server: Rewind Hit Confirmed! Applying game logic."));
				// Ici, vous appliquez la logique de jeu (ex: dégâts, mort, etc.)
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Server: Rewind Hit Denied!"));
			}
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
			Btn->IncrementTaskServerOnly(MyPS); // ✅ Passe le PlayerState
		}
	}
}

