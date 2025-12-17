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
    ExpectedPlayerCount = 0;
    
    bUseSeamlessTravel = true;
}

void AAmongUsGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    ExpectedPlayerCount = UGameplayStatics::GetIntOption(Options, TEXT("ExpectedPlayers"), 0);
}

void AAmongUsGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    AAmongUsPlayerController* PC = Cast<AAmongUsPlayerController>(NewPlayer);
    if (!PC) return;

    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    if (!GS) return;

    int32 CurrentPlayerCount = GS->PlayerArray.Num();
    UWorld* World = GetWorld();
    if (!World) return;

    // --- LOGIQUE MAP LOBBY ---
    if (World->GetMapName().EndsWith("Lobby"))
    {
        if (CurrentPlayerCount >= 2 && !GetWorldTimerManager().IsTimerActive(GS->LobbyTimerHandle))
        {
            GS->LobbyCountdown = 10; // 10 secondes avant le départ
            GetWorldTimerManager().SetTimer(GS->LobbyTimerHandle, GS, &AAmongUsGameState::LobbyCountdownTick, 1.0f, true);
            UE_LOG(LogTemp, Warning, TEXT("TEST MODE: Lancement auto du Lobby avec %d joueurs !"), CurrentPlayerCount);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Lobby : Joueur connecté (%d). En attente de 2 joueurs..."), CurrentPlayerCount);
        }
    }
    // --- LOGIQUE MAP LEVEL (Jeu Principal) ---
    else if (World->GetMapName().EndsWith("Level"))
    {
        // On vérifie que les rôles ne sont pas déjà donnés
        if (!HasAuthority() || GS->bRolesAssigned) return;
        
        if (CurrentPlayerCount >= ExpectedPlayerCount) 
        {
            UE_LOG(LogTemp, Warning, TEXT("Level : Tous les joueurs sont là. Lancement du jeu !"));

            // 1. Initialiser les variables de jeu
            GS->nbTache = FMath::RandRange(5, 10);
            GS->bRolesAssigned = true;

            FTimerHandle RoleAssignTimerHandle;
            GetWorldTimerManager().SetTimer(RoleAssignTimerHandle, [this]()
            {
                AssignRolesOnLevel();
                SpawnButtons();
            }, 1.0f, false);

            GS->GameCountdown = static_cast<int32>(GameDuration);
            
            // Démarrer le tick du timer dans le GameState
            if (!GetWorldTimerManager().IsTimerActive(GS->GameTimerHandle))
            {
                GetWorldTimerManager().SetTimer(GS->GameTimerHandle, GS, &AAmongUsGameState::GameCountdownTick, 1.0f, true);
            }
        }
        else
        {
             UE_LOG(LogTemp, Warning, TEXT("Level : En attente de joueurs... (%d/%d)"), CurrentPlayerCount, ExpectedPlayerCount);
        }
    }
}

void AAmongUsGameMode::CheckAllPlayersReady()
{
    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    if (!GS) return;

    // 1. On ne lance rien s'il n'y a pas assez de joueurs (minimum 2 ou ExpectedPlayers)
    if (GS->PlayerArray.Num() < 2) return; 

    // 2. Vérifier si TOUT LE MONDE est prêt
    bool bAllReady = true;
    for (APlayerState* PS : GS->PlayerArray)
    {
        AAmongUsPlayerState* MyPS = Cast<AAmongUsPlayerState>(PS);
        if (MyPS && !MyPS->bIsReady) 
        {
            bAllReady = false;
            break;
        }
    }

    // 3. Gestion du Timer
    bool bTimerActive = GetWorldTimerManager().IsTimerActive(GS->LobbyTimerHandle);

    if (bAllReady && !bTimerActive)
    {
        // TOUT LE MONDE EST PRÊT -> Lancer le décompte (ex: 5 secondes)
        GS->LobbyCountdown = 5; 
        GetWorldTimerManager().SetTimer(GS->LobbyTimerHandle, GS, &AAmongUsGameState::LobbyCountdownTick, 1.0f, true);
        UE_LOG(LogTemp, Warning, TEXT("Tout le monde est prêt ! Lancement dans 5s"));
    }
    else if (!bAllReady && bTimerActive)
    {
        // QUELQU'UN N'EST PLUS PRÊT -> Annuler le lancement
        GetWorldTimerManager().ClearTimer(GS->LobbyTimerHandle);
        GS->LobbyCountdown = 30; // Reset visuel
        UE_LOG(LogTemp, Warning, TEXT("Lancement annulé : un joueur n'est plus prêt."));
    }
}

