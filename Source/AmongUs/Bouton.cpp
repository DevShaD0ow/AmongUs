#include "Bouton.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AmongUsGameState.h"
#include "Engine/StaticMesh.h"


ABouton::ABouton()
{
    bReplicates = true;       
    bAlwaysRelevant = true;   
    PrimaryActorTick.bCanEverTick = true;

    // Création du Root
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    // Création de la sphère de collision 
    SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
    SphereComp->SetupAttachment(RootComponent);
    SphereComp->SetSphereRadius(200.f);

    // Création du Mesh 
    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(RootComponent);

    InteractionDistance = 200.f;

    // Création du Highlight (Visuel)
    HighlightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HighlightMesh"));
    HighlightMesh->SetupAttachment(RootComponent); 
    HighlightMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HighlightMesh->SetVisibility(false);
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