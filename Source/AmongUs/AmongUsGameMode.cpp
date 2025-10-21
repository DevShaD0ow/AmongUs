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

    // === AJOUT ===
    GameDuration = 120.0f; // 2 minutes de jeu
}

void AAmongUsGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    UE_LOG(LogTemp, Warning, TEXT("PostLogin appelé pour %s"), *NewPlayer->GetName());

    UWorld* World = GetWorld();
    if (!World) return;

    FString CurrentMapName = World->GetMapName();

    // === LOBBY ===
    if (CurrentMapName.EndsWith("Lobby"))
    {
        AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
        if (!GS)return;

        int32 PlayerCount = GS->PlayerArray.Num();
        UE_LOG(LogTemp, Warning, TEXT("Nombre de joueurs connectés : %d"), PlayerCount);

        // Quand assez de joueurs, démarrer le compte à rebours
        if (!bHasMapChanged && PlayerCount >= NumPlayersExpected)
        {
            bHasMapChanged = true;
            GS->LobbyCountdown = static_cast<int32>(LobbyCountdownDuration);
            UE_LOG(LogTemp, Warning, TEXT("Début du compte à rebours du lobby (%d secondes)"), GS->LobbyCountdown);

            // Timer pour décrémenter le compte à rebours chaque seconde
            GetWorldTimerManager().SetTimer(
                LobbyCountdownTickHandle,
                [this, GS]()
                {
                    if (GS)
                    {
                        GS->UpdateLobbyCountdown();

                        if (GS->LobbyCountdown <= 0)
                        {
                            GetWorldTimerManager().ClearTimer(LobbyCountdownTickHandle);
                            UE_LOG(LogTemp, Warning, TEXT("Fin du compte à rebours du lobby — changement de map"));
                            ChangeMap();
                        }
                    }
                },
                1.0f, true);
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

            // === Compte à rebours partie (2 minutes) ===
            GS->GameCountdown = static_cast<int32>(GameDuration);
            UE_LOG(LogTemp, Warning, TEXT("Début du compte à rebours de partie (%d secondes)"), GS->GameCountdown);

            GetWorldTimerManager().SetTimer(
                GameCountdownTickHandle,
                [this, GS]()
                {
                    if (GS)
                    {
                        GS->UpdateGameCountdown();

                        if (GS->GameCountdown <= 0)
                        {
                            GetWorldTimerManager().ClearTimer(GameCountdownTickHandle);
                            UE_LOG(LogTemp, Warning, TEXT("Fin du compte à rebours de partie — retour au lobby"));
                            ReturnToLobby();
                        }
                    }
                },
                1.0f, true);
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

// === AJOUT : Retour automatique au lobby après la durée ===
void AAmongUsGameMode::ReturnToLobby()
{
    UE_LOG(LogTemp, Warning, TEXT("Fin du timer de partie → retour au lobby"));
    UWorld* World = GetWorld();
    if (World)
    {
        FString LobbyPath = "/Game/Maps/Lobby";
        World->ServerTravel(LobbyPath + "?listen");
    }
}

// === Déjà présent ===
void AAmongUsGameMode::CheckWinCondition()
{
    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    if (!GS || !HasAuthority()) return;

    if (GS->nbTache <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Toutes les tâches terminées → WIN ! Retour au lobby"));

        UWorld* World = GetWorld();
        if (World)
        {
            GetWorldTimerManager().ClearTimer(GameCountdownTickHandle);
            FString LobbyPath = "/Game/Maps/Lobby";
            World->ServerTravel(LobbyPath + "?listen");
        }
    }
}
