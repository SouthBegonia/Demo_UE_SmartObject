#include "Utility_ST/STTask_MoveAndUseSmartObject.h"
#include "AIController.h"
#include "GameplayBehavior.h"
#include "GameplayBehaviorConfig.h"
#include "GameplayBehaviorSmartObjectBehaviorDefinition.h"
#include "GameplayBehaviorSubsystem.h"
#include "MotionWarpingComponent.h"
#include "SmartObjectComponent.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "Interface/SmartObjectInteracteeInterface.h"
#include "Interface/SmartObjectInteractorInterface.h"
#include "Tasks/AITask_MoveTo.h"


#define LOCTEXT_NAMESPACE "GameplayStateTree"

bool FSTTask_MoveAndUseSmartObject::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(SmartObjectSubsystemHandle);

	return true;
}

EStateTreeRunStatus FSTTask_MoveAndUseSmartObject::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.AIController == nullptr || InstanceData.AIController->GetPawn() == nullptr)
	{
		UE_VLOG(Context.GetOwner(), LogStateTree, Error, TEXT("FSTTask_MoveAndUseSmartObject failed since AIController or AIPawn is null."));
		return EStateTreeRunStatus::Failed;
	}

	AActor* AIPawn = InstanceData.AIController->GetPawn();

	USmartObjectSubsystem* SmartObjectSubsystem = Context.GetExternalDataPtr(SmartObjectSubsystemHandle);
	//USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(Context.GetWorld());
	if (SmartObjectSubsystem == nullptr)
	{
		UE_VLOG(Context.GetOwner(), LogStateTree, Error, TEXT("FSTTask_MoveAndUseSmartObject failed since SmartObjectSubsystem is null."));
		return EStateTreeRunStatus::Failed;
	}

	// Check ClaimHandle
	const FSmartObjectClaimHandle& SOClaimHandle = InstanceData.SOClaimHandle;
	if (!SOClaimHandle.IsValid() || !SmartObjectSubsystem->IsClaimedSmartObjectValid(SOClaimHandle))
	{
		return EStateTreeRunStatus::Failed;
	}
	if (AIPawn->Implements<USmartObjectInteractorInterface>())
		Cast<ISmartObjectInteractorInterface>(AIPawn)->SetSOClaimHandle(SOClaimHandle);

	// Set MotionWarping
	if (!InstanceData.SlotMotionWarpingName.IsNone())
	{
		UMotionWarpingComponent* MotionWarpingComp = InstanceData.AIController->GetPawn()->GetComponentByClass<UMotionWarpingComponent>();

		if (MotionWarpingComp != nullptr)
			MotionWarpingComp->AddOrUpdateWarpTargetFromTransform(FName(TEXT("SmartObjectWarp")), InstanceData.SOGoalTransform);
	}

	// Register SlotInvalidationEvent
	// TODO : switch to a MulticastDelegate instead of OnSlotInvalidatedDelegate(Delegate)
	SmartObjectSubsystem->RegisterSlotInvalidationCallback(SOClaimHandle, FOnSlotInvalidated::CreateLambda([InstanceDataRef = Context.GetInstanceDataStructRef(*this)](const FSmartObjectClaimHandle& InnerSOClaimHandle, ESmartObjectSlotState InnerSlotState)
	{
		if (FInstanceDataType* InnerInstanceData = InstanceDataRef.GetPtr())
			InnerInstanceData->bAbortTag = true;
	}));
	// Register SmartObjectEvent
	if (FOnSmartObjectEvent* SmartObjectDelegate = SmartObjectSubsystem->GetSlotEventDelegate(SOClaimHandle.SlotHandle))
	{
		InstanceData.OnReceiveSmartObjectEventDelegateHandle = SmartObjectDelegate->AddLambda([InstanceDataRef = Context.GetInstanceDataStructRef(*this)](const FSmartObjectEventData& InnerEvent)
		{
			if (InnerEvent.Reason == ESmartObjectChangeReason::OnSlotDisabled || InnerEvent.Reason == ESmartObjectChangeReason::OnObjectDisabled)
			{
				if (FInstanceDataType* InnerInstanceData = InstanceDataRef.GetPtr())
					InnerInstanceData->bAbortTag = true;
			}
		});
	}


	InstanceData.TaskOwner = TScriptInterface<IGameplayTaskOwnerInterface>(InstanceData.AIController->FindComponentByInterface(UGameplayTaskOwnerInterface::StaticClass()));
	if (!InstanceData.TaskOwner)
	{
		InstanceData.TaskOwner = InstanceData.AIController;
	}

	return PerformMoveTask(Context, *InstanceData.AIController);;
}

