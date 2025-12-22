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

    // --- CONFIGURATION DU CONTOUR (SPHERE X-RAY) ---
    HighlightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HighlightMesh"));
    HighlightMesh->SetupAttachment(RootComponent); 
    
    // On utilise une sphère standard
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereAsset.Succeeded())
    {
        HighlightMesh->SetStaticMesh(SphereAsset.Object);
        // On la grossit pour qu'elle englobe le bouton (Scale 0.5 car la sphère de base est grosse)
        HighlightMesh->SetRelativeScale3D(FVector(0.5f)); 
    }

    // Pas de collision physique, c'est juste visuel
    HighlightMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    // Caché par défaut
    HighlightMesh->SetVisibility(false);
}

void ABouton::BeginPlay()
{
    Super::BeginPlay();
    // IMPORTANT : N'oubliez pas d'assigner le Material "M_XRay" (Translucent, Disable Depth Test)
    // sur le composant HighlightMesh dans l'éditeur Unreal (Blueprint BP_Bouton).
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
    // Méthode Simple : On affiche ou cache la sphère
    if (HighlightMesh)
    {
        HighlightMesh->SetVisibility(bActive);
    }
}

void ABouton::Interact(AAmongUsPlayerState* PlayerState)
{
    if (!PlayerState) return;

    AAmongUsGameState* GS = Cast<AAmongUsGameState>(UGameplayStatics::GetGameState(GetWorld()));
    if (GS && GS->HasAuthority())
    {
        GS->ServerModifyNbtache(PlayerState);
    }
}