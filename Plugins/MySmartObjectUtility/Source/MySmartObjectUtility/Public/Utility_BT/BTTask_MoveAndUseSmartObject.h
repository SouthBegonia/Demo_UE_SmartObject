// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SmartObjectSubsystem.h"
#include "BehaviorTree/Tasks/BTTask_BlueprintBase.h"
#include "BTTask_MoveAndUseSmartObject.generated.h"

class UGameplayBehavior;

UENUM(BlueprintType, DisplayName="GoalLocationType")
enum EGoalLocationTypeForMoveAndUseSmartObjectTask
{
	EntranceOrSlot,

	OnlyEntrance,

	OnlySlot,
};

/**
 * Task for moving to a SmartObject and using it
 */
UCLASS()
class MYSMARTOBJECTUTILITY_API UBTTask_MoveAndUseSmartObject : public UBTTask_BlueprintBase
{
	GENERATED_BODY()

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	virtual FString GetStaticDescription() const override;

	virtual void OnGameplayTaskDeactivated(UGameplayTask& Task) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SmartObject")
	FBlackboardKeySelector SOClaimHandleBlackboardKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SmartObject")
	FSmartObjectSlotEntranceLocationRequest EntranceRequest;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SmartObject")
	TEnumAsByte<EGoalLocationTypeForMoveAndUseSmartObjectTask> GoalLocationType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SmartObject", meta=(ToolTip = "Warping Pawn to SlotTransform"))
	FName SlotMotionWarpingName = TEXT("SmartObjectWarp");

protected:
	UPROPERTY()
	TObjectPtr<class UAITask_MoveTo> MoveToTask;

	UPROPERTY()
	TObjectPtr<UGameplayBehavior> GameplayBehavior;

	FSmartObjectClaimHandle ClaimedHandle;
	FDelegateHandle OnBehaviorFinishedNotifyHandle;
	FDelegateHandle OnReceiveSmartObjectEventDelegateHandle;

	bool bBehaviorFinished;


	bool StartInteraction();
	void Abort();

	void OnSmartObjectBehaviorFinished(UGameplayBehavior& Behavior, AActor& Avatar, const bool bInterrupted);
	void OnSlotInvalidated(const FSmartObjectClaimHandle& ClaimHandle, ESmartObjectSlotState State);
	void OnReceiveSmartObjectEvent( const FSmartObjectEventData& Event);

	bool GetGoalTransform(EGoalLocationTypeForMoveAndUseSmartObjectTask InGoalLocationType, const FSmartObjectClaimHandle& InClaimHandle, FTransform& OutGoalTransform);
	bool GetSlotTransform(FTransform& OutGoalTransform, const FSmartObjectClaimHandle& InClaimHandle) const;
	bool GetEntranceTransform(FTransform& OutGoalTransform, const FSmartObjectClaimHandle& InClaimHandle, FSmartObjectSlotEntranceLocationRequest& InEntranceRequest) const;
};

