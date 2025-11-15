#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RewindSubsystem.generated.h"

class URewindableComponent;
class AAmongUsPlayerState;

USTRUCT()
struct FRewindState
{
	GENERATED_BODY()

	UPROPERTY()
	float ServerTime = 0.f;

	UPROPERTY()
	FTransform Transform;
};

USTRUCT()
struct FComponentHistory
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FRewindState> History;
};

UCLASS()
class AMONGUS_API URewindSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	// UTickableWorldSubsystem overrides
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// Contrôle création uniquement côté serveur
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// API
	void RegisterRewindableComponent(URewindableComponent* Component);
	bool VerifyHit(float ClientTimestamp, const FVector& StartLocation, const FRotator& Rotation, APlayerState* TargetPlayerState);

protected:
	// helpers
	void RecordRewindStates(float ServerTime);
	bool GetRewindStatesForTime(float RewindTime, TMap<URewindableComponent*, FTransform>& OutTransforms);

	// données
	UPROPERTY()
	TArray<TWeakObjectPtr<URewindableComponent>> RegisteredComponents;

	UPROPERTY()
	TMap<AAmongUsPlayerState*, FComponentHistory> ComponentHistories;

	// garde 1 seconde par défaut
	UPROPERTY(EditAnywhere)
	float MaxHistoryTime = 1.0f;
};
