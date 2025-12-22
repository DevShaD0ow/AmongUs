#include "AmongUsCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "AmongUs/RewindSubsystem.h"
#include "InputActionValue.h"
#include "AmongUs.h"
#include "AmongUsGameMode.h"
#include "AmongUsPlayerController.h"
#include "Bouton.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(LogAmongUsCharacter);

AAmongUsCharacter::AAmongUsCharacter()
{
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
    
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;

    RewindCapsule = CreateDefaultSubobject<URewindableComponent>(TEXT("RewindCapsule"));
    RewindCapsule->SetupAttachment(GetCapsuleComponent()); 
    RewindCapsule->SetCapsuleRadius(GetCapsuleComponent()->GetScaledCapsuleRadius());
    RewindCapsule->SetCapsuleHalfHeight(GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

void AAmongUsCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Cache des boutons pour optimiser l'affichage du highlight
    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABouton::StaticClass(), Actors);
    
    for (AActor* Actor : Actors)
    {
        if (ABouton* Btn = Cast<ABouton>(Actor))
        {
            CachedButtons.Add(Btn);
        }
    }
    UpdateTaskHighlights();
}

void AAmongUsCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    if (AAmongUsPlayerState* PS = GetPlayerState<AAmongUsPlayerState>())
    {
        ApplyColorToSkin(PS->PlayerColor);
    }
}

void AAmongUsCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    if (AAmongUsPlayerState* PS = GetPlayerState<AAmongUsPlayerState>())
    {
        ApplyColorToSkin(PS->PlayerColor);
    }
}

void AAmongUsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) 
    {
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAmongUsCharacter::Move);
        EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AAmongUsCharacter::Look);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAmongUsCharacter::Look);
        PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AAmongUsCharacter::Fire);
    }
}

void AAmongUsCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();
    DoMove(MovementVector.X, MovementVector.Y);
}

void AAmongUsCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();
    DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AAmongUsCharacter::DoMove(float Right, float Forward)
{
    if (GetController() != nullptr)
    {
        const FRotator Rotation = GetController()->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Forward);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Right);
    }
}

void AAmongUsCharacter::DoLook(float Yaw, float Pitch)
{
    if (GetController() != nullptr)
    {
        AddControllerYawInput(Yaw);
        AddControllerPitchInput(Pitch);
    }
}

void AAmongUsCharacter::DoJumpStart()
{
    Jump();
}

void AAmongUsCharacter::DoJumpEnd()
{
    StopJumping();
}

void AAmongUsCharacter::TryInteract()
{
    FVector PlayerLocation = GetActorLocation();
    TArray<FHitResult> Hits;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(InteractionDistance);

    bool bHit = GetWorld()->SweepMultiByChannel(Hits, PlayerLocation, PlayerLocation, FQuat::Identity, ECC_Pawn, Sphere);

    if (bHit)
    {
        for (auto& Hit : Hits)
        {
            AActor* HitActor = Hit.GetActor();
            
            if (ABouton* Btn = Cast<ABouton>(HitActor))
            {
                OpenTaskWidget(Btn);
                break; 
            }
            if (Cast<AColorCube>(HitActor))
            {
                OnOpenColorPicker();
                return; 
            }
        }
    }
}

void AAmongUsCharacter::OpenTaskWidget(ABouton* Btn)
{
    if (!Btn || !IsLocallyControlled()) return; 

    AAmongUsPlayerState* PS = GetPlayerState<AAmongUsPlayerState>();
    if (!PS) return;

    for (const FAmongUsTask& Task : PS->AssignedTasks)
    {
        if (!Task.bIsCompleted)
        {
            // Vérification de l'ordre strict
            if (Task.TaskID == Btn->TaskIDRef)
            {
                TSubclassOf<UUserWidget> WidgetClassToSpawn = Task.TaskWidgetClass;
                if (!WidgetClassToSpawn) WidgetClassToSpawn = Btn->TaskWidgetToOpen;

                if (WidgetClassToSpawn)
                {
                    if (APlayerController* PC = Cast<APlayerController>(GetController()))
                    {
                        UUserWidget* TaskWidget = CreateWidget<UUserWidget>(PC, WidgetClassToSpawn);
                        if (TaskWidget)
                        {
                            // if (UAmongUsTaskBase* TaskBase = Cast<UAmongUsTaskBase>(TaskWidget)) TaskBase->CurrentTaskID = Task.TaskID;
                            TaskWidget->AddToViewport();
                            FInputModeUIOnly InputMode;
                            InputMode.SetWidgetToFocus(TaskWidget->TakeWidget());
                            PC->SetInputMode(InputMode);
                            PC->bShowMouseCursor = true;
                        }
                    }
                }
            }
            else
            {
                UE_LOG(LogAmongUsCharacter, Warning, TEXT("Ordre incorrect ! Tâche active : %s"), *Task.TaskID.ToString());
            }
            return; // On sort pour empêcher de faire les tâches suivantes
        }
    }
}

void AAmongUsCharacter::UpdateTaskHighlights()
{
    if (!IsLocallyControlled()) return;

    AAmongUsPlayerState* PS = GetPlayerState<AAmongUsPlayerState>();
    if (!PS) return;

    FName TargetTaskID = NAME_None;
    // Trouver la première tâche non finie
    for (const FAmongUsTask& Task : PS->AssignedTasks)
    {
        if (!Task.bIsCompleted)
        {
            TargetTaskID = Task.TaskID;
            break; 
        }
    }

    // Mettre à jour les boutons
    for (ABouton* Btn : CachedButtons)
    {
        if (!Btn) continue;
        bool bShouldHighlight = (TargetTaskID != NAME_None && Btn->TaskIDRef == TargetTaskID);
        Btn->SetHighlight(bShouldHighlight);
    }
}

