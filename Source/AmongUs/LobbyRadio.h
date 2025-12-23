#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h" // Important pour le son
#include "LobbyRadio.generated.h"

UCLASS()
class AMONGUS_API ALobbyRadio : public AActor
{
	GENERATED_BODY()
	
public:	
	ALobbyRadio();

protected:
	virtual void BeginPlay() override;

public:	
	// Composant Audio pour jouer le son
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	UAudioComponent* AudioComponent;

	// Le fichier son à jouer (à assigner dans l'éditeur)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* MusicTrack;

	// Est-ce que le son est en 2D (dans la tête) ou 3D (spatialisé dans la pièce) ?
	UPROPERTY(EditAnywhere, Category = "Audio")
	bool bIsSpatialized = false;
};