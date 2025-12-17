#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ColorCube.generated.h"

UCLASS()
class AMONGUS_API AColorCube : public AActor
{
	GENERATED_BODY()
	
public:	
	AColorCube();

protected:
	// Un mesh pour qu'on puisse le voir et le toucher
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComp;
};