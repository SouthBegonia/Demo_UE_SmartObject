// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility_BT/BTTask_GetClaimedSmartObjectSlotTransform.h"

#include "AIController.h"
#include "BlackboardKeyType_SOClaimHandle.h"
#include "BehaviorTree/BlackboardComponent.h"

EBTNodeResult::Type UBTTask_GetClaimedSmartObjectSlotTransform::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UWorld* World = GetWorld();
	USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(World);
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();


	if (SmartObjectSubsystem == nullptr
		|| BlackboardComp == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	// Check ClaimHandle is valid.
	FSmartObjectClaimHandle ClaimHandle;
	GetSOClaimHandle(OwnerComp, SOClaimedHandleBlackboardKey, ClaimHandle);
	if (!IsClaimedSmartObjectValid(OwnerComp, ClaimHandle))
	{
		return EBTNodeResult::Failed;
	}

	bool bHasResult = false;
	FTransform TargetTransform;

	FTransform EntranceTransform;
	bHasResult = GetClaimedSlotEntranceTransform(OwnerComp, ClaimHandle, EntranceTransform);
	if (bHasResult)
	{
		TargetTransform = EntranceTransform;
	}
	else
	{
		FTransform SlotTransform;
		bHasResult = GetClaimedSlotTransform(OwnerComp, ClaimHandle, SlotTransform);
		TargetTransform = SlotTransform;
	}

	if (bHasResult)
	{
		if (!ResultLocationKeyName.IsNone())
			BlackboardComp->SetValueAsVector(ResultLocationKeyName, TargetTransform.GetLocation());
		if (!ResultRotatorKeyName.IsNone())
			BlackboardComp->SetValueAsRotator(ResultRotatorKeyName, TargetTransform.GetRotation().Rotator());
	}

	return bHasResult ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

bool UBTTask_GetClaimedSmartObjectSlotTransform::GetClaimedSlotTransform(const UBehaviorTreeComponent& OwnerComp, const FSmartObjectClaimHandle& ClaimHandle, FTransform& OutSlotTransform)
{
	bool bHasResult = false;

	USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(OwnerComp.GetWorld());
	if (SmartObjectSubsystem != nullptr)
	{
		const TOptional<FTransform> GoalTransform = SmartObjectSubsystem->GetSlotTransform(ClaimHandle);

		if (GoalTransform.IsSet())
		{
			OutSlotTransform = GoalTransform.GetValue();
			bHasResult = true;
		}
	}

	return bHasResult;
}

bool UBTTask_GetClaimedSmartObjectSlotTransform::GetClaimedSlotEntranceTransform(const UBehaviorTreeComponent& OwnerComp, const FSmartObjectClaimHandle& ClaimHandle, FTransform& OutEntranceTransform)
{
	bool bHasResult = false;

	const USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(OwnerComp.GetWorld());
	if (SmartObjectSubsystem != nullptr)
	{
		FSmartObjectSlotEntranceLocationRequest& Request = EntranceRequest;
		Request.UserActor = Cast<AAIController>(OwnerComp.GetOwner())->GetPawn();
		FSmartObjectSlotEntranceLocationResult Result;
		if (SmartObjectSubsystem->FindEntranceLocationForSlot(ClaimHandle.SlotHandle, Request, Result))
		{
			OutEntranceTransform = FTransform(Result.Rotation, Result.Location);
			bHasResult = true;
		}
	}

	return bHasResult;
}


bool UBTTask_GetClaimedSmartObjectSlotTransform::IsClaimedSmartObjectValid(const UBehaviorTreeComponent& OwnerComp, const FSmartObjectClaimHandle& ClaimHandle) const
{
	if (!ClaimHandle.IsValid())
		return false;

	const USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(OwnerComp.GetWorld());
	if (SmartObjectSubsystem != nullptr)
	{
		return SmartObjectSubsystem->IsClaimedSmartObjectValid(ClaimHandle);
	}

	return false;
}

FString UBTTask_GetClaimedSmartObjectSlotTransform::GetStaticDescription() const
{
	FString Result;
	if (SOClaimedHandleBlackboardKey.SelectedKeyType != UBlackboardKeyType_SOClaimHandle::StaticClass())
	{
		Result += FString::Printf(TEXT("SOClaimedHandleBlackboardKey must be type of SOClaimHandle"));
	}

	if (ResultLocationKeyName.IsNone() && ResultRotatorKeyName.IsNone())
	{
		Result += FString::Printf(TEXT("ResultLocationKeyName or ResultRotatorKeyName can't be none"));
	}

	if (Result.Len() > 0)
		return Result;

	Result += FString(TEXT("Result in BBKey : "));
	if (!ResultLocationKeyName.IsNone())
		Result += FString::Printf(TEXT(" %s "), *ResultLocationKeyName.ToString());
	if (!ResultRotatorKeyName.IsNone())
		Result += FString::Printf(TEXT(" %s "), *ResultRotatorKeyName.ToString());

    return Result;
}

bool UBTTask_GetClaimedSmartObjectSlotTransform::GetSOClaimHandle(const UBehaviorTreeComponent& OwnerComp, const FBlackboardKeySelector& ClaimHandleKey, FSmartObjectClaimHandle& OutClaimHandle)
{
	if (const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent())
	{
		OutClaimHandle = BlackboardComp->GetValue<UBlackboardKeyType_SOClaimHandle>(ClaimHandleKey.SelectedKeyName);
		return true;
	}

	return false;
}