EStateTreeRunStatus FSTTask_MoveAndUseSmartObject::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// Abort (slot、SO being invalid
	if (InstanceData.bAbortTag)
		return EStateTreeRunStatus::Failed;

	// Success (Behavior done
	if (InstanceData.bBehaviorFinished)
		return EStateTreeRunStatus::Succeeded;

	if (InstanceData.MoveToTask)
	{
		if (InstanceData.MoveToTask->IsFinished())
		{
			if (InstanceData.MoveToTask->WasMoveSuccessful())
			{
				InstanceData.MoveToTask = nullptr;

				if (StartInteraction(Context) == false)
					return EStateTreeRunStatus::Failed;
			}
			else
				return EStateTreeRunStatus::Failed;	// AutoMove Failed
		}
		else
			return EStateTreeRunStatus::Running;	// AutoMoving
	}

	// still processing (Interacting
	return EStateTreeRunStatus::Running;
}

void FSTTask_MoveAndUseSmartObject::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// Stop AutoMoveTask
	if (InstanceData.MoveToTask)
	{
		UAITask_MoveTo* Task = InstanceData.MoveToTask;
		InstanceData.MoveToTask = nullptr;
		Task->ExternalCancel();
	}

	if (InstanceData.GameplayBehavior != nullptr)
	{
		// Unregister BehaviorFinishedEvent
		if (InstanceData.OnBehaviorFinishedNotifyHandle.IsValid())
			InstanceData.GameplayBehavior->GetOnBehaviorFinishedDelegate().Remove(InstanceData.OnBehaviorFinishedNotifyHandle);

		// Stop Behavior
		if (!InstanceData.bBehaviorFinished)
		{
			check(InstanceData.AIController);
			check(InstanceData.AIController->GetPawn());
			InstanceData.GameplayBehavior->AbortBehavior(*InstanceData.AIController->GetPawn());
		}
	}

	FSmartObjectClaimHandle& SOClaimHandle = InstanceData.SOClaimHandle;
	USmartObjectSubsystem* SmartObjectSubsystem = Context.GetExternalDataPtr(SmartObjectSubsystemHandle);
	//USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(Context.GetWorld());
	if (SmartObjectSubsystem && SOClaimHandle.IsValid())
	{
		// Operate the interface for Interactor/Interactee. You can remove the code according to your project
		const USmartObjectComponent* SmartObjectComponent = SmartObjectSubsystem->GetSmartObjectComponent(SOClaimHandle);
		AActor* InteractorActor = InstanceData.AIController ? InstanceData.AIController->GetPawn() : nullptr;
		AActor* InteracteeActor = SmartObjectComponent ? SmartObjectComponent->GetOwner() : nullptr;
		if (InteractorActor && InteractorActor->Implements<USmartObjectInteractorInterface>())
		{
			ISmartObjectInteractorInterface* InteractorInterface = Cast<ISmartObjectInteractorInterface>(InteractorActor);
			InteractorInterface->InvalidateSOClaimHandle();
			if (InstanceData.OnBehaviorFinishedNotifyHandle.IsValid())	// means started interaction successfully before
				InteractorInterface->OnInteractionEnd();
		}
		if (InteracteeActor && InteracteeActor->Implements<USmartObjectInteracteeInterface>())
		{
			ISmartObjectInteracteeInterface* InteracteeInterface = Cast<ISmartObjectInteracteeInterface>(InteracteeActor);
			InteracteeInterface->OnSlotInteractionEnd();
		}

		// Unregister SmartObjectEvent
		if (InstanceData.OnReceiveSmartObjectEventDelegateHandle.IsValid())
		{
			if (FOnSmartObjectEvent* SmartObjectDelegate = SmartObjectSubsystem->GetSlotEventDelegate(SOClaimHandle.SlotHandle))
				SmartObjectDelegate->Remove(InstanceData.OnReceiveSmartObjectEventDelegateHandle);
			InstanceData.OnReceiveSmartObjectEventDelegateHandle.Reset();
		}

		// Unregister SlotInvalidationEvent
		SmartObjectSubsystem->UnregisterSlotInvalidationCallback(SOClaimHandle);

		// Free Slot
		SmartObjectSubsystem->MarkSlotAsFree(SOClaimHandle);
		SOClaimHandle.Invalidate();
	}
}

