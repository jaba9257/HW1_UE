#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "HomeworkGuardController.generated.h"

class AHomeworkCharacter;

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
    static const FName FleeFromLocationKey;
    static const FName ShouldFleeKey;
    static const FName SprintNoiseTag;
    static constexpr float SprintHearingRange = 1900.0f;

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

private:
    void ObservePlayer(AHomeworkCharacter* Player);

    UFUNCTION()
    void OnPlayerInteraction();

    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    TWeakObjectPtr<AHomeworkCharacter> ObservedPlayer;
    double FleeUntilTime = 0.0;
    double LastSightTime = 0.0;
};
