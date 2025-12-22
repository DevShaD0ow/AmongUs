#include "Bouton.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AmongUsGameState.h"
#include "Engine/StaticMesh.h"
#include "AmongUs.h"
#include "Components/PointLightComponent.h"
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

	// CRÉATION DE LA LUMIÈRE
	LightComp = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightComp"));
	LightComp->SetupAttachment(RootComponent);
    
	LightComp->SetLightColor(FLinearColor(1.0f, 0.8f, 0.2f)); 
	LightComp->SetIntensity(5000.0f); 
	LightComp->SetAttenuationRadius(300.0f);
	LightComp->SetVisibility(false);
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
	Interact(PlayerState);
}


void ABouton::SetHighlight(bool bActive)
{
	// Méthode 1 : La lumière (Facile et joli)
	if (LightComp)
	{
		LightComp->SetVisibility(bActive);
	}
	if (MeshComp)
	{
		UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(MeshComp->GetMaterial(0));
		if (!DynMat)
		{
			DynMat = MeshComp->CreateDynamicMaterialInstance(0);
		}

		if (DynMat)
		{
			FLinearColor TargetColor = bActive ? FLinearColor::Green : FLinearColor::White;
			DynMat->SetVectorParameterValue(FName("Color"), TargetColor);
		}
	}
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
