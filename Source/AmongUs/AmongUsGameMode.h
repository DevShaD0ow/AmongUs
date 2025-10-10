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
	void CheckWinCondition();
protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	bool bHasMapChanged;

	void ChangeMap();
	void AssignRolesOnLevel();
	void SpawnButtons();

	int32 NumPlayersExpected = 4; // Nombre total de joueurs attendu pour démarrer le level
};
