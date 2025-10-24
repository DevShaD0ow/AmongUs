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

	UPROPERTY(Replicated, BlueprintReadWrite)
	int32 nbTache;

	UPROPERTY()
	bool bRolesAssigned;

	// Timers lobby/partie
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 LobbyCountdown;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 GameCountdown;

	// Interaction joueur
	UFUNCTION(Server, Reliable)
	void ServerModifyNbtache(AAmongUsPlayerState* PlayerState);

	// Mise à jour countdown
	void UpdateLobbyCountdown();
	void UpdateGameCountdown();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
