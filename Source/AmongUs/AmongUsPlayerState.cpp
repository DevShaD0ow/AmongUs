#include "AmongUsPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "AmongUs.h"


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
}

void AAmongUsPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAmongUsPlayerState, PlayerRole);
}
