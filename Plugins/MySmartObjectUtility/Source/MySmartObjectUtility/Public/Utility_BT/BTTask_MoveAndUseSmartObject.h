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

	bool GetGoalLocation(EGoalLocationTypeForMoveAndUseSmartObjectTask InGoalLocationType, const FSmartObjectClaimHandle& InClaimHandle, FVector& OutGoalLocation);
	bool GetSlotLocation(FVector& OutLocation, const FSmartObjectClaimHandle& InClaimHandle) const;
	bool GetEntranceLocation(FVector& OutLocation, const FSmartObjectClaimHandle& InClaimHandle, FSmartObjectSlotEntranceLocationRequest& InEntranceRequest) const;
};

