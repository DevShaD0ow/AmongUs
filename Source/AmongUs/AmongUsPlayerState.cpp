#include "AmongUsPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "AmongUs.h"
#include "AmongUsCharacter.h"
#include "AmongUsGameMode.h"
#include "AmongUsPlayerController.h"


AAmongUsPlayerState::AAmongUsPlayerState()
{
    PlayerRole = EPlayerRole::Gentil; // Rôle par défaut
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
    const TCHAR* RoleText = (PlayerRole == EPlayerRole::Gentil) ? TEXT("Gentil") :
                            (PlayerRole == EPlayerRole::Mechant) ? TEXT("Mechant") :
                            TEXT("Mort");
    UE_LOG(LogTemp, Warning, TEXT("Le rôle du joueur a changé : %s"), RoleText);
    if (PlayerRole == EPlayerRole::Mort)
    {

        AAmongUsPlayerController* PC = Cast<AAmongUsPlayerController>(GetPlayerController());
        if (PC && PC->IsLocalController())
        {
            PC->EnterSpectatorMode();
        }
    }
}


void AAmongUsPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAmongUsPlayerState, PlayerRole);
    DOREPLIFETIME(AAmongUsPlayerState, bIsReady); // <--- AJOUT
}

void AAmongUsPlayerState::ServerSetReady_Implementation(bool bReady)
{
    bIsReady = bReady;
    
    // Quand un joueur change son état, on demande au GameMode de vérifier si tout le monde est prêt
    if (AAmongUsGameMode* GM = Cast<AAmongUsGameMode>(GetWorld()->GetAuthGameMode()))
    {
        GM->CheckAllPlayersReady();
    }
}



