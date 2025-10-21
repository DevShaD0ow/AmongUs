#include "ColorsButton.h"
#include "AmongUsPlayerState.h"
#include "AmongUsCharacter.h"

AColorsButton::AColorsButton()
{
	// Couleur par défaut si rien n’est défini dans l’éditeur
	TargetColor = FLinearColor::Red;
}

void AColorsButton::Interact(AAmongUsPlayerState* PlayerState)
{
	if (!PlayerState) return;

	if (HasAuthority())
	{
		// Change la couleur du joueur côté serveur
		PlayerState->ServerSetPlayerColor(TargetColor);
	}
}
