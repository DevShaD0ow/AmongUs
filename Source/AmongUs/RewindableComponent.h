#pragma once

#include "CoreMinimal.h"
#include "Components/CapsuleComponent.h"
#include "RewindableComponent.generated.h"

class URewindSubsystem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class AMONGUS_API URewindableComponent : public UCapsuleComponent
{
	GENERATED_BODY()

public:	
	URewindableComponent();

protected:
	virtual void BeginPlay() override;

private:
	URewindSubsystem* RewindSubsystem;
};