#include "HomeworkGuard.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HomeworkGuardController.h"

#define LOCTEXT_NAMESPACE "HomeworkGuard"

AHomeworkGuard::AHomeworkGuard()
{
    AIControllerClass = AHomeworkGuardController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    bUseControllerRotationYaw = false;
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = 280.0f;
    GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -96), FRotator(0, -90, 0));

    AwarenessLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("AwarenessLabel"));
    AwarenessLabel->SetupAttachment(GetCapsuleComponent());
    AwarenessLabel->SetRelativeLocation(FVector(0, 0, 135));
    AwarenessLabel->SetAbsolute(false, true, false);
    AwarenessLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    AwarenessLabel->SetWorldSize(28.0f);
    AwarenessLabel->SetText(LOCTEXT("Patrol", "PATROL"));
    AwarenessLabel->SetTextRenderColor(FColor(98, 226, 212));
}

void AHomeworkGuard::UpdateAwarenessDisplay(EHomeworkGuardState State, const FVector& ViewLocation)
{
    AwarenessLabel->SetWorldRotation((ViewLocation - AwarenessLabel->GetComponentLocation()).Rotation());
    if (DisplayedState == State)
    {
        return;
    }

    DisplayedState = State;
    switch (State)
    {
    case EHomeworkGuardState::Flee:
        AwarenessLabel->SetText(LOCTEXT("Flee", "FLEE"));
        AwarenessLabel->SetTextRenderColor(FColor(193, 144, 255));
        break;
    case EHomeworkGuardState::Chase:
        AwarenessLabel->SetText(LOCTEXT("Chase", "CHASE"));
        AwarenessLabel->SetTextRenderColor(FColor(255, 102, 91));
        break;
    case EHomeworkGuardState::Investigate:
        AwarenessLabel->SetText(LOCTEXT("Investigate", "INVESTIGATE"));
        AwarenessLabel->SetTextRenderColor(FColor(255, 186, 73));
        break;
    case EHomeworkGuardState::Patrol:
        AwarenessLabel->SetText(LOCTEXT("Patrol", "PATROL"));
        AwarenessLabel->SetTextRenderColor(FColor(98, 226, 212));
        break;
    }
}

#undef LOCTEXT_NAMESPACE
