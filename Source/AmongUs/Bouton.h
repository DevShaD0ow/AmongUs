#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AmongUsPlayerState.h"
#include "Bouton.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class AMONGUS_API ABouton : public AActor
{
	GENERATED_BODY()

public:
	ABouton();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// Zone de collision pour détecter les joueurs proches
	UPROPERTY(VisibleAnywhere)
	USphereComponent* SphereComp;

	// Mesh du bouton
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComp;

	// Distance maximale pour interaction
	UPROPERTY(EditAnywhere)
	float InteractionDistance;

	// Interaction côté serveur
	UFUNCTION(Server, Reliable)
	void IncrementTaskServerOnly(AAmongUsPlayerState* PlayerState);

	// Fonction réelle d’interaction
	void Interact(AAmongUsPlayerState* PlayerState);
};
