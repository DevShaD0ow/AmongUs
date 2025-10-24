#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AmongUsPlayerController.generated.h"

UCLASS()
class AMONGUS_API AAmongUsPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAmongUsPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PostNetInit() override;
	UFUNCTION(BlueprintPure, Category="Network Clock")
	float GetServerWorldTimeDelta() const;

	UFUNCTION(BlueprintPure, Category="Network Clock")
	float GetServerWorldTime() const;

	// === Quit Menu ===
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> QuitMenuWidgetClass;

	UPROPERTY()
	class UUserWidget* QuitMenuWidgetInstance;

	UPROPERTY()
	TObjectPtr<UUserWidget> QuitMenuWidget;

	UFUNCTION()
	void QuitGameClient();

	void OnInteractPressed();
	void ToggleQuitMenu();

protected:
	// === Input Mapping ===
	UPROPERTY(EditAnywhere, Category="Input")
	TArray<class UInputMappingContext*> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category="Input")
	TArray<class UInputMappingContext*> MobileExcludedMappingContexts;

	// === Network Synced Clock ===
	UPROPERTY(EditDefaultsOnly, Category="Network Clock")
	float NetworkClockUpdateFrequency = 1.0f; // toutes les secondes

private:
	float ServerWorldTimeDelta = 0.f;
	TArray<float> RTTCircularBuffer;

	FTimerHandle ClientTimerHandle;

	void RequestWorldTime_Internal();
	void UpdateClientCountdowns();

	UFUNCTION(Server, Unreliable)
	void ServerRequestWorldTime(float ClientTimestamp);

	UFUNCTION(Client, Unreliable)
	void ClientUpdateWorldTime(float ClientTimestamp, float ServerTimestamp);
	
};
