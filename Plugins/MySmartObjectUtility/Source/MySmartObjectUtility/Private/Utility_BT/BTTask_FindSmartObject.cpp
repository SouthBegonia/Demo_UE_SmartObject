// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility_BT/BTTask_FindSmartObject.h"
#include "AIController.h"
#include "BlackboardKeyType_SOClaimHandle.h"
#include "GameplayTagAssetInterface.h"
#include "GameplayBehaviorSmartObjectBehaviorDefinition.h"
#include "SmartObjectSubsystem.h"
#include "VisualLogger/VisualLogger.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvQueryItemType_SmartObject.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Interface/SmartObjectInteractorInterface.h"


UBTTask_FindSmartObject::UBTTask_FindSmartObject()
{
	Radius = 500.f;
	EQSQueryFinishedDelegate = FQueryFinishedSignature::CreateUObject(this, &UBTTask_FindSmartObject::OnQueryFinished);
	EQSRequest.RunMode = EEnvQueryRunMode::AllMatching;
	bNotifyTaskFinished = true;
}

void UBTTask_FindSmartObject::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FBTQuerySOMemory>(NodeMemory, InitType);
}

void UBTTask_FindSmartObject::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<FBTQuerySOMemory>(NodeMemory, CleanupType);
}

EBTNodeResult::Type UBTTask_FindSmartObject::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type NodeResult = EBTNodeResult::Failed;

	UWorld* World = GetWorld();
	USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(World);
	AAIController* MyController = OwnerComp.GetAIOwner();
	if (SmartObjectSubsystem == nullptr || MyController == nullptr
		|| MyController->GetPawn() == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	FBTQuerySOMemory* MyMemory = reinterpret_cast<FBTQuerySOMemory*>(NodeMemory);
	MyMemory->EQSRequestID = INDEX_NONE;

	AActor& Avatar = *MyController->GetPawn();

	if (EQSRequest.IsValid() && (EQSRequest.EQSQueryBlackboardKey.IsSet() || EQSRequest.QueryTemplate))
	{
		const UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
		MyMemory->EQSRequestID = EQSRequest.Execute(Avatar, BlackboardComponent, EQSQueryFinishedDelegate);

		if (MyMemory->EQSRequestID != INDEX_NONE)
		{
			NodeResult = EBTNodeResult::InProgress;
		}
	}
	else 
	{
		const FVector UserLocation = Avatar.GetActorLocation();

		// Create filter
		FSmartObjectRequestFilter Filter;
		Filter.ActivityRequirements = ActivityRequirements;
		Filter.BehaviorDefinitionClasses = { UGameplayBehaviorSmartObjectBehaviorDefinition::StaticClass() };
		const IGameplayTagAssetInterface* TagsSource = Cast<const IGameplayTagAssetInterface>(&Avatar);
		if (TagsSource != nullptr)
		{
			TagsSource->GetOwnedGameplayTags(Filter.UserTags);
		}

		// Create request
		FSmartObjectRequest Request(FBox(UserLocation, UserLocation).ExpandBy(FVector(Radius), FVector(Radius)), Filter);
		TArray<FSmartObjectRequestResult> Results;
		const FSmartObjectActorUserData ActorUserData(&Avatar);
		const FConstStructView ActorUserDataView(FConstStructView::Make(ActorUserData));
	
		if (SmartObjectSubsystem->FindSmartObjects(Request, Results, ActorUserDataView))
		{
			for (const FSmartObjectRequestResult& Result : Results)
			{
				FSmartObjectClaimHandle ClaimHandle = SmartObjectSubsystem->MarkSlotAsClaimed(Result.SlotHandle, ClaimPriority, ActorUserDataView);
				if (ClaimHandle.IsValid())
				{
					OwnerComp.GetBlackboardComponent()->SetValue<UBlackboardKeyType_SOClaimHandle>(SOClaimHandleKey.SelectedKeyName, ClaimHandle);

					if (Avatar.Implements<USmartObjectInteractorInterface>())
						Cast<ISmartObjectInteractorInterface>(&Avatar)->SetSOClaimHandle(ClaimHandle, SOClaimHandleKey.SelectedKeyName);

					NodeResult = EBTNodeResult::Succeeded;
					break;
				}
			}

			UE_CVLOG_UELOG(NodeResult == EBTNodeResult::Failed, MyController, LogSmartObject, Warning, TEXT("%s failed to claim smart object"), *GetNodeName());
		}
		else
		{
			UE_VLOG_UELOG(MyController, LogSmartObject
				, Verbose, TEXT("%s failed to find smart objects for request: %s")
				, *GetNodeName(), *Avatar.GetName());
		}
	}

	return NodeResult;
}

