#include "AmongUsPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "AmongUs.h"
#include "AmongUsCharacter.h"
#include "AmongUsGameMode.h"
#include "AmongUsGameState.h"
#include "AmongUsPlayerState.h"
#include "AmongUsPlayerController.h"
#include "GameFramework/GameStateBase.h"

AAmongUsPlayerState::AAmongUsPlayerState()
{
    PlayerRole = EPlayerRole::Gentil; // Rôle par défaut
    PlayerColor = EPlayerColor::None; // Initialisation couleur
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
void AAmongUsPlayerState::CopyProperties(APlayerState* PlayerState)
{
    Super::CopyProperties(PlayerState);

    AAmongUsPlayerState* TargetPS = Cast<AAmongUsPlayerState>(PlayerState);
    if (TargetPS)
    {
        TargetPS->PlayerColor = this->PlayerColor;
        TargetPS->PlayerRole = this->PlayerRole; 
        
        TargetPS->ColorID = this->ColorID;
        TargetPS->bIsReady = this->bIsReady;
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
            // On vérifie si un autre joueur a déjà cette couleur
            if (OtherPS && OtherPS != this && OtherPS->PlayerColor == RequestedColor)
            {
                return;
            }
        }
    }

    // 2. Si libre, on l'attribue
    PlayerColor = RequestedColor;
    
    // 3. On force la mise à jour sur le serveur aussi (pour que le serveur voie sa propre couleur)
    OnRep_PlayerColor();
}

void AAmongUsPlayerState::OnRep_PlayerColor()
{
    // Appliquer la couleur au Pawn possédé par ce PlayerState
    if (APawn* MyPawn = GetPawn())
    {
        if (AAmongUsCharacter* MyChar = Cast<AAmongUsCharacter>(MyPawn))
        {
            // Assure-toi que cette fonction existe bien dans AmongUsCharacter (voir étape précédente)
            MyChar->ApplyColorToSkin(PlayerColor);
        }
    }
}

void AAmongUsPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(AAmongUsPlayerState, PlayerRole);
    DOREPLIFETIME(AAmongUsPlayerState, bIsReady); 
    
    // AJOUT CRITIQUE : Il faut dire à Unreal de répliquer la variable PlayerColor
    // Sinon, les clients ne verront jamais la couleur changer !
    DOREPLIFETIME(AAmongUsPlayerState, PlayerColor); 
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