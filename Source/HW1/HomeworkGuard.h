#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "HomeworkGuard.generated.h"

class UBehaviorTree;
class UTextRenderComponent;

enum class EHomeworkGuardState : uint8
{
    Patrol,
    Investigate,
    Chase,
    Flee
};

UCLASS()
class HW1_API AHomeworkGuard : public ACharacter
{
    GENERATED_BODY()

public:
    AHomeworkGuard();

    void UpdateAwarenessDisplay(EHomeworkGuardState State, const FVector& ViewLocation);

    UPROPERTY(EditAnywhere, Category="Guard")
    TObjectPtr<UBehaviorTree> BehaviorTree;

private:
    UPROPERTY(VisibleAnywhere, Category="Guard")
    TObjectPtr<UTextRenderComponent> AwarenessLabel;

    EHomeworkGuardState DisplayedState = EHomeworkGuardState::Patrol;
};
