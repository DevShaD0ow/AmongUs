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

void AAmongUsGameMode::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    if (World && World->GetMapName().EndsWith("Level"))
    {
        GetWorldTimerManager().SetTimer(StartCheckTimer, this, &AAmongUsGameMode::CheckLevelStart, 1.0f, true);
        UE_LOG(LogTemp, Warning, TEXT("Level : En attente des joueurs via Seamless Travel..."));
    }
}

void AAmongUsGameMode::CheckLevelStart()
{
    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    if (!GS) return;

    if (GS->bRolesAssigned)
    {
        GetWorldTimerManager().ClearTimer(StartCheckTimer);
        return;
    }

    int32 CurrentPlayerCount = GS->PlayerArray.Num();

    if (CurrentPlayerCount >= ExpectedPlayerCount && ExpectedPlayerCount > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Level : Tous les joueurs (%d/%d) sont arrivés ! Lancement."), CurrentPlayerCount, ExpectedPlayerCount);

        GetWorldTimerManager().ClearTimer(StartCheckTimer);
        GS->bRolesAssigned = true;

        AssignRolesOnLevel();

        GS->GameCountdown = 300; 
        if (!GetWorldTimerManager().IsTimerActive(GS->GameTimerHandle))
        {
            GetWorldTimerManager().SetTimer(GS->GameTimerHandle, GS, &AAmongUsGameState::GameCountdownTick, 1.0f, true);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Level : Chargement des joueurs... (%d/%d)"), CurrentPlayerCount, ExpectedPlayerCount);
    }
}
void AAmongUsGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    ExpectedPlayerCount = UGameplayStatics::GetIntOption(Options, TEXT("ExpectedPlayers"), 0);
}

void AAmongUsGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    AssignAvailableColorToPlayer(NewPlayer);

    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    if (!GS) return;

    UWorld* World = GetWorld();
    if (World && World->GetMapName().EndsWith("Lobby"))
    {
        int32 CurrentPlayerCount = GS->PlayerArray.Num();
        UE_LOG(LogTemp, Warning, TEXT("Lobby : Joueur connecté (%d). Couleur attribuée."), CurrentPlayerCount);
        CheckAllPlayersReady();
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

void AAmongUsGameMode::AssignAvailableColorToPlayer(APlayerController* NewPlayer)
{
    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    AAmongUsPlayerState* NewPS = NewPlayer->GetPlayerState<AAmongUsPlayerState>();
    
    if (!GS || !NewPS) return;

    // On parcourt toutes les couleurs possibles (de Red=1 à Pink=8)
    // On suppose que votre Enum a 9 éléments (0=None, 1..8=Couleurs)
    for (uint8 i = 1; i <= 8; i++) 
    {
        EPlayerColor CandidateColor = (EPlayerColor)i;
        bool bIsTaken = false;

        // On vérifie si un autre joueur a DÉJÀ cette couleur
        for (APlayerState* PS : GS->PlayerArray)
        {
            AAmongUsPlayerState* ExistingPS = Cast<AAmongUsPlayerState>(PS);
            
            // On ignore le joueur qu'on est en train de configurer
            if (ExistingPS && ExistingPS != NewPS)
            {
                if (ExistingPS->PlayerColor == CandidateColor)
                {
                    bIsTaken = true;
                    break; // Couleur prise, on arrête de vérifier celle-ci
                }
            }
        }

        if (!bIsTaken)
        {
            NewPS->PlayerColor = CandidateColor;
            
            NewPS->OnRep_PlayerColor(); 
            
            UE_LOG(LogTemp, Warning, TEXT("Couleur %d attribuée au joueur %s"), i, *NewPlayer->GetName());
            return; 
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Aucune couleur disponible pour %s ! (Lobby plein ?)"), *NewPlayer->GetName());
}

void AAmongUsGameMode::AssignRolesOnLevel()
{
    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    if (!GS) return;

    // 1. Trouver tous les boutons disponibles sur la map pour piocher dedans
    TArray<AActor*> AllButtons;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABouton::StaticClass(), AllButtons);

    if (AllButtons.Num() == 0) 
    {
        UE_LOG(LogTemp, Error, TEXT("ERREUR: Aucun ABouton trouvé dans le niveau !"));
        return;
    }

    // CONFIG : Combien de tâches par joueur ?
    const int32 TasksPerPlayer = 4; 
    
    GS->nbTache = 0; // On remet le compteur global à 0

    // Récupération des joueurs
    TArray<AAmongUsPlayerState*> Players;
    for (APlayerState* PS : GS->PlayerArray) 
    {
        if (AAmongUsPlayerState* MyPS = Cast<AAmongUsPlayerState>(PS)) 
        {
            Players.Add(MyPS);
            // On vide les anciennes tâches pour éviter les doublons au restart
            MyPS->AssignedTasks.Empty(); 
        }
    }
    
    if (Players.Num() == 0) return;
    
    // Initialisation du générateur aléatoire
    FRandomStream Stream(FPlatformTime::Cycles64());
    
    // Mélange des joueurs (Pour que l'imposteur change à chaque partie)
    for (int32 i = Players.Num() - 1; i > 0; i--)
    {
        int32 SwapIndex = Stream.RandRange(0, i);
        Players.Swap(i, SwapIndex);
    }
    
    // Distribution des Rôles et Tâches
    for (int32 i = 0; i < Players.Num(); i++)
    {
        // === MECHANT ===
        if (i == 0) 
        {
            Players[i]->SetPlayerRole(EPlayerRole::Mechant);
            // Le méchant n'a pas de tâches
            UE_LOG(LogTemp, Warning, TEXT("DISTRIBUTION: Joueur %s est l'IMPOSTEUR"), *Players[i]->GetPlayerName());
        }
        // === GENTIL ===
        else 
        {
            Players[i]->SetPlayerRole(EPlayerRole::Gentil);

            // 1. On copie la liste complète des boutons pour ce joueur
            TArray<AActor*> ButtonsForThisPlayer = AllButtons;

            // 2. ON MÉLANGE CETTE LISTE (Spécifique à ce joueur)
            // C'est ce bloc qui assure que chacun a des tâches différentes
            for (int32 k = 0; k < ButtonsForThisPlayer.Num(); k++)
            {
                int32 Index = Stream.RandRange(0, ButtonsForThisPlayer.Num() - 1);
                ButtonsForThisPlayer.Swap(k, Index);
            }

            // 3. On prend les X premiers boutons de sa liste mélangée
            int32 CountToAssign = FMath::Min(TasksPerPlayer, ButtonsForThisPlayer.Num());

            for (int32 k = 0; k < CountToAssign; k++)
            {
                ABouton* Btn = Cast<ABouton>(ButtonsForThisPlayer[k]);
                if (Btn)
                {
                    FAmongUsTask NewTask;
                    NewTask.TaskID = Btn->TaskIDRef;
                    NewTask.TaskWidgetClass = Btn->TaskWidgetToOpen;
                    NewTask.bIsCompleted = false; 

                    Players[i]->AssignedTasks.Add(NewTask);
                }
            }

            // 4. On met à jour le compteur global
            GS->nbTache += Players[i]->AssignedTasks.Num();

            // --- LOGGING ---
            FString TaskListLog = "";
            for (const FAmongUsTask& T : Players[i]->AssignedTasks)
            {
                TaskListLog += T.TaskID.ToString() + ", ";
            }
            if (TaskListLog.Len() > 0) TaskListLog = TaskListLog.Left(TaskListLog.Len() - 2);

            UE_LOG(LogTemp, Warning, TEXT("DISTRIBUTION: Joueur %s (GENTIL) doit faire : [%s]"), 
                *Players[i]->GetPlayerName(), 
                *TaskListLog
            );
        }
    }
}

void AAmongUsGameMode::ReturnToLobby()
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (AAmongUsGameState* GS = GetGameState<AAmongUsGameState>())
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
            if (AAmongUsPlayerState* MyPS = Cast<AAmongUsPlayerState>(PS))MyPS->bIsReady = false;
        }
    }

    // 4. Lancer le voyage
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