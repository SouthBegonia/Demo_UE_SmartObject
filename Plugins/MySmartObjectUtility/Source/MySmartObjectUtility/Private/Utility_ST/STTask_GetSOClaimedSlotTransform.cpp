#include "Utility_ST/STTask_GetSOClaimedSlotTransform.h"

#include "StateTreeExecutionContext.h"

#define LOCTEXT_NAMESPACE "GameplayStateTree"

EStateTreeRunStatus FSTTask_GetSOClaimedSlotTransform::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	check(InstanceData.QueryOwner)

	FTransform GoalTransform;
	if (GetGoalLocation(GoalTransform, Context))
	{
		auto GoalTransformPtr = InstanceData.GoalTransform.GetMutablePtr<FTransform>(Context);
		if (GoalTransformPtr)
		{
			*GoalTransformPtr = GoalTransform;
			return EStateTreeRunStatus::Running;
		}
	}

	return EStateTreeRunStatus::Failed;
}

bool FSTTask_GetSOClaimedSlotTransform::GetGoalLocation(FTransform& OutGoalTransform, FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.GoalLocationType == EGoalLocationTypeForGetSOClaimedSlotTransformTask::EntranceOrSlot)
	{
		return GetEntranceLocation(OutGoalTransform, Context, InstanceData.QueryOwner)
			|| GetSlotLocation(OutGoalTransform, Context);
	}
	if (InstanceData.GoalLocationType == EGoalLocationTypeForGetSOClaimedSlotTransformTask::OnlyEntrance)
	{
		return GetEntranceLocation(OutGoalTransform, Context, InstanceData.QueryOwner);
	}
	if (InstanceData.GoalLocationType == EGoalLocationTypeForGetSOClaimedSlotTransformTask::OnlySlot)
	{
		return GetSlotLocation(OutGoalTransform, Context);
	}

	return false;
}

bool FSTTask_GetSOClaimedSlotTransform::GetSlotLocation(FTransform& OutTransform, FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (const USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(Context.GetWorld()); SmartObjectSubsystem != nullptr)
	{
		const TOptional<FTransform> GoalTransform = SmartObjectSubsystem->GetSlotTransform(InstanceData.SOClaimHandle);
		if (GoalTransform.IsSet())
		{
			OutTransform = GoalTransform.GetValue();
			return true;
		}
	}

	return false;
}

bool FSTTask_GetSOClaimedSlotTransform::GetEntranceLocation(FTransform& OutTransform, FStateTreeExecutionContext& Context, AActor* UserActor) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (const USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(Context.GetWorld()); SmartObjectSubsystem != nullptr)
	{
		FSmartObjectSlotEntranceLocationRequest& Request = InstanceData.EntranceRequest;
		Request.UserActor = UserActor;
		FSmartObjectSlotEntranceLocationResult Result;
		if (SmartObjectSubsystem->FindEntranceLocationForSlot(InstanceData.SOClaimHandle.SlotHandle, Request, Result))
		{
			OutTransform = FTransform(Result.Rotation, Result.Location);
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR

FText FSTTask_GetSOClaimedSlotTransform::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText(LOCTEXT("GetSlotTransform", "Get Slot Transform"));
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
