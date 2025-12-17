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
	// Couleur
	// Variable répliquée (Le serveur l'envoie à tout le monde)
	UPROPERTY(ReplicatedUsing = OnRep_PlayerColor, BlueprintReadOnly, Category = "Color")
	EPlayerColor PlayerColor;

	// Fonction appelée quand la couleur change
	UFUNCTION()
	void OnRep_PlayerColor();

	// Fonction pour demander une couleur au serveur
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerRequestColor(EPlayerColor RequestedColor);
	
protected:
	// Rôle du joueur
	UPROPERTY(ReplicatedUsing = OnRep_PlayerRole, BlueprintReadOnly, Category = "Role")
	EPlayerRole PlayerRole;
	
	virtual void CopyProperties(APlayerState* PlayerState) override;
	
};
