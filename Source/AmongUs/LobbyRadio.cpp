#include "LobbyRadio.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

ALobbyRadio::ALobbyRadio()
{
	PrimaryActorTick.bCanEverTick = false; // Pas besoin de Tick pour une radio

	// Création du composant racine (un cube ou juste un point invisible)
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Création du composant Audio
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(RootComponent);
    
	// Par défaut, on ne joue pas automatiquement au spawn, on le gère dans le BeginPlay pour plus de contrôle
	AudioComponent->bAutoActivate = false; 
}

void ALobbyRadio::BeginPlay()
{
	Super::BeginPlay();

	if (MusicTrack)
	{
		AudioComponent->SetSound(MusicTrack);

		if (!bIsSpatialized)
		{
			// --- MODE 2D (Dans la tête) ---
			// Le son est considéré comme "UI", donc joué en 2D sans atténuation
			AudioComponent->SetUISound(true); 
		}
		else
		{
			// --- MODE 3D (Spatialisé) ---
			AudioComponent->SetUISound(false);

			// On écrase les réglages par défaut pour forcer la spatialisation ici
			AudioComponent->bOverrideAttenuation = true;
			AudioComponent->AttenuationOverrides.bSpatialize = true;
			AudioComponent->AttenuationOverrides.bAttenuate = true;
            
			// Réglage de la distance (le son commence à baisser après 4m et s'arrête à 30m)
			AudioComponent->AttenuationOverrides.FalloffDistance = 3000.0f; 
		}

		AudioComponent->Play();
	}
}