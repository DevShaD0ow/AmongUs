#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RewindSubsystem.generated.h"

class URewindableComponent;

USTRUCT()
struct FRewindState
{
	GENERATED_BODY()

	UPROPERTY()
	float ServerTime;

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
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	void RegisterRewindableComponent(URewindableComponent* Component);

	bool VerifyHit(float ClientTimestamp, const FVector& Start, const FRotator& Rot, APlayerState* TargetState);

private:
	void RecordRewindStates(float ServerTime);
	bool GetRewindStatesForTime(float Time, TMap<URewindableComponent*, FTransform>& OutTransforms);

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<URewindableComponent>> RegisteredComponents;

	UPROPERTY()
	TMap<APlayerState*, FComponentHistory> ComponentHistories;

	float MaxHistoryTime = 1.0f;
};