EBTNodeResult::Type UBTTask_FindSmartObject::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	check(NodeMemory);

	FBTQuerySOMemory* MyMemory = reinterpret_cast<FBTQuerySOMemory*>(NodeMemory);

	if (MyMemory->EQSRequestID != INDEX_NONE)
	{
		if (UWorld* World = OwnerComp.GetWorld())
		{
			if (UEnvQueryManager* EnvQueryManager = UEnvQueryManager::GetCurrent(World))
			{
				EnvQueryManager->AbortQuery(MyMemory->EQSRequestID);
			}
		}
		MyMemory->EQSRequestID = INDEX_NONE;
	}

	return EBTNodeResult::Aborted;
}

void UBTTask_FindSmartObject::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	FBTQuerySOMemory* MyMemory = reinterpret_cast<FBTQuerySOMemory*>(NodeMemory);
	check(MyMemory->EQSRequestID == INDEX_NONE);
}

FString UBTTask_FindSmartObject::GetStaticDescription() const
{
	FString Result;
	if (SOClaimHandleKey.SelectedKeyType != UBlackboardKeyType_SOClaimHandle::StaticClass())
	{
		Result += FString::Printf(TEXT("SOClaimHandleKey must be type of SOClaimHandle"));
	}

	if (ActivityRequirements.IsEmpty() == false)
	{
		Result += FString::Printf(TEXT("Object requirements: %s")
			, *ActivityRequirements.GetDescription());
	}

	if (Result.Len() > 0)
		return Result;

	FString KeyDesc = SOClaimHandleKey.SelectedKeyName.ToString();

	return FString::Printf(TEXT("Result ClaimedHandle in BBKey : %s"), *KeyDesc);
}

FName UBTTask_FindSmartObject::GetNodeIconName() const
{
	return Super::GetNodeIconName();
}

void UBTTask_FindSmartObject::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	EQSRequest.InitForOwnerAndBlackboard(*this, GetBlackboardAsset());
}

