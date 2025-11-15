#include "RewindSubsystem.h"
#include "Subsystems/WorldSubsystem.h" 
#include "Stats/Stats.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "AmongUsPlayerState.h"
#include "RewindableComponent.h"
#include "DrawDebugHelpers.h"
#include "AmongUsCharacter.h"

// declare stat
DECLARE_CYCLE_STAT(TEXT("RewindSubsystemTick"), STAT_RewindSubsystemTick, STATGROUP_Tickables);

TStatId URewindSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URewindSubsystem, STATGROUP_Tickables);
}


// 3a. N'existe que sur le serveur.
bool URewindSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    if (!Super::ShouldCreateSubsystem(Outer)) return false;

    if (const UWorld* World = Cast<UWorld>(Outer))
    {
       // Le subsystème n'existe que sur les serveurs (Listen Server ou Dedicated Server)
       return World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer; //
    }
    return false;
}

// 4. Enregistrement
void URewindSubsystem::RegisterRewindableComponent(URewindableComponent* Component)
{
    if (Component)
    {
       RegisteredComponents.AddUnique(Component);
       
       // Récupération sécurisée du PlayerState
       AAmongUsPlayerState* PS = nullptr;
       if (APawn* OwnerPawn = Cast<APawn>(Component->GetOwner()))
       {
           if (AController* Controller = OwnerPawn->GetController())
           {
               PS = Cast<AAmongUsPlayerState>(Controller->GetPlayerState<APlayerState>());
           }
       }
       
       if (PS)
       {
          ComponentHistories.FindOrAdd(PS); //
       }
    }
}

// 5. Enregistre l'état à chaque tick
void URewindSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World) return;

    float ServerTime = World->GetTimeSeconds(); //
    
    RecordRewindStates(ServerTime);
}

void URewindSubsystem::RecordRewindStates(float ServerTime)
{
    // 5. Enregistrer la position de tous les URewindableComponent
    for (const TWeakObjectPtr<URewindableComponent>& CompPtr : RegisteredComponents)
    {
       if (URewindableComponent* Comp = CompPtr.Get())
       {
          // Récupération sécurisée du PlayerState
          AAmongUsPlayerState* PS = nullptr;
          if (APawn* OwnerPawn = Cast<APawn>(Comp->GetOwner()))
          {
              if (AController* Controller = OwnerPawn->GetController())
              {
                  PS = Cast<AAmongUsPlayerState>(Controller->GetPlayerState<APlayerState>());
              }
          }

          if (!PS) continue; //

          FRewindState NewState;
          NewState.ServerTime = ServerTime; //
          NewState.Transform = Comp->GetComponentToWorld(); //

          FComponentHistory& History = ComponentHistories.FindOrAdd(PS);
          History.History.Add(NewState);

          // 5d. Ne garder que 1 seconde d’historique
          while (History.History.Num() > 0 && (ServerTime - History.History[0].ServerTime) > MaxHistoryTime)
          {
             History.History.RemoveAt(0); //
          }

          // 6. Visualisation de debug
          // DrawDebugCapsule(GetWorld(), NewState.Transform.GetLocation(), Comp->GetScaledCapsuleHalfHeight(), Comp->GetScaledCapsuleRadius(), NewState.Transform.GetRotation().Rotator(), FColor::Cyan, false, 0.5f, 0, 5.f); //
       }
    }
}

