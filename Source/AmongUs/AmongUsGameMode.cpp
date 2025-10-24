#include "AmongUsGameMode.h"
#include "AmongUsCharacter.h"
#include "AmongUsPlayerController.h"
#include "AmongUsGameState.h"
#include "AmongUsPlayerState.h"
#include "Bouton.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AAmongUsGameMode::AAmongUsGameMode()
{
	bHasMapChanged = false;
	GameStateClass = AAmongUsGameState::StaticClass();
	PlayerStateClass = AAmongUsPlayerState::StaticClass();
	DefaultPawnClass = AAmongUsCharacter::StaticClass();
}

void AAmongUsGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UWorld* World = GetWorld();
	if (!World) return;

	FString CurrentMapName = World->GetMapName();
	AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
	if (!GS) return;

	int32 PlayerCount = GS->PlayerArray.Num();
	UE_LOG(LogTemp, Warning, TEXT("PostLogin appelé pour %s, PlayerCount=%d, Map=%s"), 
		*NewPlayer->GetName(), PlayerCount, *CurrentMapName);

	// === LOBBY ===
	if (CurrentMapName.EndsWith("Lobby"))
	{
		if (!bHasMapChanged && PlayerCount >= NumPlayersExpected)
		{
			bHasMapChanged = true;
			if (HasAuthority())
			{
				GS->LobbyCountdown = static_cast<int32>(LobbyCountdownDuration);
				UE_LOG(LogTemp, Warning, TEXT("Début countdown lobby : %d secondes"), GS->LobbyCountdown);

				GetWorldTimerManager().SetTimer(
					LobbyCountdownTickHandle,
					[this, GS]()
					{
						if (GS)
						{
							GS->UpdateLobbyCountdown();
							UE_LOG(LogTemp, Warning, TEXT("LobbyCountdown = %d"), GS->LobbyCountdown);

							if (GS->LobbyCountdown <= 0)
							{
								GetWorldTimerManager().ClearTimer(LobbyCountdownTickHandle);
								UE_LOG(LogTemp, Warning, TEXT("Lobby terminé → changement de map"));
								ChangeMap();
							}
						}
					},
					1.0f, true
				);
			}
		}
	}

	// === LEVEL ===
	else if (CurrentMapName.EndsWith("Level"))
	{
		if (!HasAuthority()) return;

		if (!GS->bRolesAssigned && PlayerCount >= NumPlayersExpected)
		{
			GS->nbTache = FMath::RandRange(5, 10);
			UE_LOG(LogTemp, Warning, TEXT("Nombre de tâches généré : %d"), GS->nbTache);

			AssignRolesOnLevel();
			GS->bRolesAssigned = true;

			SpawnButtons();

			GS->GameCountdown = static_cast<int32>(GameDuration);
			UE_LOG(LogTemp, Warning, TEXT("Début countdown partie : %d secondes"), GS->GameCountdown);

			GetWorldTimerManager().SetTimer(
				GameCountdownTickHandle,
				[this, GS]()
				{
					if (GS)
					{
						GS->UpdateGameCountdown();
						UE_LOG(LogTemp, Warning, TEXT("GameCountdown = %d"), GS->GameCountdown);

						if (GS->GameCountdown <= 0)
						{
							GetWorldTimerManager().ClearTimer(GameCountdownTickHandle);
							UE_LOG(LogTemp, Warning, TEXT("Fin de partie → retour au lobby"));
							ReturnToLobby();
						}
					}
				},
				1.0f, true
			);
		}
	}
}

void AAmongUsGameMode::ChangeMap()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer)
	{
		FString MapPath = "/Game/Maps/Level?listen";
		UE_LOG(LogTemp, Warning, TEXT("ServerTravel vers %s"), *MapPath);
		World->ServerTravel(MapPath);
	}
}

void AAmongUsGameMode::AssignRolesOnLevel()
{
	AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
	if (!GS) return;

	TArray<AAmongUsPlayerState*> Players;
	for (APlayerState* PS : GS->PlayerArray)
	{
		AAmongUsPlayerState* MyPS = Cast<AAmongUsPlayerState>(PS);
		if (MyPS) Players.Add(MyPS);
	}

	if (Players.Num() == 0) return;

	// Mélanger la liste
	for (int32 i = 0; i < Players.Num(); i++)
	{
		int32 SwapIndex = FMath::RandRange(0, Players.Num() - 1);
		Players.Swap(i, SwapIndex);
	}

	int32 NumImpostors = (Players.Num() > 6) ? 2 : 1;
	for (int32 i = 0; i < NumImpostors; i++) Players[i]->SetPlayerRole(EPlayerRole::Mechant);
	for (int32 i = NumImpostors; i < Players.Num(); i++) Players[i]->SetPlayerRole(EPlayerRole::Gentil);

	for (AAmongUsPlayerState* PS : Players)
	{
		const TCHAR* RoleText = (PS->GetPlayerRole() == EPlayerRole::Gentil) ? TEXT("Gentil") : TEXT("Mechant");
		UE_LOG(LogTemp, Warning, TEXT("Joueur %s est %s"), *PS->GetPlayerName(), RoleText);
	}
}

void AAmongUsGameMode::SpawnButtons()
{
	UWorld* World = GetWorld();
	if (!World || !HasAuthority()) return;

	AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
	if (!GS || GS->nbTache <= 0) return;

	for (int32 i = 0; i < GS->nbTache; i++)
	{
		FVector SpawnLocation(FMath::FRandRange(-500.f, 500.f), FMath::FRandRange(-500.f, 500.f), 200.f);
		FRotator SpawnRotation = FRotator::ZeroRotator;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		World->SpawnActor<ABouton>(ABouton::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
	}
}

void AAmongUsGameMode::ReturnToLobby()
{
	UE_LOG(LogTemp, Warning, TEXT("Retour au lobby"));
	UWorld* World = GetWorld();
	if (World)
	{
		FString LobbyPath = "/Game/Maps/Lobby?listen";
		World->ServerTravel(LobbyPath);
	}
}

void AAmongUsGameMode::CheckWinCondition()
{
	AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
	if (!GS || !HasAuthority()) return;

	if (GS->nbTache <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Toutes les tâches terminées → WIN ! Retour au lobby"));
		GetWorldTimerManager().ClearTimer(GameCountdownTickHandle);
		ReturnToLobby();
	}
}