bool FSTTask_MoveAndUseSmartObject::StartInteraction(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	//USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(Context.GetWorld());
	USmartObjectSubsystem* SmartObjectSubsystem = Context.GetExternalDataPtr(SmartObjectSubsystemHandle);
	if (!SmartObjectSubsystem)
	{
		return false;
	}

	// Check ClaimHandle
	const FSmartObjectClaimHandle& SOClaimHandle = InstanceData.SOClaimHandle;
	if (!SOClaimHandle.IsValid() || !SmartObjectSubsystem->IsClaimedSmartObjectValid(SOClaimHandle))
	{
		UE_VLOG(Context.GetOwner(), LogStateTree, Error, TEXT("FSTTask_MoveAndUseSmartObject StartInteraction failed since SOClaimHandle or SO is invalid."));
		return false;
	}

	const UGameplayBehaviorSmartObjectBehaviorDefinition* SmartObjectGameplayBehaviorDefinition = SmartObjectSubsystem->MarkSlotAsOccupied<UGameplayBehaviorSmartObjectBehaviorDefinition>(SOClaimHandle);
	const UGameplayBehaviorConfig* GameplayBehaviorConfig = SmartObjectGameplayBehaviorDefinition != nullptr ? SmartObjectGameplayBehaviorDefinition->GameplayBehaviorConfig : nullptr;
	InstanceData.GameplayBehavior = GameplayBehaviorConfig != nullptr ? GameplayBehaviorConfig->GetBehavior(*Context.GetWorld()) : nullptr;
	if (InstanceData.GameplayBehavior == nullptr)
	{
		UE_VLOG(Context.GetOwner(), LogStateTree, Error, TEXT("FSTTask_MoveAndUseSmartObject StartInteraction failed GameplayBehavior is null."));
		return false;
	}

	const USmartObjectComponent* SmartObjectComponent = SmartObjectSubsystem->GetSmartObjectComponent(SOClaimHandle);
	AActor& InteractorActor = *InstanceData.AIController->GetPawn();
	AActor* InteracteeActor = SmartObjectComponent ? SmartObjectComponent->GetOwner() : nullptr;
	const bool bBehaviorActive = UGameplayBehaviorSubsystem::TriggerBehavior(*InstanceData.GameplayBehavior, InteractorActor, GameplayBehaviorConfig, InteracteeActor);
	// Behavior can be successfully triggered AND ended synchronously. We are only interested to register callback when still running
	if (bBehaviorActive)
	{
		InstanceData.OnBehaviorFinishedNotifyHandle = InstanceData.GameplayBehavior->GetOnBehaviorFinishedDelegate().AddLambda([InstanceDataRef = Context.GetInstanceDataStructRef(*this)](UGameplayBehavior& InnerBehavior, AActor& InnerAvatar, const bool bInterrupted)
		{
			if (FInstanceDataType* InnerInstanceData = InstanceDataRef.GetPtr())
			{
				InnerBehavior.GetOnBehaviorFinishedDelegate().Remove(InnerInstanceData->OnBehaviorFinishedNotifyHandle);
				InnerInstanceData->bBehaviorFinished = true;
			}
		});

		// Operate the interface for Interactor/Interactee. You can remove the code according to your project
		if (InteractorActor.Implements<USmartObjectInteractorInterface>())
		{
			ISmartObjectInteractorInterface* InteractorInterface = Cast<ISmartObjectInteractorInterface>(&InteractorActor);
			InteractorInterface->OnInteractionBegin(SOClaimHandle, InteracteeActor);
		}
		if (InteracteeActor->Implements<USmartObjectInteracteeInterface>())
		{
			ISmartObjectInteracteeInterface* InteracteeInterface = Cast<ISmartObjectInteracteeInterface>(InteracteeActor);
			InteracteeInterface->OnSlotInteractionBegin(SOClaimHandle, &InteractorActor);
		}
	}
	else
		UE_VLOG(Context.GetOwner(), LogStateTree, Error, TEXT("FSTTask_MoveAndUseSmartObject StartInteraction failed since activate Behavior failed."));

	return bBehaviorActive;
}


