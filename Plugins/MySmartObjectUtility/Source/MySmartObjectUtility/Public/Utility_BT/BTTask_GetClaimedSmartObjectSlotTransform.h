// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SmartObjectSubsystem.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_GetClaimedSmartObjectSlotTransform.generated.h"

/**
 * Task for getting the transform of a claimed slot from SmartObject
 */
UCLASS()
class MYSMARTOBJECTUTILITY_API UBTTask_GetClaimedSmartObjectSlotTransform : public UBTTaskNode
{
	GENERATED_BODY()

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blackboard")
	FBlackboardKeySelector SOClaimedHandleBlackboardKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blackboard")
	FName ResultLocationKeyName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Blackboard")
	FName ResultRotatorKeyName;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SmartObject")
	FSmartObjectSlotEntranceLocationRequest EntranceRequest;

	virtual FString GetStaticDescription() const override;

protected:
	bool GetSOClaimHandle(const UBehaviorTreeComponent& OwnerComp, const FBlackboardKeySelector& ClaimHandleKey, FSmartObjectClaimHandle& OutClaimHandle);

	bool IsClaimedSmartObjectValid(const UBehaviorTreeComponent& OwnerComp, const FSmartObjectClaimHandle& ClaimHandle) const;

	bool GetClaimedSlotTransform(const UBehaviorTreeComponent& OwnerComp, const FSmartObjectClaimHandle& ClaimHandle, FTransform& OutSlotTransform);
	bool GetClaimedSlotEntranceTransform(const UBehaviorTreeComponent& OwnerComp, const FSmartObjectClaimHandle& ClaimHandle, FTransform& OutEntranceTransform);
};
