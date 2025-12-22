#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AmongUsPlayerController.generated.h"

// Forward declaration pour éviter d'inclure le header du plugin ici
class UOTSessionMenu; 

UCLASS()
class AMONGUS_API AAmongUsPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAmongUsPlayerController();

	virtual void BeginPlay() override;
	virtual void PostNetInit() override;

	// === NETWORK CLOCK (Pour le Rewind) ===
	UFUNCTION(BlueprintPure, Category="Network Clock")
	float GetServerWorldTimeDelta() const;
	
	UFUNCTION(BlueprintPure, Category="Network Clock")
	float GetServerWorldTime() const;

	// === QUIT MENU (Touche F) ===
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> QuitMenuWidgetClass;

	UPROPERTY()
	class UUserWidget* QuitMenuWidgetInstance;

	UFUNCTION()
	void QuitGameClient();

	void OnInteractPressed();
	void ToggleQuitMenu();

	// === ONLINE TOOLBOX INTEGRATION (Menu Session) ===
	// Glisse ici "WBP_SessionsMenu_BasicSystem" dans le Blueprint
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI | OnlineToolbox")
	TSubclassOf<class UUserWidget> SessionMenuWidgetClass;

	// Fonction pour créer et afficher le menu
	UFUNCTION(BlueprintCallable, Category = "UI | OnlineToolbox")
	void ShowSessionMenu();

protected:
	virtual void SetupInputComponent() override;

	// === Input Mapping ===
	UPROPERTY(EditAnywhere, Category="Input")
	TArray<class UInputMappingContext*> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category="Input")
	TArray<class UInputMappingContext*> MobileExcludedMappingContexts;

	// === Network Synced Clock Settings ===
	UPROPERTY(EditDefaultsOnly, Category="Network Clock")
	float NetworkClockUpdateFrequency = 1.0f; 

	// Callbacks pour la synchro
	void RequestWorldTime_Internal();

	UFUNCTION(Client, Reliable)
	void ClientUpdateWorldTime(float ClientTimestamp, float ServerTimestamp);

	UFUNCTION(Server, Reliable)
	void ServerRequestWorldTime(float ClientTimestamp);

	// Debug
	UFUNCTION(BlueprintCallable)
	void UpdateClientCountdowns();

private:
	float ServerWorldTimeDelta = 0.f;
	TArray<float> RTTCircularBuffer;
};