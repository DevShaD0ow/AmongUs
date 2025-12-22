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
	// --- Constructor & Ticks ---
	ABouton();
	virtual void Tick(float DeltaTime) override;

	// --- Task Configuration ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task Config")
	FName TaskIDRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task Config")
	TSubclassOf<UUserWidget> TaskWidgetToOpen;

	// --- Visuals & Components ---
	UPROPERTY(VisibleAnywhere)
	USphereComponent* SphereComp;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComp;

	void SetHighlight(bool bActive);

	// --- Interaction Logic ---
	UPROPERTY(EditAnywhere)
	float InteractionDistance;

	virtual void Interact(AAmongUsPlayerState* PlayerState);

	UFUNCTION(Server, Reliable)
	void IncrementTaskServerOnly(AAmongUsPlayerState* PlayerState);

protected:
	// --- Lifecycle ---
	virtual void BeginPlay() override;
};