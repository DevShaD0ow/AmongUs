#include "AmongUsGameState.h"

#include "AmongUsGameMode.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "AmongUs.h"


AAmongUsGameState::AAmongUsGameState()
{
	bRolesAssigned = false;
	nbTache = 0;
}

void AAmongUsGameState::BeginPlay()
{
	Super::BeginPlay();
	// On ne génère plus nbTache ici pour éviter le problème
}

void AAmongUsGameState::ServerModifyNbtache_Implementation(AAmongUsPlayerState* PlayerState)
{
	if (!PlayerState) return;

	switch (PlayerState->GetPlayerRole())
	{
	case EPlayerRole::Gentil:
		nbTache = FMath::Max(0, nbTache - 1);
		UE_LOG(LogTemp, Warning, TEXT("Gentil a appuyé, nbTache = %d"), nbTache);
		break;
	case EPlayerRole::Mechant:
		nbTache += 1;
		UE_LOG(LogTemp, Warning, TEXT("Méchant a appuyé, nbTache = %d"), nbTache);
		break;
	case EPlayerRole::Mort:
		UE_LOG(LogTemp, Warning, TEXT("Mort ne peut pas interagir"));
		break;
	}
	AAmongUsGameMode* GM = Cast<AAmongUsGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (GM)
	{
		GM->CheckWinCondition();
	}
	OnRep_Nbtache();
}

void AAmongUsGameState::OnRep_Nbtache()
{
	UE_LOG(LogTemp, Warning, TEXT("nbTache répliqué = %d"), nbTache);
}

void AAmongUsGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAmongUsGameState, nbTache);
}
