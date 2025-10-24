#include "AmongUsPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "AmongUsCharacter.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AAmongUsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("BeginPlay PlayerController"));

	if (QuitMenuWidgetClass)
	{
		QuitMenuWidgetInstance = CreateWidget<UUserWidget>(this, QuitMenuWidgetClass);
		if (!QuitMenuWidgetInstance)
		{
			UE_LOG(LogTemp, Error, TEXT("Impossible de créer le widget QuitMenuWidget"));
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("Widget QuitMenuWidget créé"));

		if (UButton* QuitButton = Cast<UButton>(QuitMenuWidgetInstance->GetWidgetFromName(TEXT("QuitButton"))))
		{
			UE_LOG(LogTemp, Warning, TEXT("QuitButton trouvé, binding OnClicked"));
			QuitButton->OnClicked.AddDynamic(this, &AAmongUsPlayerController::QuitGameClient);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("QuitButton non trouvé dans le widget ! Vérifie le nom dans le Blueprint"));
		}

		// Affiche le menu pour tester
		QuitMenuWidgetInstance->AddToViewport();
		UE_LOG(LogTemp, Warning, TEXT("Widget ajouté à la viewport"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("QuitMenuWidgetClass non défini dans le PlayerController"));
	}
}

void AAmongUsPlayerController::QuitGameClient()
{
	UE_LOG(LogTemp, Warning, TEXT("QuitGameClient appelé"));

	// Ferme le jeu côté client
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, true);
}

void AAmongUsPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (IsLocalPlayerController())
	{
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AAmongUsPlayerController::OnInteractPressed);

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
	if (AAmongUsCharacter* MyPawn = Cast<AAmongUsCharacter>(GetPawn()))
	{
		MyPawn->TryInteract();
	}
}
