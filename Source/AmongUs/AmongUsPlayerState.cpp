#include "AmongUsPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "AmongUs.h"
#include "AmongUsCharacter.h"


AAmongUsPlayerState::AAmongUsPlayerState()
{
    PlayerRole = EPlayerRole::Gentil; // Rôle par défaut
    PlayerColor = FLinearColor::White; // Couleur par défaut

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

void AAmongUsPlayerState::ServerSetPlayerColor_Implementation(const FLinearColor& NewColor)
{
    PlayerColor = NewColor;
    OnRep_PlayerColor();
}

void AAmongUsPlayerState::OnRep_PlayerColor()
{
    // Appliquer la couleur au Pawn du joueur (si présent)
    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!PC) return;

    AAmongUsCharacter* Character = Cast<AAmongUsCharacter>(PC->GetPawn());
    if (Character)
    {
        if (!Character->DynamicMaterial)
        {
            Character->DynamicMaterial = UMaterialInstanceDynamic::Create(Character->GetMesh()->GetMaterial(0), Character);
            Character->GetMesh()->SetMaterial(0, Character->DynamicMaterial);
        }
        Character->DynamicMaterial->SetVectorParameterValue("BodyColor", PlayerColor);
    }
}

void AAmongUsPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAmongUsPlayerState, PlayerColor);
}



