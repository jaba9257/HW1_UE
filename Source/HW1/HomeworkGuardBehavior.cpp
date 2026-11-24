#include "HomeworkGuardBehavior.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Pawn.h"
#include "HomeworkGuardController.h"
#include "NavigationSystem.h"

UBTService_HomeworkGuardAwareness::UBTService_HomeworkGuardAwareness()
{
    NodeName = TEXT("Guard Awareness");
    Interval = 0.15f;
    RandomDeviation = 0.0f;
    INIT_SERVICE_NODE_NOTIFY_FLAGS();
}

void UBTService_HomeworkGuardAwareness::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
    float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
    if (auto* Controller = Cast<AHomeworkGuardController>(OwnerComp.GetAIOwner()))
    {
        Controller->UpdateAwareness();
    }
}

UBTTask_HomeworkGuardChoosePoint::UBTTask_HomeworkGuardChoosePoint()
{
    NodeName = TEXT("Choose Reachable Point");
    BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_HomeworkGuardChoosePoint, BlackboardKey));
    CenterKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_HomeworkGuardChoosePoint, CenterKey));
}

void UBTTask_HomeworkGuardChoosePoint::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);
    if (Asset.BlackboardAsset)
    {
        CenterKey.ResolveSelectedKey(*Asset.BlackboardAsset);
    }
}

EBTNodeResult::Type UBTTask_HomeworkGuardChoosePoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    const auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(OwnerComp.GetWorld());
    if (!Nav || !Blackboard || !CenterKey.IsSet() || !BlackboardKey.IsSet())
    {
        return EBTNodeResult::Failed;
    }

    FNavLocation Center;
    FNavLocation Destination;
    if (!Nav->ProjectPointToNavigation(Blackboard->GetValue<UBlackboardKeyType_Vector>(CenterKey.GetSelectedKeyID()),
            Center, FVector(120, 120, 250))
        || !Nav->GetRandomReachablePointInRadius(Center.Location, Radius, Destination))
    {
        return EBTNodeResult::Failed;
    }

    Blackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), Destination.Location);
    return EBTNodeResult::Succeeded;
}

FString UBTTask_HomeworkGuardChoosePoint::GetStaticDescription() const
{
    return FString::Printf(TEXT("%s\nAround %s within %.0f cm"), *Super::GetStaticDescription(),
        *CenterKey.SelectedKeyName.ToString(), Radius);
}

UBTTask_HomeworkGuardFindFleePoint::UBTTask_HomeworkGuardFindFleePoint()
{
    NodeName = TEXT("Find Flee Point");
    BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_HomeworkGuardFindFleePoint, BlackboardKey));
    ThreatKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_HomeworkGuardFindFleePoint, ThreatKey));
}

void UBTTask_HomeworkGuardFindFleePoint::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);
    if (Asset.BlackboardAsset)
    {
        ThreatKey.ResolveSelectedKey(*Asset.BlackboardAsset);
    }
}

EBTNodeResult::Type UBTTask_HomeworkGuardFindFleePoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* Board = OwnerComp.GetBlackboardComponent();
    const AAIController* Controller = OwnerComp.GetAIOwner();
    const APawn* Guard = Controller ? Controller->GetPawn() : nullptr;
    const auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(OwnerComp.GetWorld());
    if (!Board || !Guard || !Nav || !ThreatKey.IsSet() || !BlackboardKey.IsSet())
    {
        return EBTNodeResult::Failed;
    }

    FNavLocation Origin;
    if (!Nav->ProjectPointToNavigation(Guard->GetActorLocation(), Origin, FVector(120, 120, 250))
        || !Board->IsVectorValueSet(ThreatKey.GetSelectedKeyID()))
    {
        return EBTNodeResult::Failed;
    }

    constexpr int32 CandidateCount = 16;
    constexpr float SearchRadius = 1500.0f;
    constexpr float MinimumDistanceGain = 150.0f;
    const FVector Threat = Board->GetValue<UBlackboardKeyType_Vector>(ThreatKey.GetSelectedKeyID());
    double BestDistanceSquared = FMath::Square(FVector::Dist2D(Origin.Location, Threat) + MinimumDistanceGain);
    TOptional<FVector> BestLocation;
    for (int32 Sample = 0; Sample < CandidateCount; ++Sample)
    {
        FNavLocation Candidate;
        if (Nav->GetRandomReachablePointInRadius(Origin.Location, SearchRadius, Candidate))
        {
            const double DistanceSquared = FVector::DistSquared2D(Candidate.Location, Threat);
            if (DistanceSquared > BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                BestLocation = Candidate.Location;
            }
        }
    }
    if (!BestLocation)
    {
        return EBTNodeResult::Failed;
    }

    Board->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), *BestLocation);
    return EBTNodeResult::Succeeded;
}
