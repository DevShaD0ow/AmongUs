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

	AAmongUsPlayerController* PC = Cast<AAmongUsPlayerController>(NewPlayer);
	if (!PC) return;

	AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
	if (!GS) return;

	int32 PlayerCount = GS->PlayerArray.Num();

	UWorld* World = GetWorld();
	if (!World) return;

	if (World->GetMapName().EndsWith("Lobby"))
	{
		// Lancer le timer seulement si au moins 2 joueurs et pas déjà actif
		if (PlayerCount >= 2 && !GetWorldTimerManager().IsTimerActive(GS->LobbyTimerHandle) && HasAuthority())
		{
			GS->LobbyCountdown = static_cast<int32>(LobbyCountdownDuration);

			GetWorldTimerManager().SetTimer(
				GS->LobbyTimerHandle,
				GS,
				&AAmongUsGameState::LobbyCountdownTick,
				1.0f,
				true
			);

			UE_LOG(LogTemp, Warning, TEXT("LobbyCountdown lancé avec %d joueurs !"), PlayerCount);
		}else GS->LobbyCountdown = 0;
	}
	else if (World->GetMapName().EndsWith("Level"))
	{
		if (!HasAuthority() || GS->bRolesAssigned) return;

		GS->nbTache = FMath::RandRange(5, 10);
		GS->bRolesAssigned = true;

		FTimerHandle RoleAssignTimerHandle;
		GetWorldTimerManager().SetTimer(
			RoleAssignTimerHandle,
			[this]()
			{
				AssignRolesOnLevel();
				SpawnButtons();
			},
			0.5f,
			false
		);

		GS->GameCountdown = static_cast<int32>(GameDuration);
	}
}


void AAmongUsGameMode::ChangeMap()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer)
	{
		FString MapPath = "/Game/Maps/Level?listen";
		World->ServerTravel(MapPath);
	}
}

void AAmongUsGameMode::AssignRolesOnLevel()
{
	AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
	if (!GS) return;

	TArray<AAmongUsPlayerState*> Players;

	// Récupérer tous les PlayerState du GameState
	for (APlayerState* PS : GS->PlayerArray)
	{
		AAmongUsPlayerState* MyPS = Cast<AAmongUsPlayerState>(PS);
		if (MyPS)
			Players.Add(MyPS);
	}

	if (Players.Num() == 0) return;

	// --- Affichage de la liste des joueurs avant attribution ---
	UE_LOG(LogTemp, Warning, TEXT("Liste des joueurs avant attribution des rôles :"));
	for (int32 i = 0; i < Players.Num(); i++)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerNum %d : %s"), i, *Players[i]->GetPlayerName());
	}

	// Initialiser le random (optionnel mais plus sûr)
	FMath::RandInit(FDateTime::Now().GetMillisecond());

	// --- Fisher-Yates shuffle pour un vrai mélange ---
	for (int32 i = Players.Num() - 1; i > 0; i--)
	{
		int32 SwapIndex = FMath::RandRange(0, i);
		Players.Swap(i, SwapIndex);
	}

	// Déterminer le nombre d'imposteurs
	int32 NumImpostors = (Players.Num() > 6) ? 2 : 1;

	// Assigner les imposteurs
	for (int32 i = 0; i < NumImpostors; i++)
	{
		Players[i]->SetPlayerRole(EPlayerRole::Mechant);
	}

	// Assigner le reste comme Gentil
	for (int32 i = NumImpostors; i < Players.Num(); i++)
	{
		Players[i]->SetPlayerRole(EPlayerRole::Gentil);
	}

	// --- Log des rôles après attribution ---
	UE_LOG(LogTemp, Warning, TEXT("Rôles attribués :"));
	for (int32 i = 0; i < Players.Num(); i++)
	{
		const TCHAR* RoleText = (Players[i]->GetPlayerRole() == EPlayerRole::Gentil) ? TEXT("Gentil") :
								TEXT("Mechant");
		UE_LOG(LogTemp, Warning, TEXT("PlayerNum %d : Joueur %s est %s"), i, *Players[i]->GetPlayerName(), RoleText);
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
		FRotator SpawnRotation(0.f, 0.f, 0.f);
		World->SpawnActor<ABouton>(ABouton::StaticClass(), SpawnLocation, SpawnRotation);
	}
}

void AAmongUsGameMode::ReturnToLobby()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FString MapPath = "/Game/Maps/Lobby?listen";
	World->ServerTravel(MapPath);
}

void AAmongUsGameMode::CheckWinCondition()
{
	AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
	if (!GS || !HasAuthority()) return;

	if (GS->nbTache <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Toutes les tâches terminées → WIN ! Retour au lobby"));
		GS->StopGameCountdownTimer();
		ReturnToLobby();
	}
}
