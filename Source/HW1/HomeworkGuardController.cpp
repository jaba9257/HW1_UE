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
const FName AHomeworkGuardController::FleeFromLocationKey(TEXT("FleeFromLocation"));
const FName AHomeworkGuardController::ShouldFleeKey(TEXT("ShouldFlee"));
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
        Board->SetValueAsBool(ShouldFleeKey, false);
        FleeUntilTime = 0.0;
        LastSightTime = 0.0;
        ObservePlayer(Cast<AHomeworkCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)));
        RunBehaviorTree(BehaviorTree);
    }
}

void AHomeworkGuardController::OnUnPossess()
{
    ObservePlayer(nullptr);
    if (UBrainComponent* Brain = GetBrainComponent())
    {
        Brain->StopLogic(TEXT("Guard unpossessed"));
    }
    Super::OnUnPossess();
}

void AHomeworkGuardController::ObservePlayer(AHomeworkCharacter* Player)
{
    if (ObservedPlayer.Get() == Player)
    {
        return;
    }

    if (AHomeworkCharacter* Previous = ObservedPlayer.Get())
    {
        Previous->OnInteraction.RemoveDynamic(this, &AHomeworkGuardController::OnPlayerInteraction);
    }
    ObservedPlayer = Player;
    if (Player)
    {
        Player->OnInteraction.AddUniqueDynamic(this, &AHomeworkGuardController::OnPlayerInteraction);
    }
}

void AHomeworkGuardController::OnPlayerInteraction()
{
    const AHomeworkCharacter* Player = ObservedPlayer.Get();
    UBlackboardComponent* Board = GetBlackboardComponent();
    if (!Player || !Board || !GetPawn())
    {
        return;
    }

    constexpr double FleeDuration = 3.5;
    FleeUntilTime = GetWorld()->GetTimeSeconds() + FleeDuration;
    Board->SetValueAsVector(FleeFromLocationKey, Player->GetActorLocation());
    Board->SetValueAsBool(ShouldFleeKey, true);
    Board->ClearValue(TargetActorKey);
    Board->SetValueAsBool(HasNoiseKey, false);
    UpdateAwareness();
}

void AHomeworkGuardController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    const auto* Player = Cast<AHomeworkCharacter>(Actor);
    UBlackboardComponent* Board = GetBlackboardComponent();
    if (!GetPawn() || !Board || !Player || !Player->IsPlayerControlled()
        || !Stimulus.WasSuccessfullySensed() || Stimulus.Tag != SprintNoiseTag
        || IsValid(Board->GetValueAsObject(TargetActorKey)) || Board->GetValueAsBool(HasNoiseKey)
        || Board->GetValueAsBool(ShouldFleeKey))
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
    ObservePlayer(Player);
    const double Now = GetWorld()->GetTimeSeconds();
    const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (Board->GetValueAsBool(ShouldFleeKey))
    {
        if (Now < FleeUntilTime)
        {
            Guard->GetCharacterMovement()->MaxWalkSpeed = 750.0f;
            if (Camera)
            {
                Guard->UpdateAwarenessDisplay(EHomeworkGuardState::Flee, Camera->GetCameraLocation());
            }
            return;
        }
        Board->SetValueAsBool(ShouldFleeKey, false);
    }

    const bool bTracking = Player && Board->GetValueAsObject(TargetActorKey) == Player;
    const float Range = bTracking ? LoseSightRange : SightRange;
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
    if (Camera)
    {
        Guard->UpdateAwarenessDisplay(State, Camera->GetCameraLocation());
    }
}
