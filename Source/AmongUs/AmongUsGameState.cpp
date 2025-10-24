#include "AmongUsGameState.h"
#include "AmongUsGameMode.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

AAmongUsGameState::AAmongUsGameState()
{
	bRolesAssigned = false;
	nbTache = 0;
	LobbyCountdown = 30;
	GameCountdown = 120;
}

void AAmongUsGameState::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// Plus de lancement automatique du timer ici
	if (World->GetMapName().EndsWith("Level"))
	{
		if (!GetWorldTimerManager().IsTimerActive(GameTimerHandle))
		{
			GetWorldTimerManager().SetTimer(
				GameTimerHandle,
				this,
				&AAmongUsGameState::GameCountdownTick,
				1.0f,
				true
			);
		}
	}
}

void AAmongUsGameState::LobbyCountdownTick()
{
	if (PlayerArray.Num() < 2)
	{
		GetWorldTimerManager().ClearTimer(LobbyTimerHandle);
		UE_LOG(LogTemp, Warning, TEXT("LobbyCountdown arrêté car moins de 2 joueurs."));
		return;
	}

	if (LobbyCountdown > 0)
	{
		LobbyCountdown--;
		UE_LOG(LogTemp, Warning, TEXT("LobbyCountdown = %d"), LobbyCountdown);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(LobbyTimerHandle);
		if (AAmongUsGameMode* GM = Cast<AAmongUsGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
		{
			GM->ChangeMap();
		}
	}
}

void AAmongUsGameState::GameCountdownTick()
{
	if (GameCountdown > 0)
	{
		GameCountdown--;
		UE_LOG(LogTemp, Warning, TEXT("GameCountdown = %d"), GameCountdown);
	}
	else
	{
		GetWorldTimerManager().ClearTimer(GameTimerHandle);
		if (AAmongUsGameMode* GM = Cast<AAmongUsGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
		{
			GM->ReturnToLobby();
		}
	}
}

void AAmongUsGameState::ServerModifyNbtache_Implementation(AAmongUsPlayerState* PlayerState)
{
	if (!PlayerState) return;

	switch (PlayerState->GetPlayerRole())
	{
	case EPlayerRole::Gentil:
		nbTache = FMath::Max(0, nbTache - 1);
		break;
	case EPlayerRole::Mechant:
		nbTache += 1;
		break;
	case EPlayerRole::Mort:
		break;
	}

	if (AAmongUsGameMode* GM = Cast<AAmongUsGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GM->CheckWinCondition();
	}
}

void AAmongUsGameState::StopGameCountdownTimer()
{
	GetWorldTimerManager().ClearTimer(GameTimerHandle);
}

void AAmongUsGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAmongUsGameState, nbTache);
	DOREPLIFETIME(AAmongUsGameState, bRolesAssigned);
	DOREPLIFETIME(AAmongUsGameState, LobbyCountdown);
	DOREPLIFETIME(AAmongUsGameState, GameCountdown);
}
