#pragma once

#include "CoreMinimal.h"
#include "Bouton.h"
#include "ColorsButton.generated.h"

UCLASS()
class AMONGUS_API AColorsButton : public ABouton
{
	GENERATED_BODY()

public:
	AColorsButton();

protected:
	// La couleur que ce bouton appliquera au joueur
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Couleur")
	FLinearColor TargetColor;

public:
	// Interaction côté serveur
	virtual void Interact(AAmongUsPlayerState* PlayerState) override;
};
