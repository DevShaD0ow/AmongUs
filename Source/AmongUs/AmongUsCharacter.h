// Copyright Epic Games, Inc. All Rights Reserved.

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
class ABouton; // pour interaction avec les boutons

DECLARE_LOG_CATEGORY_EXTERN(LogAmongUsCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AMONGUS_API AAmongUsCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    /** Constructor */
    AAmongUsCharacter();
    void BeginPlay();
    virtual void OnRep_PlayerState() override;

    // Appelée sur le SERVEUR quand le Controller prend possession du Pion
    virtual void PossessedBy(AController* NewController) override;
    UFUNCTION(NetMulticast, Reliable)
    void MulticastTriggerDeath();
    
    UFUNCTION(BlueprintCallable)
    void ApplyColorToSkin(EPlayerColor Color);

    // Helper pour convertir l'enum en vraie couleur RGB
    FLinearColor GetLinearColorFromEnum(EPlayerColor Color);
    
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void OnOpenColorPicker();
    // Fonction de mise à jour visuelle
    void UpdateTaskHighlights();
protected:
    // ==========================
    // Components
    // ==========================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rewind", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<URewindableComponent> RewindCapsule;

    // Fonction appelée lors du "tir"
    void Fire(); 

    // 7. RPC du client vers le serveur
    UFUNCTION(Server, Reliable)
    void ServerConfirmHit(float ClientTimestamp, const FVector& StartLocation, const FRotator& Rotation, APlayerState* TargetPlayerState);
    void ServerConfirmHit_Implementation(float ClientTimestamp, const FVector& StartLocation, const FRotator& Rotation, APlayerState* TargetPlayerState);


    UPROPERTY()
    TArray<class ABouton*> CachedButtons;

    
    
    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    // ==========================
    // Input Actions
    // ==========================
    
    /** Jump Input Action */
    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* JumpAction;

    /** Move Input Action */
    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* MoveAction;

    /** Look Input Action */
    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* LookAction;

    /** Mouse Look Input Action */
    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* MouseLookAction;

    // ==========================
    // Protected Functions
    // ==========================

    /** Initialize input action bindings */
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    /** Called for movement input */
    void Move(const FInputActionValue& Value);

    /** Called for looking input */
    void Look(const FInputActionValue& Value);

public:
    // ==========================
    // Public Functions
    // ==========================
    /** Handles move inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoMove(float Right, float Forward);

    /** Handles look inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoLook(float Yaw, float Pitch);

    /** Handles jump pressed inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoJumpStart();

    /** Handles jump release inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoJumpEnd();

    /** Interaction avec un bouton, RPC côté serveur */
    UFUNCTION(Server, Reliable)
    void ServerInteractWithButton(ABouton* Btn);

    /** Tente d’interagir avec un bouton proche (appelé côté client) */
    void TryInteract();
    void OpenTaskWidget(ABouton* Btn);

    // ==========================
    // Properties
    // ==========================

    UPROPERTY()
    UMaterialInstanceDynamic* DynamicMaterial;

    /** Distance max pour interagir */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    float InteractionDistance = 200.f; 

    // ==========================
    // Inline Functions
    // ==========================

    /** Returns CameraBoom subobject */
    FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
};