void UBTTask_FindSmartObject::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	if (!Result)
	{
		return;
	}

	AActor* MyOwner = Cast<AActor>(Result->Owner.Get());
	if (APawn* PawnOwner = Cast<APawn>(MyOwner))
	{
		MyOwner = PawnOwner->GetController();
	}

	UBehaviorTreeComponent* BTComponent = MyOwner ? MyOwner->FindComponentByClass<UBehaviorTreeComponent>() : NULL;
	if (!BTComponent)
	{
		UE_LOG(LogBehaviorTree, Warning, TEXT("%s [%s]: Unable to find behavior tree to notify about finished query!")
			, ANSI_TO_TCHAR(__FUNCTION__), *GetNameSafe(MyOwner));
		return;
	}

	const FEnvQueryResult& QueryResult = *Result.Get();

	uint8* RawMemory = BTComponent->GetNodeMemory(this, BTComponent->FindInstanceContainingNode(this));
	check(RawMemory);
	FBTQuerySOMemory* MyMemory = reinterpret_cast<FBTQuerySOMemory*>(RawMemory);
	if (MyMemory->EQSRequestID != QueryResult.QueryID)
	{
		UE_VLOG_UELOG(BTComponent, LogBehaviorTree, Log, TEXT("%s [%s] ignoring EQS result due to QueryID mismatch.")
			, ANSI_TO_TCHAR(__FUNCTION__), *GetNameSafe(MyOwner));

		check(MyMemory->EQSRequestID != INDEX_NONE)

		return;
	}
	else if (QueryResult.IsAborted())
	{
		UE_VLOG_UELOG(BTComponent, LogBehaviorTree, Log, TEXT("%s [%s] observed EQS query finished as Aborted. Aborting the BT node as well.")
			, ANSI_TO_TCHAR(__FUNCTION__), *GetNameSafe(MyOwner));

		FinishLatentTask(*BTComponent, EBTNodeResult::Aborted);
		return;
	}

	// at this point we've already confirmed that QueryResult does indeed corresponds to the the query we're waiting for
	// so we need to clear the EQSRequestID here in case the next task we're about to issue (the UAITask_UseGameplayBehaviorSmartObject)
	// might fail instantly and we do check EQSRequestID on tasks end to make sure everything has been cleaned up properly.
	MyMemory->EQSRequestID = INDEX_NONE;

	bool bSmartObjectClaimed = false;

	if (QueryResult.IsSuccessful() && (QueryResult.Items.Num() >= 1))
	{
		if (QueryResult.ItemType->IsChildOf(UEnvQueryItemType_SmartObject::StaticClass()) == false)
		{
			UE_VLOG_UELOG(BTComponent, LogSmartObject, Error, TEXT("%s used EQS query that did not generate EnvQueryItemType_SmartObject items"), *GetNodeName());
		}
		else if (USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(MyOwner->GetWorld()))
		{
			const FSmartObjectActorUserData ActorUserData(Cast<AActor>(Result->Owner.Get()));
			const FConstStructView ActorUserDataView(FConstStructView::Make(ActorUserData));

			// we could use QueryResult.GetItemAsTypeChecked, but the below implementation is more efficient
			for (int i = 0; i < QueryResult.Items.Num(); ++i)
			{
				const FSmartObjectSlotEQSItem& Item = UEnvQueryItemType_SmartObject::GetValue(QueryResult.GetItemRawMemory(i));
				const FSmartObjectClaimHandle ClaimHandle = SmartObjectSubsystem->MarkSlotAsClaimed(Item.SlotHandle, ClaimPriority, ActorUserDataView);
				if (ClaimHandle.IsValid())
				{
					BTComponent->GetBlackboardComponent()->SetValue<UBlackboardKeyType_SOClaimHandle>(SOClaimHandleKey.SelectedKeyName, ClaimHandle);
					bSmartObjectClaimed = true;

					AActor* Avatar = Cast<AActor>(Result->Owner.Get());
					if (Avatar && Avatar->Implements<USmartObjectInteractorInterface>())
						Cast<ISmartObjectInteractorInterface>(Avatar)->SetSOClaimHandle(ClaimHandle, SOClaimHandleKey.SelectedKeyName);


					UE_VLOG_UELOG(BTComponent, LogSmartObject, Verbose, TEXT("%s claimed EQS-found smart object: %s"), *GetNodeName(), *LexToString(ClaimHandle));

					/*
					FSmartObjectSlotEntranceLocationRequest LocationRequest;
					LocationRequest.LocationType = ESmartObjectSlotNavigationLocationType::Entry;
					//LocationRequest.UserActor = Cast<AActor>(Result->Owner.Get());
					LocationRequest.ValidationFilter = USmartObjectSlotValidationFilter::StaticClass();
					FSmartObjectSlotEntranceLocationResult LocationResult;
					bool result = SmartObjectSubsystem->FindEntranceLocationForSlot(ClaimHandle.SlotHandle, LocationRequest, LocationResult);
					*/

					break;
				}
			}
		}
	}

	FinishLatentTask(*BTComponent, bSmartObjectClaimed ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
}