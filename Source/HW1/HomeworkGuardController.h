#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "HomeworkGuardController.generated.h"

UCLASS()
class HW1_API AHomeworkGuardController : public AAIController
{
    GENERATED_BODY()

public:
    AHomeworkGuardController();
    void UpdateAwareness();

    static const FName HomeLocationKey;
    static const FName TargetActorKey;
    static const FName NoiseLocationKey;
    static const FName HasNoiseKey;
    static const FName MoveLocationKey;
    static const FName SprintNoiseTag;
    static constexpr float SprintHearingRange = 1900.0f;

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

private:
    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    double LastSightTime = 0.0;
};
