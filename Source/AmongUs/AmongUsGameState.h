#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AmongUsPlayerState.h"
#include "AmongUsGameState.generated.h"

UCLASS()
class AMONGUS_API AAmongUsGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AAmongUsGameState();

	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing=OnRep_Nbtache, BlueprintReadWrite)
	int32 nbTache;

	UPROPERTY()
	bool bRolesAssigned;

	UFUNCTION(Server, Reliable)
	void ServerModifyNbtache(AAmongUsPlayerState* PlayerState);
	void ServerModifyNbtache_Implementation(AAmongUsPlayerState* PlayerState);

	UFUNCTION()
	void OnRep_Nbtache();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
