// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility_BT/BTTask_GetClaimedSmartObjectSlotTransform.h"

#include "AIController.h"
#include "BlackboardKeyType_SOClaimHandle.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_GetClaimedSmartObjectSlotTransform::UBTTask_GetClaimedSmartObjectSlotTransform()
{
	// accept only UBlackboardKeyType_SOClaimHandle
	const FName PropertyName = GET_MEMBER_NAME_CHECKED(UBTTask_GetClaimedSmartObjectSlotTransform, SOClaimHandleBlackboardKey);
	const FString FilterName = PropertyName.ToString() + TEXT("_SOClaimHandle");
	SOClaimHandleBlackboardKey.AllowedTypes.Add(NewObject<UBlackboardKeyType_SOClaimHandle>(this, *FilterName));
	SOClaimHandleBlackboardKey.AllowNoneAsValue(false);

	ResultLocationBlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_GetClaimedSmartObjectSlotTransform, ResultLocationBlackboardKey));
	ResultLocationBlackboardKey.AllowNoneAsValue(true);
	ResultRotatorBlackboardKey.AddRotatorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_GetClaimedSmartObjectSlotTransform, ResultRotatorBlackboardKey));
	ResultRotatorBlackboardKey.AllowNoneAsValue(true);
}

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
	GetSOClaimHandle(OwnerComp, SOClaimHandleBlackboardKey, ClaimHandle);
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
		if (ResultLocationBlackboardKey.IsSet())
			BlackboardComp->SetValueAsVector(ResultLocationBlackboardKey.SelectedKeyName, TargetTransform.GetLocation());
		if (ResultRotatorBlackboardKey.IsSet())
			BlackboardComp->SetValueAsRotator(ResultRotatorBlackboardKey.SelectedKeyName, TargetTransform.GetRotation().Rotator());
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
	if (!SOClaimHandleBlackboardKey.IsSet())
	{
		return FString::Printf(TEXT("SOClaimedHandleBlackboardKey must be setting with type of SOClaimHandle."));
	}

	if (ResultLocationBlackboardKey.IsNone() && ResultRotatorBlackboardKey.IsNone())
	{
		return FString::Printf(TEXT("ResultLocationBlackboardKey or ResultRotatorBlackboardKey can't be none both."));
	}

	FString Result;

	Result += FString(TEXT("Result in BBKey : "));
	if (!ResultLocationBlackboardKey.IsNone())
		Result += FString::Printf(TEXT(" %s "), *ResultLocationBlackboardKey.SelectedKeyName.ToString());
	if (!ResultRotatorBlackboardKey.IsNone())
		Result += FString::Printf(TEXT(" %s "), *ResultRotatorBlackboardKey.SelectedKeyName.ToString());
	Result += FString::Printf(TEXT("\nSO ClaimHandle: %s"), *SOClaimHandleBlackboardKey.SelectedKeyName.ToString());

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
