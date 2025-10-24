#include "AmongUsGameState.h"
#include "AmongUsGameMode.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

AAmongUsGameState::AAmongUsGameState()
{
	bRolesAssigned = false;
	nbTache = 0;
	LobbyCountdown = 0;
	GameCountdown = 0;
}

void AAmongUsGameState::BeginPlay()
{
	Super::BeginPlay();
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

	AAmongUsGameMode* GM = Cast<AAmongUsGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GM)
	{
		GM->CheckWinCondition();
	}
}

void AAmongUsGameState::UpdateLobbyCountdown()
{
	if (LobbyCountdown > 0)
	{
		LobbyCountdown--;
	}
}

void AAmongUsGameState::UpdateGameCountdown()
{
	if (GameCountdown > 0)
	{
		GameCountdown--;
	}
}

void AAmongUsGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAmongUsGameState, nbTache);
	DOREPLIFETIME(AAmongUsGameState, LobbyCountdown);
	DOREPLIFETIME(AAmongUsGameState, GameCountdown);
}