void AAmongUsGameMode::ChangeMap()
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer)
    {
        int32 PlayersToSend = 0;
        if (AAmongUsGameState* GS = GetGameState<AAmongUsGameState>())
        {
            PlayersToSend = GS->PlayerArray.Num();
        }
        FString MapPath = FString::Printf(TEXT("/Game/Maps/Level?listen?ExpectedPlayers=%d"), PlayersToSend);
        World->ServerTravel(MapPath);
    }
}

void AAmongUsGameMode::AssignRolesOnLevel()
{
    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    if (!GS) return;

    TArray<AAmongUsPlayerState*> Players;
    
    // CORRECTION: Iterate using APlayerState*, then cast
    for (APlayerState* PS : GS->PlayerArray)
    {
        AAmongUsPlayerState* MyPS = Cast<AAmongUsPlayerState>(PS);
        if (MyPS) Players.Add(MyPS);
    }

    if (Players.Num() == 0) return;
    FRandomStream Stream(FPlatformTime::Cycles64());
    
    // Shuffle logic
    for (int32 i = Players.Num() - 1; i > 0; i--)
    {
        int32 SwapIndex = Stream.RandRange(0, i);
        Players.Swap(i, SwapIndex);
    }
    
    // Assign Roles
    Players[0]->SetPlayerRole(EPlayerRole::Mechant);

    for (int32 i = 1; i < Players.Num(); i++)
    {
        Players[i]->SetPlayerRole(EPlayerRole::Gentil);
        // Note: Make sure nbGentil is defined in your .h
    }

    // Log Roles
    for (int32 i = 0; i < Players.Num(); i++)
    {
        const TCHAR* RoleText = (Players[i]->GetPlayerRole() == EPlayerRole::Gentil) ? TEXT("Gentil") :
                                (Players[i]->GetPlayerRole() == EPlayerRole::Mechant) ? TEXT("Méchant") : 
                                TEXT("Mort");
        UE_LOG(LogTemp, Warning, TEXT("Role: %s -> %s"), *Players[i]->GetPlayerName(), RoleText);
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
    World->ServerTravel("/Game/Maps/Lobby?listen");
}

void AAmongUsGameMode::CheckWinCondition()
{
    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    if (!GS || !HasAuthority()) return;

    // 1. Victoire Gentils (Tâches finies)
    if (GS->nbTache <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("VICTOIRE GENTILS ! (Tâches finies)"));
        GS->StopGameCountdownTimer();
        ReturnToLobby();
        return;
    }

    // 2. Compter les vivants
    int32 GentilsVivants = 0;
    int32 MechantsVivants = 0;

    // CORRECTION: Iterate using APlayerState*, then cast
    for (APlayerState* PS : GS->PlayerArray)
    {
        AAmongUsPlayerState* MyPS = Cast<AAmongUsPlayerState>(PS);
        if (MyPS && MyPS->GetPlayerRole() != EPlayerRole::Mort)
        {
            if (MyPS->GetPlayerRole() == EPlayerRole::Gentil)
            {
                GentilsVivants++;
            }
            else if (MyPS->GetPlayerRole() == EPlayerRole::Mechant)
            {
                MechantsVivants++;
            }
        }
    }

    // Victoire Gentils (Si le méchant quitte ou meurt)
    if (MechantsVivants == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("VICTOIRE GENTILS ! (Plus de méchants)"));
        GS->StopGameCountdownTimer();
        ReturnToLobby();
        return;
    }

    // CORRECTION: Victoire Méchants (Domination >=)
    if (MechantsVivants >= GentilsVivants)
    {
        UE_LOG(LogTemp, Warning, TEXT("VICTOIRE MECHANTS ! (Domination)"));
        GS->StopGameCountdownTimer();
        ReturnToLobby();
    }
}