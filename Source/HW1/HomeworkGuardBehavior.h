#pragma once

#include "BehaviorTree/BTService.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "HomeworkGuardBehavior.generated.h"

UCLASS(DisplayName="Guard Awareness")
class HW1_API UBTService_HomeworkGuardAwareness : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_HomeworkGuardAwareness();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

UCLASS(DisplayName="Choose Reachable Point")
class HW1_API UBTTask_HomeworkGuardChoosePoint : public UBTTask_BlackboardBase
{
    GENERATED_BODY()

public:
    UBTTask_HomeworkGuardChoosePoint();
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual FString GetStaticDescription() const override;

    UPROPERTY(EditAnywhere, Category="Blackboard")
    FBlackboardKeySelector CenterKey;

    UPROPERTY(EditAnywhere, Category="Navigation", meta=(ClampMin="1"))
    float Radius = 1200.0f;
};

UCLASS(DisplayName="Find Flee Point")
class HW1_API UBTTask_HomeworkGuardFindFleePoint : public UBTTask_BlackboardBase
{
    GENERATED_BODY()

public:
    UBTTask_HomeworkGuardFindFleePoint();
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    UPROPERTY(EditAnywhere, Category="Blackboard")
    FBlackboardKeySelector ThreatKey;
};
