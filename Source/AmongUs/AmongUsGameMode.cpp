#include "AmongUsGameMode.h"
#include "AmongUsCharacter.h"
#include "AmongUsPlayerController.h"
#include "AmongUsGameState.h"
#include "AmongUsPlayerState.h"
#include "Bouton.h"
#include "TimerManager.h"
#include "Engine/World.h"

AAmongUsGameMode::AAmongUsGameMode()
{
    bHasMapChanged = false;
    GameStateClass = AAmongUsGameState::StaticClass();
    PlayerStateClass = AAmongUsPlayerState::StaticClass();
    DefaultPawnClass = AAmongUsCharacter::StaticClass();
    PlayerControllerClass = AAmongUsPlayerController::StaticClass();
}

void AAmongUsGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    UWorld* World = GetWorld();
    if (!World) return;

    FString CurrentMapName = World->GetMapName();

    // === LOBBY ===
    if (CurrentMapName.EndsWith("Lobby"))
    {
        AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
        if (!GS) return;

        int32 PlayerCount = GS->PlayerArray.Num();
        UE_LOG(LogTemp, Warning, TEXT("Nombre de joueurs connectés : %d"), PlayerCount);

        if (!bHasMapChanged && PlayerCount >= NumPlayersExpected)
        {
            bHasMapChanged = true;
            UE_LOG(LogTemp, Warning, TEXT("Nombre requis de joueurs atteint ! La map changera dans 3 secondes"));

            FTimerHandle TimerHandle;
            GetWorldTimerManager().SetTimer(TimerHandle, this, &AAmongUsGameMode::ChangeMap, 3.0f, false);
        }
    }

    // === LEVEL ===
    else if (CurrentMapName.EndsWith("Level"))
    {
        if (!HasAuthority()) return;

        AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
        if (!GS) return;

        int32 PlayerCount = GS->PlayerArray.Num();
        UE_LOG(LogTemp, Warning, TEXT("Arrivé dans le level avec %d joueurs"), PlayerCount);

        // Attendre que tous les joueurs soient présents
        if (!GS->bRolesAssigned && PlayerCount >= NumPlayersExpected)
        {
            // Générer le nombre de tâches
            GS->nbTache = FMath::RandRange(5, 10);
            UE_LOG(LogTemp, Warning, TEXT("Nombre de tâches généré : %d"), GS->nbTache);

            AssignRolesOnLevel();
            GS->bRolesAssigned = true;

            SpawnButtons();
        }
    }
}

void AAmongUsGameMode::ChangeMap()
{
    UWorld* World = GetWorld();
    if (World && World->GetNetMode() == NM_ListenServer)
    {
        FString MapPath = "/Game/Maps/Level";
        UE_LOG(LogTemp, Warning, TEXT("ServerTravel vers %s"), *MapPath);
        World->ServerTravel(MapPath + "?listen");
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
        if (MyPS)
            Players.Add(MyPS);
    }

    if (Players.Num() == 0) return;

    // Mélanger la liste des joueurs
    for (int32 i = 0; i < Players.Num(); i++)
    {
        int32 SwapIndex = FMath::RandRange(0, Players.Num() - 1);
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

    // Log des rôles
    for (AAmongUsPlayerState* PS : Players)
    {
        const TCHAR* RoleText = (PS->GetPlayerRole() == EPlayerRole::Gentil) ? TEXT("Gentil") :
                                TEXT("Mechant");
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
        FVector SpawnLocation(
            FMath::FRandRange(-500.f, 500.f),
            FMath::FRandRange(-500.f, 500.f),
            200.f
        );
        FRotator SpawnRotation = FRotator::ZeroRotator;

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        World->SpawnActor<ABouton>(ABouton::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
    }
}

// Dans ton GameMode
void AAmongUsGameMode::CheckWinCondition()
{
    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    if (!GS || !HasAuthority()) return;

    if (GS->nbTache <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Toutes les tâches terminées → WIN ! Retour au lobby"));

        // ServerTravel vers le Lobby pour tout le monde
        UWorld* World = GetWorld();
        if (World)
        {
            FString LobbyPath = "/Game/Maps/Lobby";
            World->ServerTravel(LobbyPath + "?listen");
        }
    }
}


