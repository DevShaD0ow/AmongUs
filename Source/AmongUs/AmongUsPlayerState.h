#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
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

protected:
	// Rôle du joueur
	UPROPERTY(ReplicatedUsing = OnRep_PlayerRole, BlueprintReadOnly, Category = "Role")
	EPlayerRole PlayerRole;

	// Couleur du joueur
	UPROPERTY(ReplicatedUsing = OnRep_PlayerColor, BlueprintReadOnly, Category = "Appearance")
	FLinearColor PlayerColor;

public:
	// Getters et setters pour le rôle
	UFUNCTION(BlueprintCallable, Category = "Role")
	void SetPlayerRole(EPlayerRole NewRole);

	UFUNCTION(BlueprintCallable, Category = "Role")
	EPlayerRole GetPlayerRole() const;

	// Réplication
	UFUNCTION()
	void OnRep_PlayerRole();

	UFUNCTION()
	void OnRep_PlayerColor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Fonction serveur pour changer la couleur du joueur
	UFUNCTION(Server, Reliable)
	void ServerSetPlayerColor(const FLinearColor& NewColor);
};
