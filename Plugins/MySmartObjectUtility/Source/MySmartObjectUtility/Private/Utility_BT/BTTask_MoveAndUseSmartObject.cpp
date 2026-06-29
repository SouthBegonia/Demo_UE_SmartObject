// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility_BT/BTTask_MoveAndUseSmartObject.h"
#include "AIController.h"
#include "Engine/World.h"
#include "BlackboardKeyType_SOClaimHandle.h"
#include "SmartObjectBlueprintFunctionLibrary.h"
#include "SmartObjectSubsystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "SmartObjectComponent.h"
#include "Tasks/AITask_MoveTo.h"
#include "GameplayBehavior.h"
#include "GameplayBehaviorConfig.h"
#include "GameplayBehaviorSubsystem.h"
#include "GameplayBehaviorSmartObjectBehaviorDefinition.h"
#include "Interface/SmartObjectInteracteeInterface.h"
#include "Interface/SmartObjectInteractorInterface.h"


EBTNodeResult::Type UBTTask_MoveAndUseSmartObject::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type NodeResult = EBTNodeResult::InProgress;

	UWorld* World = GetWorld();
	USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(World);
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	AAIController* MyController = OwnerComp.GetAIOwner();

	if (SmartObjectSubsystem == nullptr
		|| BlackboardComp == nullptr
		|| MyController == nullptr
		|| MyController->GetPawn() == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	// Check ClaimHandle and SO is still valid.
	FSmartObjectClaimHandle TempClaimHandle = USmartObjectBlueprintFunctionLibrary::GetValueAsSOClaimHandle(BlackboardComp, SOClaimHandleBlackboardKey.SelectedKeyName);
	if (!SmartObjectSubsystem->IsClaimedSmartObjectValid(TempClaimHandle))
	{
		return EBTNodeResult::Failed;
	}
	ClaimedHandle = TempClaimHandle;

	// Get GoalLocation
	FVector GoalLocation = FVector::ZeroVector;
	if (!GetGoalLocation(GoalLocationType, ClaimedHandle, GoalLocation))
	{
		return EBTNodeResult::Failed;
	}

	// Register SlotInvalidationEvent
	SmartObjectSubsystem->RegisterSlotInvalidationCallback(ClaimedHandle, FOnSlotInvalidated::CreateUObject(this, &UBTTask_MoveAndUseSmartObject::OnSlotInvalidated));
	// Register SmartObjectEvent
	if (FOnSmartObjectEvent* SmartObjectDelegate = SmartObjectSubsystem->GetSlotEventDelegate(ClaimedHandle.SlotHandle))
	{
		OnReceiveSmartObjectEventDelegateHandle = SmartObjectDelegate->AddUObject(this, &UBTTask_MoveAndUseSmartObject::OnReceiveSmartObjectEvent);
	}

	// Start Moving
	{
		FAIMoveRequest MoveReq(GoalLocation);
		MoveReq.SetUsePathfinding(true);
		MoveReq.SetAcceptanceRadius(15.f);
		MoveReq.SetAllowPartialPath(false);
		MoveReq.SetCanStrafe(false);

		MoveToTask = UAITask::NewAITask<UAITask_MoveTo>(*AIOwner, *this, EAITaskPriority::High, TEXT("SmartObject"));
		MoveToTask->SetUp(AIOwner, MoveReq);
		MoveToTask->ReadyForActivation();
	}

	return NodeResult;
}

