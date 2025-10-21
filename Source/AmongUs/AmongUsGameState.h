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

	/** Nombre de tâches restantes */
	UPROPERTY(Replicated, BlueprintReadWrite)
	int32 nbTache;

	/** Rôles déjà assignés */
	UPROPERTY()
	bool bRolesAssigned;

	/** Compteurs répliqués */
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 LobbyCountdown;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 GameCountdown;

	/** Handles internes de timers */
	FTimerHandle LobbyCountdownTimer;
	FTimerHandle GameCountdownTimer;

	/** Mise à jour côté serveur */
	void UpdateLobbyCountdown();
	void UpdateGameCountdown();
	
	/** Interaction joueur */
	UFUNCTION(Server, Reliable)
	void ServerModifyNbtache(AAmongUsPlayerState* PlayerState);

	/** Réplication */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