UAITask_MoveTo* FSTTask_MoveAndUseSmartObject::PrepareMoveToTask(FStateTreeExecutionContext& Context, AAIController& Controller, UAITask_MoveTo* ExistingTask, FAIMoveRequest& MoveRequest) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	UAITask_MoveTo* MoveTask = ExistingTask ? ExistingTask : UAITask::NewAITask<UAITask_MoveTo>(Controller, *InstanceData.TaskOwner);
	if (MoveTask)
	{
		MoveTask->SetUp(&Controller, MoveRequest);
	}

	return MoveTask;
}

EStateTreeRunStatus FSTTask_MoveAndUseSmartObject::PerformMoveTask(FStateTreeExecutionContext& Context, AAIController& Controller) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	FAIMoveRequest MoveReq;
	MoveReq.SetNavigationFilter(InstanceData.FilterClass ? InstanceData.FilterClass : Controller.GetDefaultNavigationFilterClass())
		.SetAllowPartialPath(InstanceData.bAllowPartialPath)
		.SetAcceptanceRadius(InstanceData.AcceptableRadius)
		.SetCanStrafe(InstanceData.bAllowStrafe)
		.SetReachTestIncludesAgentRadius(InstanceData.bReachTestIncludesAgentRadius)
		.SetReachTestIncludesGoalRadius(InstanceData.bReachTestIncludesGoalRadius)
		.SetRequireNavigableEndLocation(InstanceData.bRequireNavigableEndLocation)
		.SetProjectGoalLocation(InstanceData.bProjectGoalLocation)
		.SetUsePathfinding(true);

	MoveReq.SetGoalLocation(InstanceData.SOGoalTransform.GetLocation());

	if (MoveReq.IsValid())
	{
		InstanceData.MoveToTask = PrepareMoveToTask(Context, Controller, InstanceData.MoveToTask, MoveReq);
		if (InstanceData.MoveToTask)
		{
			if (InstanceData.MoveToTask->IsActive())
			{
				InstanceData.MoveToTask->ConditionalPerformMove();
			}
			else
			{
				InstanceData.MoveToTask->ReadyForActivation();
			}

			if (InstanceData.MoveToTask->GetState() == EGameplayTaskState::Finished)
			{
				return InstanceData.MoveToTask->WasMoveSuccessful() ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
			}

			return EStateTreeRunStatus::Running;
		}
	}

	UE_VLOG(Context.GetOwner(), LogStateTree, Error, TEXT("FSTTask_MoveAndUseSmartObject failed because it doesn't have a destination."));
	return EStateTreeRunStatus::Failed;
}

#if WITH_EDITOR

FText FSTTask_MoveAndUseSmartObject::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText(LOCTEXT("MoveAndUseSmartObject", "Move And Use SmartObject"));
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE