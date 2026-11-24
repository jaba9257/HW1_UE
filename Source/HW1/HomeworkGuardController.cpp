#include "HomeworkGuardController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HomeworkCharacter.h"
#include "HomeworkGuard.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"

const FName AHomeworkGuardController::HomeLocationKey(TEXT("HomeLocation"));
const FName AHomeworkGuardController::TargetActorKey(TEXT("TargetActor"));
const FName AHomeworkGuardController::NoiseLocationKey(TEXT("NoiseLocation"));
const FName AHomeworkGuardController::HasNoiseKey(TEXT("HasNoise"));
const FName AHomeworkGuardController::MoveLocationKey(TEXT("MoveLocation"));
const FName AHomeworkGuardController::SprintNoiseTag(TEXT("HomeworkSprint"));

AHomeworkGuardController::AHomeworkGuardController()
{
    auto* Hearing = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("Hearing"));
    Hearing->HearingRange = SprintHearingRange;
    // The tag and player check below identify sprint noise independently of team assignment.
    Hearing->DetectionByAffiliation = FAISenseAffiliationFilter(true, true, true);
    Hearing->SetMaxAge(1.0f);

    auto* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("GuardPerception"));
    Perception->ConfigureSense(*Hearing);
    SetPerceptionComponent(*Perception);
    Perception->OnTargetPerceptionUpdated.AddDynamic(this, &AHomeworkGuardController::OnTargetPerceptionUpdated);
}

void AHomeworkGuardController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    const auto* Guard = Cast<AHomeworkGuard>(InPawn);
    UBehaviorTree* BehaviorTree = Guard ? Guard->BehaviorTree.Get() : nullptr;
    UBlackboardComponent* Board = nullptr;
    if (ensure(BehaviorTree && BehaviorTree->BlackboardAsset)
        && UseBlackboard(BehaviorTree->BlackboardAsset, Board))
    {
        Board->SetValueAsVector(HomeLocationKey, InPawn->GetActorLocation());
        Board->ClearValue(TargetActorKey);
        Board->ClearValue(NoiseLocationKey);
        Board->SetValueAsBool(HasNoiseKey, false);
        LastSightTime = 0.0;
        RunBehaviorTree(BehaviorTree);
    }
}

void AHomeworkGuardController::OnUnPossess()
{
    if (UBrainComponent* Brain = GetBrainComponent())
    {
        Brain->StopLogic(TEXT("Guard unpossessed"));
    }
    Super::OnUnPossess();
}

void AHomeworkGuardController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    const auto* Player = Cast<AHomeworkCharacter>(Actor);
    UBlackboardComponent* Board = GetBlackboardComponent();
    if (!GetPawn() || !Board || !Player || !Player->IsPlayerControlled()
        || !Stimulus.WasSuccessfullySensed() || Stimulus.Tag != SprintNoiseTag
        || IsValid(Board->GetValueAsObject(TargetActorKey)) || Board->GetValueAsBool(HasNoiseKey))
    {
        return;
    }

    Board->SetValueAsVector(NoiseLocationKey, Stimulus.StimulusLocation);
    Board->SetValueAsBool(HasNoiseKey, true);
}

void AHomeworkGuardController::UpdateAwareness()
{
    auto* Guard = Cast<AHomeworkGuard>(GetPawn());
    UBlackboardComponent* Board = GetBlackboardComponent();
    if (!Guard || !Board)
    {
        return;
    }

    constexpr float SightRange = 1350.0f;
    constexpr float LoseSightRange = 1700.0f;
    constexpr double OcclusionGrace = 0.65;
    auto* Player = Cast<AHomeworkCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    const bool bTracking = Player && Board->GetValueAsObject(TargetActorKey) == Player;
    const float Range = bTracking ? LoseSightRange : SightRange;
    const double Now = GetWorld()->GetTimeSeconds();
    const bool bInRange = Player && FVector::DistSquared(Player->GetActorLocation(), Guard->GetActorLocation())
        <= FMath::Square(Range);
    const bool bVisible = bInRange && LineOfSightTo(Player);
    if (bVisible)
    {
        LastSightTime = Now;
    }

    const bool bChasing = bVisible || (bTracking && bInRange && Now - LastSightTime <= OcclusionGrace);
    if (bChasing)
    {
        Board->SetValueAsObject(TargetActorKey, Player);
        Board->SetValueAsBool(HasNoiseKey, false);
    }
    else
    {
        Board->ClearValue(TargetActorKey);
    }

    const bool bInvestigating = Board->GetValueAsBool(HasNoiseKey);
    Guard->GetCharacterMovement()->MaxWalkSpeed = bChasing ? 620.0f : (bInvestigating ? 420.0f : 280.0f);
    const EHomeworkGuardState State = bChasing ? EHomeworkGuardState::Chase
        : (bInvestigating ? EHomeworkGuardState::Investigate : EHomeworkGuardState::Patrol);
    if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        Guard->UpdateAwarenessDisplay(State, Camera->GetCameraLocation());
    }
}
