#include "AmongUsPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "AmongUs.h"
#include "AmongUsCharacter.h"
#include "AmongUsGameMode.h"
#include "AmongUsGameState.h"
#include "AmongUsPlayerController.h"

AAmongUsPlayerState::AAmongUsPlayerState()
{
    PlayerRole = EPlayerRole::Gentil; 
    PlayerColor = EPlayerColor::None;
}

void AAmongUsPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAmongUsPlayerState, PlayerRole);
    DOREPLIFETIME(AAmongUsPlayerState, bIsReady); 
    DOREPLIFETIME(AAmongUsPlayerState, AssignedTasks);
    DOREPLIFETIME(AAmongUsPlayerState, PlayerColor); 
}

void AAmongUsPlayerState::CopyProperties(APlayerState* PlayerState)
{
    Super::CopyProperties(PlayerState);
    if (AAmongUsPlayerState* TargetPS = Cast<AAmongUsPlayerState>(PlayerState))
    {
        TargetPS->PlayerColor = this->PlayerColor;
        TargetPS->PlayerRole = this->PlayerRole; 
        TargetPS->ColorID = this->ColorID;
        TargetPS->bIsReady = this->bIsReady;
    }
}

void AAmongUsPlayerState::SetPlayerRole(EPlayerRole NewRole)
{
    if (HasAuthority())
    {
        PlayerRole = NewRole;
        OnRep_PlayerRole();
    }
}

EPlayerRole AAmongUsPlayerState::GetPlayerRole() const
{
    return PlayerRole;
}

void AAmongUsPlayerState::OnRep_PlayerRole()
{
    if (PlayerRole == EPlayerRole::Mort)
    {
        if (AAmongUsPlayerController* PC = Cast<AAmongUsPlayerController>(GetPlayerController()))
        {
            if (PC->IsLocalController()) PC->EnterSpectatorMode();
        }
    }
}

void AAmongUsPlayerState::ServerRequestColor_Implementation(EPlayerColor RequestedColor)
{
    AAmongUsGameState* GameState = Cast<AAmongUsGameState>(GetWorld()->GetGameState());
    if (GameState)
    {
        for (APlayerState* PS : GameState->PlayerArray)
        {
            AAmongUsPlayerState* OtherPS = Cast<AAmongUsPlayerState>(PS);
            if (OtherPS && OtherPS != this && OtherPS->PlayerColor == RequestedColor) return;
        }
    }
    PlayerColor = RequestedColor;
    OnRep_PlayerColor();
}

void AAmongUsPlayerState::OnRep_PlayerColor()
{
    if (APawn* MyPawn = GetPawn())
    {
        if (AAmongUsCharacter* MyChar = Cast<AAmongUsCharacter>(MyPawn))
        {
            MyChar->ApplyColorToSkin(PlayerColor);
        }
    }
}

void AAmongUsPlayerState::ServerMarkTaskFinished_Implementation(FName TaskID)
{
    for (FAmongUsTask& Task : AssignedTasks)
    {
        if (Task.TaskID == TaskID)
        {
            if (!Task.bIsCompleted)
            {
                Task.bIsCompleted = true; 
                if (AAmongUsGameState* GS = GetWorld()->GetGameState<AAmongUsGameState>())
                {
                    GS->ServerModifyNbtache(this); 
                }
                OnRep_AssignedTasks();
            }
            break; 
        }
    }
}

void AAmongUsPlayerState::OnRep_AssignedTasks()
{
    if (APawn* MyPawn = GetPawn())
    {
        if (AAmongUsCharacter* MyChar = Cast<AAmongUsCharacter>(MyPawn))
        {
            MyChar->UpdateTaskHighlights();
        }
    }
}

void AAmongUsPlayerState::ServerSetReady_Implementation(bool bReady)
{
    bIsReady = bReady;
    if (AAmongUsGameMode* GM = Cast<AAmongUsGameMode>(GetWorld()->GetAuthGameMode()))
    {
        GM->CheckAllPlayersReady();
    }
}