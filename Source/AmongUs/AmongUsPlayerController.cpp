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

AAmongUsPlayerController::AAmongUsPlayerController()
{
	bReplicates = true;
}

void AAmongUsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Vérifie si on a un Pawn
	if (GetPawn())
	{
		UE_LOG(LogTemp, Warning, TEXT("Pawn possédé: %s"), *GetPawn()->GetName());
	}
	else
	{
		// Delay léger pour attendre possession
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			if (GetPawn())
			{
				UE_LOG(LogTemp, Warning, TEXT("Pawn possédé (après délai): %s"), *GetPawn()->GetName());
			}
		}, 0.1f, false);
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

		// Bind touche F pour afficher/masquer le widget Quit
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
	UE_LOG(LogTemp, Warning, TEXT("ToggleQuitMenu appelé"));

	if (!QuitMenuWidgetInstance)
	{
		if (!QuitMenuWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("QuitMenuWidgetClass NULL, tentative de chargement dynamique"));
			QuitMenuWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/UI/WBP_Menu.WBP_Menu_C"));
			if (!QuitMenuWidgetClass)
			{
				UE_LOG(LogTemp, Error, TEXT("Impossible de charger le widget à ce chemin : /Game/UI/WBP_Menu.WBP_Menu_C"));
				return;
			}
		}

		QuitMenuWidgetInstance = CreateWidget<UUserWidget>(this, QuitMenuWidgetClass);
		if (QuitMenuWidgetInstance)
		{
			if (UButton* QuitButton = Cast<UButton>(QuitMenuWidgetInstance->GetWidgetFromName(TEXT("QuitButton"))))
			{
				QuitButton->OnClicked.AddDynamic(this, &AAmongUsPlayerController::QuitGameClient);
			}
			UE_LOG(LogTemp, Warning, TEXT("ToggleQuitMenu: QuitMenuWidgetInstance créé avec succès"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Impossible de créer QuitMenuWidgetInstance"));
			return;
		}
	}

	bool bIsVisible = QuitMenuWidgetInstance->IsVisible();
	QuitMenuWidgetInstance->SetVisibility(bIsVisible ? ESlateVisibility::Hidden : ESlateVisibility::Visible);

	if (!bIsVisible)
	{
		QuitMenuWidgetInstance->AddToViewport(10);

		// === Affiche le curseur et change l'input mode ===
		bShowMouseCursor = true;
		SetInputMode(FInputModeUIOnly());
	}
	else
	{
		// === Masque le curseur et repasse en input jeu ===
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}

	UE_LOG(LogTemp, Warning, TEXT("ToggleQuitMenu: Widget %s"), bIsVisible ? TEXT("masqué") : TEXT("affiché"));
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
