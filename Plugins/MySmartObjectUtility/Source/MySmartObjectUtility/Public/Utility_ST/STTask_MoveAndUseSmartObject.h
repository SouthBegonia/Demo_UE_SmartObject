#pragma once

#include "AISystem.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "StateTreeTaskBase.h"
#include "SmartObjectSubsystem.h"
#include "STTask_MoveAndUseSmartObject.generated.h"


class AActor;
class AAIController;
class IGameplayTaskOwnerInterface;
class UAITask_MoveTo;
class UGameplayBehavior;

struct FAIMoveRequest;


USTRUCT()
struct FStateTreeMoveAndUseSmartObjectInstanceData
{
	GENERATED_BODY()

	#pragma region Input/Context

	UPROPERTY(EditAnywhere, Category = Input)
	FSmartObjectClaimHandle SOClaimHandle;

	UPROPERTY(EditAnywhere, Category = Input)
	FTransform SOGoalTransform;

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	#pragma endregion


	#pragma region Parameter

	/** fixed distance added to threshold between AI and goal location in destination reach test */
	UPROPERTY(EditAnywhere, Category = Parameter, meta=(ClampMin = "0.0", UIMin="0.0"))
	float AcceptableRadius = 10.f; //GET_AI_CONFIG_VAR(AcceptanceRadius);

	/** "None" will result in default filter being used */
	UPROPERTY(EditAnywhere, Category = Parameter)
	TSubclassOf<UNavigationQueryFilter> FilterClass;

	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bAllowStrafe = GET_AI_CONFIG_VAR(bAllowStrafing);

	/** if set, use incomplete path when goal can't be reached */
	//UPROPERTY(EditAnywhere, Category = Parameter)
	bool bAllowPartialPath = false;//GET_AI_CONFIG_VAR(bAcceptPartialPaths);

	/** if set, the goal location will need to be navigable */
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bRequireNavigableEndLocation = true;

	/** if set, goal location will be projected on navigation data (navmesh) before using */
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bProjectGoalLocation = true;

	/** if set, radius of AI's capsule will be added to threshold between AI and goal location in destination reach test  */
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bReachTestIncludesAgentRadius = GET_AI_CONFIG_VAR(bFinishMoveOnGoalOverlap);

	/** if set, radius of goal's capsule will be added to threshold between AI and goal location in destination reach test  */
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bReachTestIncludesGoalRadius = GET_AI_CONFIG_VAR(bFinishMoveOnGoalOverlap);


	UPROPERTY(EditAnywhere, Category = Parameter, meta=(ToolTip = "Warping Pawn to SlotTransform"))
	FName SlotMotionWarpingName = TEXT("SmartObjectWarp");

	#pragma endregion


	UPROPERTY(Transient)
	TObjectPtr<UGameplayBehavior> GameplayBehavior = nullptr;

	UPROPERTY(Transient)
	bool bAbortTag = false;
	UPROPERTY(Transient)
	bool bBehaviorFinished = false;

	UPROPERTY(Transient)
	TObjectPtr<UAITask_MoveTo> MoveToTask = nullptr;

	UPROPERTY(Transient)
	TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner = nullptr;

	FDelegateHandle OnBehaviorFinishedNotifyHandle;
	FDelegateHandle OnReceiveSmartObjectEventDelegateHandle;
};

/**
 * Task for moving to a SmartObject and using it
 */
USTRUCT(meta = (DisplayName = "Move And Use SmartObject", Category = "AI|SmartObject"))
struct FSTTask_MoveAndUseSmartObject : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeMoveAndUseSmartObjectInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	virtual UAITask_MoveTo* PrepareMoveToTask(FStateTreeExecutionContext& Context, AAIController& Controller, UAITask_MoveTo* ExistingTask, FAIMoveRequest& MoveRequest) const;
	virtual EStateTreeRunStatus PerformMoveTask(FStateTreeExecutionContext& Context, AAIController& Controller) const;

	virtual bool StartInteraction(FStateTreeExecutionContext& Context) const;


	TStateTreeExternalDataHandle<USmartObjectSubsystem> SmartObjectSubsystemHandle;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = Text) const override;
	virtual FName GetIconName() const override
	{
		return FName("StateTreeEditorStyle|Node.Movement");
	}
	virtual FColor GetIconColor() const override
	{
		return UE::StateTree::Colors::Grey;
	}
#endif // WITH_EDITOR
};
