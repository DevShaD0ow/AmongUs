#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AmongUsGameMode.generated.h"

UCLASS()
class AMONGUS_API AAmongUsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// === Constructeur ===
	AAmongUsGameMode();

	// === Accès public ===
	void CheckWinCondition(); // <-- déplacé ici

protected:
	// === Surcharges ===
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// === Fonctions principales ===
	void ChangeMap();
	void AssignRolesOnLevel();
	void SpawnButtons();
	void ReturnToLobby();

private:
	bool bHasMapChanged;

	UPROPERTY(EditDefaultsOnly, Category = "Game Flow")
	int32 NumPlayersExpected = 2;

	UPROPERTY(EditDefaultsOnly, Category = "Game Flow")
	float GameDuration = 120.0f;

	FTimerHandle GameTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Game Flow")
	float LobbyCountdownDuration = 30.0f;

	FTimerHandle LobbyCountdownTickHandle;
	FTimerHandle GameCountdownTickHandle;
};
