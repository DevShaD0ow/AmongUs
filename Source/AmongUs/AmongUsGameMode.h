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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Counter")int32 nbGentil;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Counter")int32 nbMechant;
	
	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage);

	void CheckWinCondition();
	void ChangeMap();
	void ReturnToLobby();
	void CheckAllPlayersReady();
protected:
	virtual void PostLogin(APlayerController* NewPlayer) override;

	void AssignRolesOnLevel();
	void SpawnButtons();

private:
	
	bool bHasMapChanged;

	UPROPERTY(EditDefaultsOnly, Category = "Game Flow")
	int32 NumPlayersExpected = 2;
	int32 ExpectedPlayerCount = 0;
	
	UPROPERTY(EditDefaultsOnly, Category = "Game Flow")
	float GameDuration = 120.0f; 

	UPROPERTY(EditDefaultsOnly, Category = "Game Flow")
	float LobbyCountdownDuration = 30.0f;

	FTimerHandle LobbyCountdownTickHandle;
	FTimerHandle GameCountdownTickHandle;
};
