#include "Bouton.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AmongUsGameState.h"
#include "Engine/StaticMesh.h"
#include "AmongUs.h"
#include "UObject/ConstructorHelpers.h"

ABouton::ABouton()
{
	bReplicates = true;       
	bAlwaysRelevant = true;   
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetupAttachment(RootComponent);
	SphereComp->SetSphereRadius(200.f);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		MeshComp->SetStaticMesh(CubeMesh.Object);
		MeshComp->SetRelativeScale3D(FVector(0.3f)); 
	}

	InteractionDistance = 200.f;
}

void ABouton::BeginPlay()
{
	Super::BeginPlay();
}

void ABouton::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABouton::IncrementTaskServerOnly_Implementation(AAmongUsPlayerState* PlayerState)
{
	// Appel de la logique serveur
	Interact(PlayerState);
}

void ABouton::Interact(AAmongUsPlayerState* PlayerState)
{
	if (!PlayerState) return;

	AAmongUsGameState* GS = Cast<AAmongUsGameState>(UGameplayStatics::GetGameState(GetWorld()));
	if (!GS) return;

	if (GS->HasAuthority())
	{
		GS->ServerModifyNbtache(PlayerState);
	}
}
