#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AmongUsGameMode.generated.h"

UCLASS()
class AMONGUS_API AAmongUsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAmongUsGameMode();

	// Vérifie si toutes les tâches sont faites
	void CheckWinCondition();
	void ChangeMap();
	void ReturnToLobby();
protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// Fonctions principales
	void AssignRolesOnLevel();
	void SpawnButtons();

private:
	bool bHasMapChanged;

	UPROPERTY(EditDefaultsOnly, Category = "Game Flow")
	int32 NumPlayersExpected = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Game Flow")
	float GameDuration = 120.0f; // 2 minutes

	UPROPERTY(EditDefaultsOnly, Category = "Game Flow")
	float LobbyCountdownDuration = 30.0f; // 30 secondes

	FTimerHandle LobbyCountdownTickHandle;
	FTimerHandle GameCountdownTickHandle;
};
