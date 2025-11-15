#include "RewindableComponent.h"
#include "Engine/World.h"
#include "AmongUs/RewindSubsystem.h" 

URewindableComponent::URewindableComponent()
{
	// 2.a. Taille par défaut pour correspondre au Character (si 42/96)
	SetCapsuleRadius(42.f); 
	SetCapsuleHalfHeight(96.0f);

	// 2.b. Rendre la capsule visible in-game
	SetHiddenInGame(false);
	
	// Utilisation d'un profil de collision adapté
	SetCollisionProfileName(TEXT("Pawn"));
	SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void URewindableComponent::BeginPlay()
{
	Super::BeginPlay();

	// 4. Enregistrement auprès du subsystème (uniquement sur le serveur)
	AActor* Owner = GetOwner();
    
	// Vérifiez si l'Owner existe et si c'est l'autorité sur le réseau
	if (Owner && Owner->HasAuthority())
	{
		RewindSubsystem = GetWorld()->GetSubsystem<URewindSubsystem>();

		if (RewindSubsystem)
		{
			RewindSubsystem->RegisterRewindableComponent(this);
			UE_LOG(LogTemp, Warning, TEXT("URewindableComponent enregistré sur le serveur."));
		}
	}
}