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

        if (Prev == -1 && Next == -1)continue;
        

        if (Next == -1)
        {
            Out.Add(Comp, History.History[Prev].Transform);
            continue;
        }
        
        if (Prev == -1)
        {
            Out.Add(Comp, History.History[Next].Transform);
            continue;
        }

        const FRewindState& A = History.History[Prev];
        const FRewindState& B = History.History[Next];

        if (FMath::IsNearlyEqual(A.ServerTime, B.ServerTime, 0.001f))
        {
            Out.Add(Comp, A.Transform);
            continue;
        }
        
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
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)return false;
    

    UE_LOG(LogTemp, Warning, TEXT("VerifyHit: %s tire sur %s à t=%.3f"), 
        *Shooter->GetPlayerName(), *Target->GetPlayerName(), ClientTimestamp);

    TMap<URewindableComponent*, FTransform> Rewinded;
    if (!GetRewindStatesForTime(ClientTimestamp, Rewinded))return false;
    

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
    
    if (APawn* ShooterPawn = Shooter->GetPawn())Params.AddIgnoredActor(ShooterPawn);
    

    DrawDebugLine(World, Start, End, FColor::Red, false, 2.0f, 0, 2.0f);

    bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

    for (auto& Elem : Originals)
    {
        Elem.Key->SetWorldTransform(Elem.Value);
        Elem.Key->UpdateComponentToWorld();
    }

    if (!bHit)return false;
    APawn* HitPawn = Cast<APawn>(Hit.GetActor());
    if (!HitPawn)return false;
    
    bool bIsCorrectTarget = (HitPawn->GetPlayerState() == Target);
    return bIsCorrectTarget;
}