EBTNodeResult::Type UBTTask_MoveAndUseSmartObject::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Abort();
	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTask_MoveAndUseSmartObject::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (TaskResult != EBTNodeResult::InProgress)
	{
		USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(AIOwner->GetWorld());
		check(SmartObjectSubsystem);

		if (OnReceiveSmartObjectEventDelegateHandle.IsValid())
		{
			if (FOnSmartObjectEvent* SmartObjectDelegate = SmartObjectSubsystem->GetSlotEventDelegate(ClaimedHandle.SlotHandle))
				SmartObjectDelegate->Remove(OnReceiveSmartObjectEventDelegateHandle);
			OnReceiveSmartObjectEventDelegateHandle.Reset();
		}

		if (ClaimedHandle.IsValid())
		{
			// Operate the interface for Interactor/Interactee. You can remove the code according to your project
			const USmartObjectComponent* SmartObjectComponent = SmartObjectSubsystem->GetSmartObjectComponent(ClaimedHandle);
			AActor* InteractorActor = AIOwner->GetPawn();
			AActor* InteracteeActor = SmartObjectComponent ? SmartObjectComponent->GetOwner() : nullptr;
			if (InteractorActor && InteractorActor->Implements<USmartObjectInteractorInterface>())
			{
				ISmartObjectInteractorInterface* InteractorInterface = Cast<ISmartObjectInteractorInterface>(InteractorActor);
				InteractorInterface->OnInteractionEnd();
				InteractorInterface->InvalidateSOClaimHandle(SOClaimHandleBlackboardKey.SelectedKeyName);
			}
			if (InteracteeActor && InteracteeActor->Implements<USmartObjectInteracteeInterface>())
			{
				ISmartObjectInteracteeInterface* InteracteeInterface = Cast<ISmartObjectInteracteeInterface>(InteracteeActor);
				InteracteeInterface->OnSlotInteractionEnd();
			}

			// Free Slot and unregister events
			SmartObjectSubsystem->UnregisterSlotInvalidationCallback(ClaimedHandle);
			SmartObjectSubsystem->MarkSlotAsFree(ClaimedHandle);
			ClaimedHandle.Invalidate();
		}
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

FString UBTTask_MoveAndUseSmartObject::GetStaticDescription() const
{
	FString Result;
	if (SOClaimHandleBlackboardKey.SelectedKeyType != UBlackboardKeyType_SOClaimHandle::StaticClass())
	{
		Result += FString::Printf(TEXT("SOClaimHandleBlackboardKey must be type of SOClaimHandle"));
	}

	if (Result.Len() > 0)
		return Result;

	Result += FString::Printf(TEXT("Move and UseSO with SOClaimHandle : %s"), *SOClaimHandleBlackboardKey.SelectedKeyName.ToString());
	return Result;
}

void UBTTask_MoveAndUseSmartObject::OnGameplayTaskDeactivated(UGameplayTask& Task)
{
	if (MoveToTask == &Task)
	{
		// Check MoveTask is finished
		if (MoveToTask->IsFinished())
		{
			if (MoveToTask->WasMoveSuccessful())
			{
				MoveToTask = nullptr;
				if (StartInteraction() == false)
				{
					FinishExecute(false);
				}
			}
			else
			{
				FinishExecute(false);
			}
		}
	}
}


bool UBTTask_MoveAndUseSmartObject::StartInteraction()
{
	check(AIOwner);
	UWorld* World = AIOwner->GetWorld();
	USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(World);
	if (!ensure(SmartObjectSubsystem))
	{
		return false;
	}

	const UGameplayBehaviorSmartObjectBehaviorDefinition* SmartObjectGameplayBehaviorDefinition = SmartObjectSubsystem->MarkSlotAsOccupied<UGameplayBehaviorSmartObjectBehaviorDefinition>(ClaimedHandle);
	const UGameplayBehaviorConfig* GameplayBehaviorConfig = SmartObjectGameplayBehaviorDefinition != nullptr ? SmartObjectGameplayBehaviorDefinition->GameplayBehaviorConfig : nullptr;
	GameplayBehavior = GameplayBehaviorConfig != nullptr ? GameplayBehaviorConfig->GetBehavior(*World) : nullptr;
	if (GameplayBehavior == nullptr)
	{
		return false;
	}

	const USmartObjectComponent* SmartObjectComponent = SmartObjectSubsystem->GetSmartObjectComponent(ClaimedHandle);
	AActor& InteractorActor = *AIOwner->GetPawn();
	AActor* InteracteeActor = SmartObjectComponent ? SmartObjectComponent->GetOwner() : nullptr;
	const bool bBehaviorActive = UGameplayBehaviorSubsystem::TriggerBehavior(*GameplayBehavior, InteractorActor, GameplayBehaviorConfig, InteracteeActor);
	// Behavior can be successfully triggered AND ended synchronously. We are only interested to register callback when still running
	if (bBehaviorActive)
	{
		OnBehaviorFinishedNotifyHandle = GameplayBehavior->GetOnBehaviorFinishedDelegate().AddUObject(this, &UBTTask_MoveAndUseSmartObject::OnSmartObjectBehaviorFinished);

		// Operate the interface for Interactor/Interactee. You can remove the code according to your project
		if (InteractorActor.Implements<USmartObjectInteractorInterface>())
		{
			ISmartObjectInteractorInterface* InteractorInterface = Cast<ISmartObjectInteractorInterface>(&InteractorActor);
			InteractorInterface->OnInteractionBegin(ClaimedHandle, InteracteeActor);
		}
		if (InteracteeActor->Implements<USmartObjectInteracteeInterface>())
		{
			ISmartObjectInteracteeInterface* InteracteeInterface = Cast<ISmartObjectInteracteeInterface>(InteracteeActor);
			InteracteeInterface->OnSlotInteractionBegin(ClaimedHandle, &InteractorActor);
		}
	}

	return bBehaviorActive;
}

void UBTTask_MoveAndUseSmartObject::Abort()
{
	if (MoveToTask)
	{
		// clear before triggering 'the end' so that OnGameplayTaskDeactivated
		// ignores the incoming info about task end
		UAITask_MoveTo* Task = MoveToTask;
		MoveToTask = nullptr;
		Task->ExternalCancel();
	}
	else if (!bBehaviorFinished)
	{
		if (GameplayBehavior != nullptr)
		{
			check(AIOwner);
			check(AIOwner->GetPawn());
			GameplayBehavior->GetOnBehaviorFinishedDelegate().Remove(OnBehaviorFinishedNotifyHandle);
			GameplayBehavior->AbortBehavior(*AIOwner->GetPawn());
		}
	}

	FinishExecute(false);
}


void UBTTask_MoveAndUseSmartObject::OnSmartObjectBehaviorFinished(UGameplayBehavior& Behavior, AActor& Avatar, const bool bInterrupted)
{
	// Adding an ensure in case the assumptions change in the future.
	ensure(AIOwner != nullptr);

	// make sure we handle the right pawn - we can get this notify for a different
	// Avatar if the behavior sending it out is not instanced (CDO is being used to perform actions)
	if (AIOwner && AIOwner->GetPawn() == &Avatar)
	{
		Behavior.GetOnBehaviorFinishedDelegate().Remove(OnBehaviorFinishedNotifyHandle);
		bBehaviorFinished = true;
		FinishExecute(true);
	}
}

void UBTTask_MoveAndUseSmartObject::OnSlotInvalidated(const FSmartObjectClaimHandle& ClaimHandle, ESmartObjectSlotState State)
{
	Abort();
}

void UBTTask_MoveAndUseSmartObject::OnReceiveSmartObjectEvent(const FSmartObjectEventData& Event)
{
	if (Event.Reason == ESmartObjectChangeReason::OnSlotDisabled || Event.Reason == ESmartObjectChangeReason::OnObjectDisabled)
	{
		Abort();
	}
}


bool UBTTask_MoveAndUseSmartObject::GetGoalLocation(EGoalLocationTypeForMoveAndUseSmartObjectTask InGoalLocationType, const FSmartObjectClaimHandle& InClaimHandle, FVector& OutGoalLocation)
{
	if (InGoalLocationType == EntranceOrSlot)
	{
		return GetEntranceLocation(OutGoalLocation, InClaimHandle, EntranceRequest)
			|| GetSlotLocation(OutGoalLocation, InClaimHandle);
	}
	if (InGoalLocationType == OnlyEntrance)
	{
		return GetEntranceLocation(OutGoalLocation, InClaimHandle, EntranceRequest);
	}
	if (InGoalLocationType == OnlySlot)
	{
		return GetSlotLocation(OutGoalLocation, InClaimHandle);
	}

	return false;
}

bool UBTTask_MoveAndUseSmartObject::GetSlotLocation(FVector& OutLocation, const FSmartObjectClaimHandle& InClaimHandle) const
{
	if (const USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(GetWorld()); SmartObjectSubsystem != nullptr)
	{
		const TOptional<FTransform> GoalTransform = SmartObjectSubsystem->GetSlotTransform(InClaimHandle);
		if (GoalTransform.IsSet())
		{
			OutLocation = GoalTransform.GetValue().GetLocation();
			return true;
		}
	}

	return false;
}

bool UBTTask_MoveAndUseSmartObject::GetEntranceLocation(FVector& OutLocation, const FSmartObjectClaimHandle& InClaimHandle, FSmartObjectSlotEntranceLocationRequest& InEntranceRequest) const
{
	if (const USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(GetWorld()); SmartObjectSubsystem != nullptr)
	{
		FSmartObjectSlotEntranceLocationRequest& Request = InEntranceRequest;
		Request.UserActor = AIOwner->GetPawn();
		FSmartObjectSlotEntranceLocationResult Result;
		if (SmartObjectSubsystem->FindEntranceLocationForSlot(InClaimHandle.SlotHandle, Request, Result))
		{
			OutLocation = Result.Location;
			return true;
		}
	}

	return false;
}