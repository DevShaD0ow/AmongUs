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

    if (World->GetMapName().EndsWith("Lobby"))
    {
        if (CurrentPlayerCount >= 2 && !GetWorldTimerManager().IsTimerActive(GS->LobbyTimerHandle) && HasAuthority())
        {
            GS->LobbyCountdown = static_cast<int32>(LobbyCountdownDuration);
            GetWorldTimerManager().SetTimer(GS->LobbyTimerHandle, GS, &AAmongUsGameState::LobbyCountdownTick, 1.0f, true);
            UE_LOG(LogTemp, Warning, TEXT("LobbyCountdown lancé avec %d joueurs !"), CurrentPlayerCount);
        }
        else if (CurrentPlayerCount < 2)
        {
            GS->LobbyCountdown = 0;
        }
    }
    else if (World->GetMapName().EndsWith("Level"))
    {
        if (!HasAuthority() || GS->bRolesAssigned) return;

        if (CurrentPlayerCount >= ExpectedPlayerCount) 
        {
            GS->nbTache = FMath::RandRange(5, 10);
            GS->bRolesAssigned = true;

            FTimerHandle RoleAssignTimerHandle;
            GetWorldTimerManager().SetTimer(RoleAssignTimerHandle, [this]()
            {
                AssignRolesOnLevel();
                SpawnButtons();
            }, 1.0f, false);

            GS->GameCountdown = static_cast<int32>(GameDuration);
        }
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
    for (APlayerState* PS : GS->PlayerArray)
    {
        AAmongUsPlayerState* MyPS = Cast<AAmongUsPlayerState>(PS);
        if (MyPS) Players.Add(MyPS);
    }

    if (Players.Num() == 0) return;
    FRandomStream Stream(FPlatformTime::Cycles64());
    for (int32 i = Players.Num() - 1; i > 0; i--)
    {
        int32 SwapIndex = Stream.RandRange(0, i);
        Players.Swap(i, SwapIndex);
    }
    Players[0]->SetPlayerRole(EPlayerRole::Mechant);

    for (int32 i = 1; i < Players.Num(); i++)
    {
        Players[i]->SetPlayerRole(EPlayerRole::Gentil);
    }

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

    if (GS->nbTache <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("WIN ! Retour au lobby"));
        GS->StopGameCountdownTimer();
        ReturnToLobby();
    }
}