#include "HomeworkGuardBehavior.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
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
