// Copyright Epic Games, Inc. All Rights Reserved.


#include "AmongUsPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "AmongUs.h"
#include "AmongUsCharacter.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AAmongUsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogAmongUs, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void AAmongUsPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (IsLocalPlayerController())
	{
		// Bind brut de la touche E
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AAmongUsPlayerController::OnInteractPressed);

		// Ajouter tes Input Mapping Contexts existants
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

void AAmongUsPlayerController::OnInteractPressed()
{
	AAmongUsCharacter* MyPawn = Cast<AAmongUsCharacter>(GetPawn());
	if (MyPawn)
	{
		MyPawn->TryInteract();
		UE_LOG(LogTemp, Warning, TEXT("E pressed: interaction triggered"));
	}
}

