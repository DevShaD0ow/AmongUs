#include "AmongUsPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "AmongUsCharacter.h"
#include "AmongUsGameState.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "TimerManager.h"

// IMPORTANT : On inclut le header du Plugin pour configurer le menu
#include "Sessions/OTSessionMenu.h"

AAmongUsPlayerController::AAmongUsPlayerController()
{
	bReplicates = true;
}

void AAmongUsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FString MapName = GetWorld()->GetMapName();
	
	if (SessionMenuWidgetClass == nullptr){}
	

	if (MapName.Contains("MainMenu"))
	{
		ShowSessionMenu();
		bShowMouseCursor = true;
		SetInputMode(FInputModeUIOnly());
		return; 
	}
}

// === ONLINE TOOLBOX ===
void AAmongUsPlayerController::ShowSessionMenu()
{
	if (!SessionMenuWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ATTENTION : SessionMenuWidgetClass n'est pas assigné dans BP_AmongUsPlayerController !"));
		return;
	}

	// Création du Widget
	UUserWidget* WidgetInstance = CreateWidget<UUserWidget>(this, SessionMenuWidgetClass);
	
	if (WidgetInstance)
	{
		// On essaie de le caster en UOTSessionMenu pour utiliser la fonction de setup automatique
		if (UOTSessionMenu* SessionMenu = Cast<UOTSessionMenu>(WidgetInstance))
		{
			// MenuSetup gère : AddToViewport, Visibility, InputMode, MouseCursor
			SessionMenu->MenuSetup(true, true, true, true);
			UE_LOG(LogTemp, Log, TEXT("Menu Session affiché via OnlineToolbox."));
		}
		else
		{
			// Fallback si le cast échoue (mais ça ne devrait pas arriver si tu as mis le bon widget)
			WidgetInstance->AddToViewport();
			UE_LOG(LogTemp, Warning, TEXT("Le widget n'est pas un UOTSessionMenu, affichage standard."));
		}
	}
}

void AAmongUsPlayerController::QuitGameClient()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, true);
}

void AAmongUsPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (IsLocalPlayerController())
	{
		InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AAmongUsPlayerController::OnInteractPressed);
		InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AAmongUsPlayerController::ToggleQuitMenu);

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

// === Toggle Quit Menu ===
void AAmongUsPlayerController::ToggleQuitMenu()
{
	if (!QuitMenuWidgetClass) return;

	if (!QuitMenuWidgetInstance)
	{
		QuitMenuWidgetInstance = CreateWidget<UUserWidget>(this, QuitMenuWidgetClass);
		if (QuitMenuWidgetInstance)
		{
			if (UButton* QuitButton = Cast<UButton>(QuitMenuWidgetInstance->GetWidgetFromName(TEXT("QuitButton"))))
			{
				QuitButton->OnClicked.AddDynamic(this, &AAmongUsPlayerController::QuitGameClient);
			}
		}
	}

	if (!QuitMenuWidgetInstance) return;

	bool bIsVisible = QuitMenuWidgetInstance->IsVisible();
	QuitMenuWidgetInstance->SetVisibility(bIsVisible ? ESlateVisibility::Hidden : ESlateVisibility::Visible);

	if (!bIsVisible)
	{
		QuitMenuWidgetInstance->AddToViewport(10);
		bShowMouseCursor = true;
		SetInputMode(FInputModeUIOnly());
	}
	else
	{
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
}

// === NETWORK CLOCK ===
float AAmongUsPlayerController::GetServerWorldTimeDelta() const
{
	return ServerWorldTimeDelta;
}

float AAmongUsPlayerController::GetServerWorldTime() const
{
	return GetWorld()->GetTimeSeconds() + ServerWorldTimeDelta;
}

void AAmongUsPlayerController::PostNetInit()
{
	Super::PostNetInit();

	if (GetLocalRole() != ROLE_Authority)
	{
		RequestWorldTime_Internal();
		if (NetworkClockUpdateFrequency > 0.f)
		{
			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(TimerHandle, this, &AAmongUsPlayerController::RequestWorldTime_Internal, NetworkClockUpdateFrequency, true);
		}
	}
}

void AAmongUsPlayerController::RequestWorldTime_Internal()
{
	ServerRequestWorldTime(GetWorld()->GetTimeSeconds());
}

void AAmongUsPlayerController::ClientUpdateWorldTime_Implementation(float ClientTimestamp, float ServerTimestamp)
{
	const float RoundTripTime = GetWorld()->GetTimeSeconds() - ClientTimestamp;
	RTTCircularBuffer.Add(RoundTripTime);

	float AdjustedRTT = 0.f;
	if (RTTCircularBuffer.Num() >= 10)
	{
		TArray<float> Tmp = RTTCircularBuffer;
		Tmp.Sort();
		for (int i = 1; i < 9; ++i)
			AdjustedRTT += Tmp[i];
		AdjustedRTT /= 8.f;
		RTTCircularBuffer.RemoveAt(0);
	}
	else
	{
		AdjustedRTT = RoundTripTime;
	}

	ServerWorldTimeDelta = ServerTimestamp - ClientTimestamp - (AdjustedRTT / 2.f);
}

void AAmongUsPlayerController::UpdateClientCountdowns()
{
	if (!IsValid(this)) return;

	UWorld* World = GetWorld();
	if (!World) return;

	AAmongUsGameState* GS = World->GetGameState<AAmongUsGameState>();
	if (!GS) return;

	UE_LOG(LogTemp, Warning, TEXT("[CLIENT LOG] LobbyCountdown=%d GameCountdown=%d ServerTime=%.2f"),
		GS->LobbyCountdown, GS->GameCountdown, GetServerWorldTime());
}

void AAmongUsPlayerController::ServerRequestWorldTime_Implementation(float ClientTimestamp)
{
	const float Timestamp = GetWorld()->GetTimeSeconds();
	ClientUpdateWorldTime(ClientTimestamp, Timestamp);
}