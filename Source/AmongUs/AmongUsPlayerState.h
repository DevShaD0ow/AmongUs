#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "AmongUsPlayerState.generated.h"

// Définition des rôles des joueurs
UENUM(BlueprintType)
enum class EPlayerRole : uint8
{
	Gentil UMETA(DisplayName = "Gentil"),
	Mechant UMETA(DisplayName = "Méchant"),
	Mort UMETA(DisplayName = "Mort")
};

// Classe PlayerState spécifique pour Among Us
UCLASS()
class AMONGUS_API AAmongUsPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AAmongUsPlayerState();
	// Getters et setters pour le rôle
	UFUNCTION(BlueprintCallable, Category = "Role")
	void SetPlayerRole(EPlayerRole NewRole);

	UFUNCTION(BlueprintCallable, Category = "Role")
	EPlayerRole GetPlayerRole() const;

	// Réplication
	UFUNCTION()
	void OnRep_PlayerRole();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(Replicated,BlueprintReadWrite)int32 ColorID;
	UPROPERTY(Replicated, BlueprintReadWrite)
	bool bIsReady = false;

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerSetReady(bool bReady);
protected:
	// Rôle du joueur
	UPROPERTY(ReplicatedUsing = OnRep_PlayerRole, BlueprintReadOnly, Category = "Role")
	EPlayerRole PlayerRole;
	
};
