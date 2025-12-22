#include "AmongUsGameMode.h"
#include "AmongUsCharacter.h"
#include "AmongUsPlayerController.h"
#include "AmongUsGameState.h"
#include "AmongUsPlayerState.h"
#include "Bouton.h"
#include "TimerManager.h"
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
    if (GS && GetWorld()->GetMapName().EndsWith("Lobby"))
    {
        CheckAllPlayersReady();
    }
}

void AAmongUsGameMode::CheckLevelStart()
{
    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    if (!GS || GS->bRolesAssigned) return;

    int32 CurrentPlayerCount = GS->PlayerArray.Num();
    if (CurrentPlayerCount >= ExpectedPlayerCount && ExpectedPlayerCount > 0)
    {
        GetWorldTimerManager().ClearTimer(StartCheckTimer);
        GS->bRolesAssigned = true;

        AssignRolesOnLevel();

        GS->GameCountdown = 300; 
        if (!GetWorldTimerManager().IsTimerActive(GS->GameTimerHandle))
        {
            GetWorldTimerManager().SetTimer(GS->GameTimerHandle, GS, &AAmongUsGameState::GameCountdownTick, 1.0f, true);
        }
    }
}

void AAmongUsGameMode::CheckAllPlayersReady()
{
    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    if (!GS || GS->PlayerArray.Num() < 2) return; 

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

    bool bTimerActive = GetWorldTimerManager().IsTimerActive(GS->LobbyTimerHandle);
    if (bAllReady && !bTimerActive)
    {
        GS->LobbyCountdown = 5; 
        GetWorldTimerManager().SetTimer(GS->LobbyTimerHandle, GS, &AAmongUsGameState::LobbyCountdownTick, 1.0f, true);
    }
    else if (!bAllReady && bTimerActive)
    {
        GetWorldTimerManager().ClearTimer(GS->LobbyTimerHandle);
        GS->LobbyCountdown = 30; 
    }
}

void AAmongUsGameMode::AssignRolesOnLevel()
{
    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    if (!GS) return;

    TArray<AActor*> AllButtons;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABouton::StaticClass(), AllButtons);

    if (AllButtons.Num() == 0) return;

    const int32 TasksPerPlayer = 4; 
    GS->nbTache = 0; 

    TArray<AAmongUsPlayerState*> Players;
    for (APlayerState* PS : GS->PlayerArray) 
    {
        if (AAmongUsPlayerState* MyPS = Cast<AAmongUsPlayerState>(PS)) 
        {
            Players.Add(MyPS);
            MyPS->AssignedTasks.Empty(); 
        }
    }
    
    if (Players.Num() == 0) return;
    
    FRandomStream Stream(FPlatformTime::Cycles64());
    for (int32 i = Players.Num() - 1; i > 0; i--)
    {
        int32 SwapIndex = Stream.RandRange(0, i);
        Players.Swap(i, SwapIndex);
    }
    
    for (int32 i = 0; i < Players.Num(); i++)
    {
        if (i == 0) 
        {
            Players[i]->SetPlayerRole(EPlayerRole::Mechant);
        }
        else 
        {
            Players[i]->SetPlayerRole(EPlayerRole::Gentil);
            TArray<AActor*> ButtonsForThisPlayer = AllButtons;

            for (int32 k = 0; k < ButtonsForThisPlayer.Num(); k++)
            {
                int32 Index = Stream.RandRange(0, ButtonsForThisPlayer.Num() - 1);
                ButtonsForThisPlayer.Swap(k, Index);
            }

            int32 CountToAssign = FMath::Min(TasksPerPlayer, ButtonsForThisPlayer.Num());
            for (int32 k = 0; k < CountToAssign; k++)
            {
                if (ABouton* Btn = Cast<ABouton>(ButtonsForThisPlayer[k]))
                {
                    FAmongUsTask NewTask;
                    NewTask.TaskID = Btn->TaskIDRef;
                    NewTask.TaskWidgetClass = Btn->TaskWidgetToOpen;
                    NewTask.bIsCompleted = false; 
                    Players[i]->AssignedTasks.Add(NewTask);
                }
            }
            GS->nbTache += Players[i]->AssignedTasks.Num();
        }
    }
}

void AAmongUsGameMode::AssignAvailableColorToPlayer(APlayerController* NewPlayer)
{
    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    AAmongUsPlayerState* NewPS = NewPlayer->GetPlayerState<AAmongUsPlayerState>();
    if (!GS || !NewPS) return;

    for (uint8 i = 1; i <= 8; i++) 
    {
        EPlayerColor CandidateColor = (EPlayerColor)i;
        bool bIsTaken = false;

        for (APlayerState* PS : GS->PlayerArray)
        {
            AAmongUsPlayerState* ExistingPS = Cast<AAmongUsPlayerState>(PS);
            if (ExistingPS && ExistingPS != NewPS && ExistingPS->PlayerColor == CandidateColor)
            {
                bIsTaken = true;
                break;
            }
        }
        if (!bIsTaken)
        {
            NewPS->PlayerColor = CandidateColor;
            NewPS->OnRep_PlayerColor(); 
            return; 
        }
    }
}

void AAmongUsGameMode::ChangeMap()
{
    if (UWorld* World = GetWorld())
    {
        int32 PlayersToSend = 0;
        if (AAmongUsGameState* GS = GetGameState<AAmongUsGameState>())
        {
            PlayersToSend = GS->PlayerArray.Num();
        }
        World->ServerTravel(FString::Printf(TEXT("/Game/Maps/Level?listen?ExpectedPlayers=%d"), PlayersToSend));
    }
}

void AAmongUsGameMode::ReturnToLobby()
{
    if (AAmongUsGameState* GS = GetGameState<AAmongUsGameState>())
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
            if (AAmongUsPlayerState* MyPS = Cast<AAmongUsPlayerState>(PS)) MyPS->bIsReady = false;
        }
    }
    GetWorld()->ServerTravel("/Game/Maps/Lobby?listen");
}

void AAmongUsGameMode::CheckWinCondition()
{
    AAmongUsGameState* GS = GetGameState<AAmongUsGameState>();
    if (!GS || !HasAuthority()) return;

    if (GS->nbTache <= 0)
    {
        GS->StopGameCountdownTimer();
        ReturnToLobby();
        return;
    }

    int32 GentilsVivants = 0;
    int32 MechantsVivants = 0;

    for (APlayerState* PS : GS->PlayerArray)
    {
        AAmongUsPlayerState* MyPS = Cast<AAmongUsPlayerState>(PS);
        if (MyPS && MyPS->GetPlayerRole() != EPlayerRole::Mort)
        {
            if (MyPS->GetPlayerRole() == EPlayerRole::Gentil) GentilsVivants++;
            else if (MyPS->GetPlayerRole() == EPlayerRole::Mechant) MechantsVivants++;
        }
    }

    if (MechantsVivants == 0 || MechantsVivants >= GentilsVivants)
    {
        GS->StopGameCountdownTimer();
        ReturnToLobby();
    }
}