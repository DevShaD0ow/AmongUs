#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AmongUsGameMode.generated.h"

UCLASS()
class AMONGUS_API AAmongUsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// --- Constructor & Lifecycle ---
	AAmongUsGameMode();
	virtual void BeginPlay() override;
	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage);

	// --- Game Flow Control ---
	void CheckWinCondition();
	void ChangeMap();
	void ReturnToLobby();
	void CheckAllPlayersReady();

	// --- Debug / Info ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Counter")
	int32 nbGentil;
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Counter")
	int32 nbMechant;

protected:
	// --- Login & Setup ---
	virtual void PostLogin(APlayerController* NewPlayer) override;
	void AssignAvailableColorToPlayer(APlayerController* NewPlayer);

	// --- Level Logic ---
	void AssignRolesOnLevel();
	void CheckLevelStart(); 

	FTimerHandle StartCheckTimer;

private:
	// --- Internal Variables ---
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