void AAmongUsCharacter::ServerInteractWithButton_Implementation(ABouton* Btn)
{
    if (Btn)
    {
        if (AAmongUsPlayerState* MyPS = GetPlayerState<AAmongUsPlayerState>())
        {
            Btn->IncrementTaskServerOnly(MyPS); 
        }
    }
}

void AAmongUsCharacter::ApplyColorToSkin(EPlayerColor Color)
{
    USkeletalMeshComponent* MyMesh = GetMesh();
    if (!MyMesh) return;

    UMaterialInterface* CurrentMat = MyMesh->GetMaterial(0);
    UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(CurrentMat);

    if (!DynMat)
    {
        DynMat = MyMesh->CreateDynamicMaterialInstance(0);
    }

    if (DynMat)
    {
        DynMat->SetVectorParameterValue(FName("PlayerColor"), GetLinearColorFromEnum(Color));
    }
}

FLinearColor AAmongUsCharacter::GetLinearColorFromEnum(EPlayerColor Color)
{
    switch (Color)
    {
    case EPlayerColor::Red: return FLinearColor::Red;
    case EPlayerColor::Blue: return FLinearColor::Blue;
    case EPlayerColor::Green: return FLinearColor::Green;
    case EPlayerColor::Yellow: return FLinearColor::Yellow;
    case EPlayerColor::Purple: return FLinearColor(0.5f, 0.0f, 0.5f);
    case EPlayerColor::Cyan: return FLinearColor(0.0f, 1.0f, 1.0f);
    case EPlayerColor::Orange: return FLinearColor(1.0f, 0.5f, 0.0f);
    case EPlayerColor::Pink: return FLinearColor(1.0f, 0.07f, 0.57f);
    default: return FLinearColor::White;
    }
}

void AAmongUsCharacter::Fire()
{
    UWorld* World = GetWorld();
    if (!World || !IsLocallyControlled()) return;
    if (World->GetMapName().EndsWith("Lobby")) return; 
    
    AAmongUsPlayerState* MyPS = GetPlayerState<AAmongUsPlayerState>();
    if (!MyPS || MyPS->GetPlayerRole() != EPlayerRole::Mechant) return;

    FVector StartLocation = FollowCamera->GetComponentLocation();
    FRotator CameraRotation = FollowCamera->GetComponentRotation();
    FVector EndLocation = StartLocation + (CameraRotation.Vector() * 100000.f);

    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    bool bHit = World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Pawn, Params);

    AAmongUsPlayerState* TargetPS = nullptr;
    if (bHit)
    {
        if (AAmongUsCharacter* HitCharacter = Cast<AAmongUsCharacter>(HitResult.GetActor()))
            TargetPS = HitCharacter->GetPlayerState<AAmongUsPlayerState>();
    }

    if (TargetPS)
    {
        AAmongUsPlayerController* PC = Cast<AAmongUsPlayerController>(GetController());
        float EstimatedServerTime = PC ? PC->GetServerWorldTime() : World->GetTimeSeconds();
        ServerConfirmHit(EstimatedServerTime, StartLocation, CameraRotation, TargetPS);
    }
}

void AAmongUsCharacter::ServerConfirmHit_Implementation(float ClientTimestamp, const FVector& StartLocation, const FRotator& Rotation, APlayerState* TargetPlayerState)
{
    UWorld* World = GetWorld();
    if (!World || !TargetPlayerState) return;
    
    float ServerTime = World->GetTimeSeconds();
    if (FMath::Abs(ServerTime - ClientTimestamp) > 0.5f) return;

    AAmongUsPlayerState* TargetAmongUsPS = Cast<AAmongUsPlayerState>(TargetPlayerState);
    AAmongUsCharacter* TargetCharacter = TargetAmongUsPS ? Cast<AAmongUsCharacter>(TargetAmongUsPS->GetPawn()) : nullptr;
    AAmongUsPlayerState* ShooterPS = GetPlayerState<AAmongUsPlayerState>();

    if (!TargetCharacter || !TargetCharacter->RewindCapsule || !ShooterPS) return;
    
    if (URewindSubsystem* RewindSubsystem = World->GetSubsystem<URewindSubsystem>())
    {
        if (RewindSubsystem->VerifyHit(ClientTimestamp, StartLocation, Rotation, TargetAmongUsPS, ShooterPS))
        {
            TargetCharacter->MulticastTriggerDeath();
            TargetAmongUsPS->SetPlayerRole(EPlayerRole::Mort);

            if (AAmongUsGameMode* GM = Cast<AAmongUsGameMode>(GetWorld()->GetAuthGameMode()))
            {
                GM->CheckWinCondition();
            }
        }
    }
}

void AAmongUsCharacter::MulticastTriggerDeath_Implementation()
{
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->SetComponentTickEnabled(false);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (GetMesh())
    {
        GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
        GetMesh()->SetSimulatePhysics(true);
        GetMesh()->AddImpulse(FVector(0, 0, -100.f), NAME_None, true); 
    }
}