#include "RewindableComponent.h"
#include "RewindSubsystem.h"
#include "Engine/World.h"

URewindableComponent::URewindableComponent()
{
	// Visible en jeu
	SetHiddenInGame(false);

	// Collision pour être détecté par les traces
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_Pawn);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

void URewindableComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (UWorld* World = GetWorld())
	{
		if (URewindSubsystem* Subsystem = World->GetSubsystem<URewindSubsystem>())
		{
			Subsystem->RegisterRewindableComponent(this);
		}
	}
}
