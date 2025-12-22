#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "AmongUsPlayerState.generated.h"

// --- Enums & Structs ---
UENUM(BlueprintType)
enum class EPlayerRole : uint8
{
    Gentil UMETA(DisplayName = "Gentil"),
    Mechant UMETA(DisplayName = "Méchant"),
    Mort UMETA(DisplayName = "Mort")
};

UENUM(BlueprintType)
enum class EPlayerColor : uint8
{
    None UMETA(DisplayName = "None"),
    Red UMETA(DisplayName = "Red"),
    Blue UMETA(DisplayName = "Blue"),
    Green UMETA(DisplayName = "Green"),
    Yellow UMETA(DisplayName = "Yellow"),
    Purple UMETA(DisplayName = "Purple"),
    Cyan UMETA(DisplayName = "Cyan"),
    Orange UMETA(DisplayName = "Orange"),
    Pink UMETA(DisplayName = "Pink")
};

USTRUCT(BlueprintType)
struct FAmongUsTask
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName TaskID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UUserWidget> TaskWidgetClass;

    UPROPERTY(BlueprintReadOnly)
    bool bIsCompleted = false;

    bool operator==(const FAmongUsTask& Other) const
    {
        return TaskID == Other.TaskID;
    }
};

UCLASS()
class AMONGUS_API AAmongUsPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    // --- Constructor & Lifecycle ---
    AAmongUsPlayerState();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // --- Roles Management ---
    UFUNCTION(BlueprintCallable, Category = "Role")
    void SetPlayerRole(EPlayerRole NewRole);

    UFUNCTION(BlueprintCallable, Category = "Role")
    EPlayerRole GetPlayerRole() const;

    // --- Tasks System ---
    UFUNCTION(BlueprintCallable, Server, Reliable)
    void ServerMarkTaskFinished(FName TaskID);

    UPROPERTY(ReplicatedUsing = OnRep_AssignedTasks, BlueprintReadOnly, Category = "Tasks")
    TArray<FAmongUsTask> AssignedTasks;

    UFUNCTION()
    void OnRep_AssignedTasks();

    // --- Lobby State & Colors ---
    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerSetReady(bool bReady);

    UFUNCTION(Server, Reliable, BlueprintCallable)
    void ServerRequestColor(EPlayerColor RequestedColor);
    
    UPROPERTY(Replicated,BlueprintReadWrite)
    int32 ColorID;

    UPROPERTY(Replicated, BlueprintReadWrite)
    bool bIsReady = false;

    UPROPERTY(ReplicatedUsing = OnRep_PlayerColor, BlueprintReadOnly, Category = "Color")
    EPlayerColor PlayerColor;

    // --- RepNotifies ---
    UFUNCTION()
    void OnRep_PlayerColor();

    UFUNCTION()
    void OnRep_PlayerRole();

protected:
    // --- Internal Properties ---
    virtual void CopyProperties(APlayerState* PlayerState) override;

    UPROPERTY(ReplicatedUsing = OnRep_PlayerRole, BlueprintReadOnly, Category = "Role")
    EPlayerRole PlayerRole;
};