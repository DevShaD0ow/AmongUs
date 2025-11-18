#include "RewindSubsystem.h"
#include "RewindableComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "DrawDebugHelpers.h"
#include "AmongUsPlayerState.h"

bool URewindSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    if (const UWorld* World = Cast<UWorld>(Outer))
    {
        return World->GetNetMode() == NM_ListenServer ||
               World->GetNetMode() == NM_DedicatedServer;
    }
    return false;
}

TStatId URewindSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(URewindSubsystem, STATGROUP_Tickables);
}

void URewindSubsystem::RegisterRewindableComponent(URewindableComponent* Component)
{
    RegisteredComponents.AddUnique(Component);

    if (APawn* Pawn = Cast<APawn>(Component->GetOwner()))
    {
        if (APlayerState* PS = Pawn->GetPlayerState())
        {
            ComponentHistories.FindOrAdd(PS);
        }
    }
}

void URewindSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World) return;

    float Time = World->GetTimeSeconds();
    RecordRewindStates(Time);
}

void URewindSubsystem::RecordRewindStates(float ServerTime)
{
    for (auto& Ptr : RegisteredComponents)
    {
        if (!Ptr.IsValid()) continue;
        URewindableComponent* Comp = Ptr.Get();

        APawn* Pawn = Cast<APawn>(Comp->GetOwner());
        if (!Pawn) continue;

        APlayerState* PS = Pawn->GetPlayerState();
        if (!PS) continue;

        FRewindState State;
        State.ServerTime = ServerTime;
        State.Transform = Comp->GetComponentToWorld();

        FComponentHistory& History = ComponentHistories.FindOrAdd(PS);
        History.History.Add(State);

        while (History.History.Num() > 0 &&
               ServerTime - History.History[0].ServerTime > MaxHistoryTime)
        {
            History.History.RemoveAt(0);
        }

        DrawDebugCapsule(
            GetWorld(),
            State.Transform.GetLocation(),
            Comp->GetScaledCapsuleHalfHeight(),
            Comp->GetScaledCapsuleRadius(),
            State.Transform.GetRotation(),
            FColor::Cyan,
            false, 0.1f
        );
    }
}

bool URewindSubsystem::GetRewindStatesForTime(float Time, TMap<URewindableComponent*, FTransform>& Out)
{
    for (auto& Elem : ComponentHistories)
    {
        APlayerState* PS = Elem.Key;
        FComponentHistory& History = Elem.Value;

        if (!PS || !PS->GetPawn()) continue;

        URewindableComponent* Comp = PS->GetPawn()->FindComponentByClass<URewindableComponent>();
        if (!Comp) continue;

        int32 Prev = -1, Next = -1;

        for (int32 i = 0; i < History.History.Num(); i++)
        {
            if (History.History[i].ServerTime <= Time)
                Prev = i;
            else
            {
                Next = i;
                break;
            }
        }

        if (Prev == -1 && Next == -1) 
        {
            UE_LOG(LogTemp, Warning, TEXT("Pas d'historique pour %s"), *PS->GetPlayerName());
            continue;
        }

        if (Next == -1)
        {
            Out.Add(Comp, History.History[Prev].Transform);
            UE_LOG(LogTemp, Warning, TEXT("Utilisation dernière frame pour %s"), *PS->GetPlayerName());
            continue;
        }
        
        if (Prev == -1)
        {
            Out.Add(Comp, History.History[Next].Transform);
            UE_LOG(LogTemp, Warning, TEXT("Utilisation première frame pour %s"), *PS->GetPlayerName());
            continue;
        }

        const FRewindState& A = History.History[Prev];
        const FRewindState& B = History.History[Next];

        float Alpha = (Time - A.ServerTime) / (B.ServerTime - A.ServerTime);
        Alpha = FMath::Clamp(Alpha, 0.f, 1.f);

        FTransform T;
        T.SetLocation(FMath::Lerp(A.Transform.GetLocation(), B.Transform.GetLocation(), Alpha));
        T.SetRotation(FQuat::Slerp(A.Transform.GetRotation(), B.Transform.GetRotation(), Alpha));
        T.SetScale3D(FVector(1.f));

        Out.Add(Comp, T);
        
        UE_LOG(LogTemp, Warning, TEXT("Interpolation pour %s: Alpha=%.3f, Time=%.3f (A=%.3f, B=%.3f)"), 
            *PS->GetPlayerName(), Alpha, Time, A.ServerTime, B.ServerTime);
    }

    return Out.Num() > 0;
}

bool URewindSubsystem::VerifyHit(float ClientTimestamp, const FVector& Start, const FRotator& Rot, 
                                  APlayerState* Target, APlayerState* Shooter)
{
    if (!Target || !Shooter)
    {
        UE_LOG(LogTemp, Error, TEXT("VerifyHit: Target ou Shooter NULL"));
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("VerifyHit: World NULL"));
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("VerifyHit: %s tire sur %s à t=%.3f"), 
        *Shooter->GetPlayerName(), *Target->GetPlayerName(), ClientTimestamp);

    TMap<URewindableComponent*, FTransform> Rewinded;
    if (!GetRewindStatesForTime(ClientTimestamp, Rewinded))
    {
        UE_LOG(LogTemp, Error, TEXT("VerifyHit: Aucune frame trouvée pour t=%.3f"), ClientTimestamp);
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("VerifyHit: %d composants à rewind"), Rewinded.Num());

    TMap<URewindableComponent*, FTransform> Originals;
    for (auto& Elem : Rewinded)
    {
        Originals.Add(Elem.Key, Elem.Key->GetComponentToWorld());
        Elem.Key->SetWorldTransform(Elem.Value);
        Elem.Key->UpdateComponentToWorld();
    }

    FVector End = Start + Rot.Vector() * 100000.f;
    FHitResult Hit;

    FCollisionQueryParams Params;
    Params.bTraceComplex = false;
    
    if (APawn* ShooterPawn = Shooter->GetPawn())
    {
        Params.AddIgnoredActor(ShooterPawn);
        UE_LOG(LogTemp, Warning, TEXT("VerifyHit: Ignoré tireur %s"), *ShooterPawn->GetName());
    }

    DrawDebugLine(World, Start, End, FColor::Red, false, 2.0f, 0, 2.0f);

    bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

    for (auto& Elem : Originals)
    {
        Elem.Key->SetWorldTransform(Elem.Value);
        Elem.Key->UpdateComponentToWorld();
    }

    if (!bHit)
    {
        UE_LOG(LogTemp, Warning, TEXT("VerifyHit: Aucun hit détecté"));
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("VerifyHit: Hit sur %s à distance %.2f"), 
        *Hit.GetActor()->GetName(), Hit.Distance);

    APawn* HitPawn = Cast<APawn>(Hit.GetActor());
    if (!HitPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("VerifyHit: L'acteur touché n'est pas un Pawn"));
        return false;
    }

    bool bIsCorrectTarget = (HitPawn->GetPlayerState() == Target);
    
    UE_LOG(LogTemp, Warning, TEXT("VerifyHit: Target attendue=%s, Target touchée=%s, Match=%s"), 
        *Target->GetPlayerName(), 
        HitPawn->GetPlayerState() ? *HitPawn->GetPlayerState()->GetPlayerName() : TEXT("NULL"),
        bIsCorrectTarget ? TEXT("OUI") : TEXT("NON"));

    return bIsCorrectTarget;
}