// 8. Vérification du tir côté serveur
bool URewindSubsystem::VerifyHit(float ClientTimestamp, const FVector& StartLocation, const FRotator& Rotation, APlayerState* TargetPlayerState)
{
    UWorld* World = GetWorld();
    if (!World || !TargetPlayerState) return false;

    // 8a. Récupérer l'état de tous les composants au moment du tir
    TMap<URewindableComponent*, FTransform> RewindTransforms;
    if (!GetRewindStatesForTime(ClientTimestamp, RewindTransforms))
    {
       UE_LOG(LogTemp, Warning, TEXT("VerifyHit: Impossible de trouver/interpoler l'état pour le temps %.3f. Compensation impossible."), ClientTimestamp);
       return false;
    }

    // Stocker les transformations actuelles
    TMap<URewindableComponent*, FTransform> OriginalTransforms;
    for (auto& Elem : RewindTransforms)
    {
       URewindableComponent* Comp = Elem.Key;
       OriginalTransforms.Add(Comp, Comp->GetComponentToWorld());

       // 8b. Déplacer la capsule à la position rewound
       Comp->SetWorldTransform(Elem.Value); //
    }

    // 8c. Refaire le line trace pour confirmer le tir
    FVector EndLocation = StartLocation + (Rotation.Vector() * 100000.f); 
    FHitResult HitResult;
    FCollisionQueryParams Params;
    
    Params.AddIgnoredActor(TargetPlayerState->GetPawn()); 

    bool bHit = World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Pawn, Params); //
    
    // 8b.ii. Remettre les composants à leur place après
    for (auto& Elem : OriginalTransforms)
    {
       Elem.Key->SetWorldTransform(Elem.Value); //
    }

    // Confirmer le tir sur la cible
    if (bHit)
    {
       AAmongUsCharacter* HitCharacter = Cast<AAmongUsCharacter>(HitResult.GetActor());
       if (HitCharacter && HitCharacter->GetPlayerState() == TargetPlayerState)
       {
          UE_LOG(LogTemp, Warning, TEXT("Rewind Hit Confirmed: Player %s hit!"), *TargetPlayerState->GetPlayerName());
          return true;
       }
    }

    UE_LOG(LogTemp, Warning, TEXT("Rewind Hit Denied: Trace missed or hit wrong player."));
    return false;
}


// 8a. Logique de sélection/interpolation de l'historique
bool URewindSubsystem::GetRewindStatesForTime(float RewindTime, TMap<URewindableComponent*, FTransform>& OutTransforms)
{
    for (auto& Elem : ComponentHistories)
    {
       AAmongUsPlayerState* PS = Elem.Key;
       FComponentHistory& History = Elem.Value;

       if (!PS || !PS->GetPawn()) continue;
       
       URewindableComponent* Comp = PS->GetPawn()->FindComponentByClass<URewindableComponent>();
       if (!Comp) continue;

       // 8a.i Trouver les frames (Previous/Next)
       int32 PrevIndex = -1;
       int32 NextIndex = -1;

       for (int32 i = 0; i < History.History.Num(); ++i)
       {
          if (History.History[i].ServerTime < RewindTime)
          {
             PrevIndex = i;
          }
          else if (History.History[i].ServerTime >= RewindTime)
          {
             NextIndex = i;
             break;
          }
       }

        // Gestion des cas limites (extrapolation: utiliser la frame la plus proche)
        if (NextIndex == -1 && PrevIndex == -1)
            continue; 
        if (NextIndex == -1 && PrevIndex != -1)
        {
            OutTransforms.Add(Comp, History.History[PrevIndex].Transform);
            continue;
        }
        if (PrevIndex == -1 && NextIndex != -1)
        {
            OutTransforms.Add(Comp, History.History[NextIndex].Transform);
            continue;
        }


       const FRewindState& PrevState = History.History[PrevIndex];
       const FRewindState& NextState = History.History[NextIndex];

       FTransform RewindTransform;

       if (PrevIndex == NextIndex || NextState.ServerTime == PrevState.ServerTime)
       {
          RewindTransform = PrevState.Transform;
       }
       else
       {
          // 8a.ii. Interpolation Linéaire
          float TotalTimeDelta = NextState.ServerTime - PrevState.ServerTime;
          float TimeRatio = (RewindTime - PrevState.ServerTime) / TotalTimeDelta;
          
          // L'alpha dépend du temps de la frame précédente, du temps de la suivante et du moment du tir
          float Alpha = FMath::Clamp(TimeRatio, 0.f, 1.f); 

          // Interpolation entre les deux frames
          RewindTransform.SetLocation(FMath::Lerp(PrevState.Transform.GetLocation(), NextState.Transform.GetLocation(), Alpha));
          RewindTransform.SetRotation(FQuat::Slerp(PrevState.Transform.GetRotation(), NextState.Transform.GetRotation(), Alpha));
          RewindTransform.SetScale3D(PrevState.Transform.GetScale3D()); 
       }

       OutTransforms.Add(Comp, RewindTransform);
    }

    return OutTransforms.Num() > 0;
}