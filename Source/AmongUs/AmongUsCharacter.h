#pragma once

#include "CoreMinimal.h"
#include "RewindableComponent.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AmongUsPlayerState.h"
#include "ColorCube.h"
#include "AmongUsCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class ABouton;

DECLARE_LOG_CATEGORY_EXTERN(LogAmongUsCharacter, Log, All);

UCLASS(abstract)
class AMONGUS_API AAmongUsCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    // --- Constructor & Overrides ---
    AAmongUsCharacter();
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_PlayerState() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void BeginPlay() override; 

    // --- Gameplay Interaction (Tasks & Buttons) ---
    void TryInteract();
    void OpenTaskWidget(ABouton* Btn);
    void UpdateTaskHighlights();
    
    // --- Skin & Colors ---
    UFUNCTION(BlueprintCallable)
    void ApplyColorToSkin(EPlayerColor Color);

    FLinearColor GetLinearColorFromEnum(EPlayerColor Color);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnOpenColorPicker();

    // --- Network / RPCs ---
    UFUNCTION(NetMulticast, Reliable)
    void MulticastTriggerDeath();

    UFUNCTION(Server, Reliable)
    void ServerInteractWithButton(ABouton* Btn);

    UFUNCTION(Server, Reliable)
    void ServerConfirmHit(float ClientTimestamp, const FVector& StartLocation, const FRotator& Rotation, APlayerState* TargetPlayerState);

    // --- Input Handlers (Public for UI access) ---
    UFUNCTION(BlueprintCallable, Category="Input")
    void DoMove(float Right, float Forward);

    UFUNCTION(BlueprintCallable, Category="Input")
    void DoLook(float Yaw, float Pitch);

    UFUNCTION(BlueprintCallable, Category="Input")
    void DoJumpStart();

    UFUNCTION(BlueprintCallable, Category="Input")
    void DoJumpEnd();

    // --- Public Getters ---
    FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

    // --- Public Properties ---
    UPROPERTY()
    UMaterialInstanceDynamic* DynamicMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    float InteractionDistance = 200.f; 

protected:
    // --- Internal Input Callbacks ---
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void Fire();

    // --- Components ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rewind", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<URewindableComponent> RewindCapsule;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    // --- Input Assets (Enhanced Input) ---
    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* MouseLookAction;

    // --- Internal State ---
    UPROPERTY()
    TArray<class ABouton*> CachedButtons;
};