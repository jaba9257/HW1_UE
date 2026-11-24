#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BTNode.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HomeworkCharacter.h"
#include "HomeworkGuard.h"
#include "HomeworkGuardController.h"
#include "InputKeyEventArgs.h"
#include "Misc/AutomationTest.h"
#include "NavigationSystem.h"
#include "PCGVolume.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Tests/AutomationEditorCommon.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
    class FCheckPatrolCourse : public IAutomationLatentCommand
    {
    public:
        explicit FCheckPatrolCourse(FAutomationTestBase& InTest) : Test(InTest) {}

        bool Update() override
        {
            if (FPlatformTime::Seconds() - Started > 45.0)
            {
                Test.AddError(FString::Printf(TEXT("Patrol course timed out in phase %d"), Phase));
                return true;
            }

            UWorld* World = GEditor->PlayWorld.Get();
            APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
            auto* Player = PlayerController ? Cast<AHomeworkCharacter>(PlayerController->GetPawn()) : nullptr;
            if (!Player || !Player->HasActorBegunPlay()) return false;

            TActorIterator<AHomeworkGuard> Guards(World);
            if (!Test.TestTrue(TEXT("Map has a guard"), !!Guards)) return true;
            AHomeworkGuard* Guard = *Guards;
            auto* Controller = Cast<AHomeworkGuardController>(Guard->GetController());
            auto* Tree = Controller ? Cast<UBehaviorTreeComponent>(Controller->GetBrainComponent()) : nullptr;
            UBlackboardComponent* Board = Controller ? Controller->GetBlackboardComponent() : nullptr;
            if (!Board || !Tree || !Tree->GetCurrentTree()) return false;

            auto Key = [PlayerController](FKey Name, EInputEvent Event)
            {
                PlayerController->InputKey(FInputKeyEventArgs::CreateSimulated(Name, Event, Event == IE_Pressed ? 1.f : 0.f));
            };
            auto Place = [](ACharacter* Character, FVector Location)
            {
                Character->GetCharacterMovement()->StopMovementImmediately();
                Character->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
            };
            auto* Target = Board->GetValueAsObject(AHomeworkGuardController::TargetActorKey);
            const bool bNoise = Board->GetValueAsBool(AHomeworkGuardController::HasNoiseKey);
            const UBTNode* Active = Tree->GetActiveNode();
            const FString Node = Active ? Active->GetNodeName() : FString();
            const double Now = World->GetTimeSeconds();

            switch (Phase)
            {
            case 0:
            {
                const auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
                FNavLocation WalkablePoint;
                if (!Nav || !Nav->ProjectPointToNavigation(Guard->GetActorLocation(), WalkablePoint, FVector(80, 80, 250))) break;
                int32 Props = 0;
                int32 BlockedPropCenters = 0;
                for (TActorIterator<APCGVolume> Zones(World); Zones; ++Zones)
                {
                    TInlineComponentArray<UInstancedStaticMeshComponent*> Meshes(*Zones);
                    for (const auto* Mesh : Meshes)
                    {
                        Test.TestTrue(TEXT("PCG prop has query collision"), Mesh->IsQueryCollisionEnabled());
                        Test.TestTrue(TEXT("PCG prop affects navigation"), Mesh->CanEverAffectNavigation());
                        Props += Mesh->GetInstanceCount();
                        for (int32 Index = 0; Index < Mesh->GetInstanceCount(); ++Index)
                        {
                            FTransform Transform;
                            if (Mesh->GetInstanceTransform(Index, Transform, true))
                            {
                                const FVector Center = Transform.GetLocation();
                                if (!Nav->ProjectPointToNavigation(FVector(Center.X, Center.Y, 5), WalkablePoint, FVector(10, 10, 35)))
                                {
                                    ++BlockedPropCenters;
                                }
                            }
                        }
                    }
                }
                Test.TestTrue(TEXT("Generated PCG props are present after loading the map"), Props >= 20);
                Test.TestEqual(TEXT("Recast excludes the ground under PCG props"), BlockedPropCenters, Props);
                Test.TestTrue(TEXT("HW1 interaction Blueprint is active on the new map"), Player->OnInteraction.IsBound());
                Phase = 1;
                break;
            }
            case 1:
                if (FVector::DistSquared2D(Guard->GetActorLocation(), Board->GetValueAsVector(AHomeworkGuardController::HomeLocationKey)) < FMath::Square(100.0)) break;
                Test.TestTrue(TEXT("Guard patrols before detecting a target"), Target == nullptr && !bNoise);
                Place(Guard, FVector(0, 0, 98));
                Place(Player, FVector(-1570, 0, 98));
                PlayerController->SetControlRotation(FRotator::ZeroRotator);
                Key(EKeys::LeftShift, IE_Pressed);
                PhaseTime = Now;
                Phase = 2;
                break;
            case 2:
                if (Now - PhaseTime < 0.35) break;
                Test.TestTrue(TEXT("Standing still while holding Sprint does not make noise"), !bNoise);
                Test.TestTrue(TEXT("Shift starts the HW1 sprint through Enhanced Input"), Player->bIsSprinting);
                Key(EKeys::D, IE_Pressed);
                PhaseTime = Now;
                Phase = 3;
                break;
            case 3:
                if (!bNoise || Node != TEXT("Approach Sound"))
                {
                    if (Now - PhaseTime > 1.25)
                    {
                        TArray<AActor*> Heard;
                        if (const auto* Perception = Controller->GetAIPerceptionComponent())
                        {
                            Perception->GetCurrentlyPerceivedActors(UAISense_Hearing::StaticClass(), Heard);
                        }
                        Test.AddError(FString::Printf(TEXT("No investigation after sprint. Sprint=%d Velocity=%s Player=%s Guard=%s Heard=%d HasNoise=%d Target=%s Node=%s"),
                            Player->bIsSprinting, *Player->GetVelocity().ToCompactString(), *Player->GetActorLocation().ToCompactString(),
                            *Guard->GetActorLocation().ToCompactString(), Heard.Contains(Player), bNoise, *GetNameSafe(Target), *Node));
                        Key(EKeys::D, IE_Released);
                        Key(EKeys::LeftShift, IE_Released);
                        return true;
                    }
                    break;
                }
                Test.TestTrue(TEXT("Real sprint input triggers hearing with no line of sight"), Player->bIsSprinting && Target == nullptr);
                Key(EKeys::D, IE_Released);
                Key(EKeys::LeftShift, IE_Released);
                Place(Player, FVector(-2880, -1830, 98));
                Phase = 4;
                break;
            case 4:
                if (bNoise && (Node == TEXT("Check Nearby Point") || (Active && Active->IsA<UBTTask_Wait>())))
                {
                    bReachedSearchPoint = true;
                }
                if (bNoise) break;
                Test.TestTrue(TEXT("Guard navigates around cover and searches near the sprint sound"), bReachedSearchPoint);
                Place(Guard, FVector(0, 0, 98));
                Place(Player, FVector(800, 0, 98));
                Phase = 5;
                break;
            case 5:
                if (Target != Player || Node != TEXT("Chase Player") || Guard->GetVelocity().X < 300.0) break;
                Place(Player, FVector(-2880, -1830, 98));
                Phase = 6;
                break;
            case 6:
                if (Target || Node != TEXT("Walk to Patrol Point")) break;
                Place(Guard, FVector(0, 0, 98));
                Place(Player, FVector(800, 0, 98));
                Phase = 7;
                break;
            case 7:
                if (Target != Player || Node != TEXT("Chase Player")) break;
                FleeStartDistance = FVector::Dist2D(Guard->GetActorLocation(), Player->GetActorLocation());
                Key(EKeys::E, IE_Pressed);
                Phase = 8;
                break;
            case 8:
                if (!Board->GetValueAsBool(AHomeworkGuardController::ShouldFleeKey)) break;
                Key(EKeys::E, IE_Released);
                Test.TestNull(TEXT("E interrupts the active chase"), Target);
                Test.TestTrue(TEXT("The flee task received the player's position"),
                    Board->GetValueAsVector(AHomeworkGuardController::FleeFromLocationKey).Equals(Player->GetActorLocation(), 5.0));
                Phase = 9;
                break;
            case 9:
                if (Node != TEXT("Run Away") || FVector::Dist2D(Guard->GetActorLocation(), Player->GetActorLocation()) < FleeStartDistance + 150.0) break;
                Place(Player, FVector(-2880, -1830, 98));
                Phase = 10;
                break;
            case 10:
                if (Board->GetValueAsBool(AHomeworkGuardController::ShouldFleeKey) || Target || Node != TEXT("Walk to Patrol Point")) break;
                return true;
            }
            return false;
        }

    private:
        FAutomationTestBase& Test;
        double Started = FPlatformTime::Seconds();
        double PhaseTime = 0.0;
        double FleeStartDistance = 0.0;
        int32 Phase = 0;
        bool bReachedSearchPoint = false;
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPatrolCourseTest, "HW2.Integration.PatrolCourse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPatrolCourseTest::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/HW2/Lvl_PatrolCourse")));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FCheckPatrolCourse>(*this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
