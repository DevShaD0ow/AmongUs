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

	// Nombre de tâches restantes
	UPROPERTY(Replicated, BlueprintReadWrite)
	int32 nbTache;

	// Rôles attribués ?
	UPROPERTY(Replicated, BlueprintReadWrite)
	bool bRolesAssigned;

	// Countdown du lobby
	UPROPERTY(Replicated, BlueprintReadWrite)
	int32 LobbyCountdown;

	// Countdown de la partie
	UPROPERTY(Replicated, BlueprintReadWrite)
	int32 GameCountdown;

	FTimerHandle LobbyTimerHandle;
	FTimerHandle GameTimerHandle;

	// Réplication des propriétés
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Fonctions de tick serveur pour countdown
	void LobbyCountdownTick();
	void GameCountdownTick();
	void StopGameCountdownTimer();

	// Appelé quand une tâche est complétée
	UFUNCTION(Server, Reliable)
	void ServerModifyNbtache(AAmongUsPlayerState* PlayerState);

protected:
	virtual void BeginPlay() override;

private:
	
};
	