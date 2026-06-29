#include "Utility_ST/STTask_FindSmartObject.h"
#include "BlackboardKeyType_SOClaimHandle.h"
#include "EnvQueryItemType_SmartObject.h"
#include "GameplayBehaviorSmartObjectBehaviorDefinition.h"
#include "GameplayTagAssetInterface.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"

#define LOCTEXT_NAMESPACE "GameplayStateTree"

EStateTreeRunStatus FSTTask_FindSmartObject::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	EStateTreeRunStatus NodeResult = EStateTreeRunStatus::Failed;


	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	check(InstanceData.QueryOwner)

	USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(Context.GetWorld());
	if (SmartObjectSubsystem == nullptr)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.QueryTemplate)
	{
		FEnvQueryRequest Request(InstanceData.QueryTemplate, InstanceData.QueryOwner);

		for (FAIDynamicParam& DynamicParam : InstanceData.QueryConfig)
		{
			Request.SetDynamicParam(DynamicParam, nullptr);
		}

		InstanceData.RequestId = Request.Execute(InstanceData.RunMode,
	FQueryFinishedSignature::CreateLambda([InstanceDataRef = Context.GetInstanceDataStructRef(*this)](TSharedPtr<FEnvQueryResult> QueryResult) mutable
		{
			if (FInstanceDataType* InstanceData = InstanceDataRef.GetPtr())
			{
				InstanceData->QueryResult = QueryResult;
				InstanceData->RequestId = INDEX_NONE;
			}
		}));

		if (InstanceData.RequestId != INDEX_NONE)
			NodeResult = EStateTreeRunStatus::Running;
	}else
	{
		AActor& Avatar = *InstanceData.QueryOwner;
		float Radius = InstanceData.Radius;
		const FVector UserLocation = Avatar.GetActorLocation();

		// Create filter
		FSmartObjectRequestFilter Filter;
		Filter.ActivityRequirements = InstanceData.ActivityRequirements;
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
				FSmartObjectClaimHandle ClaimHandle = SmartObjectSubsystem->MarkSlotAsClaimed(Result.SlotHandle, InstanceData.ClaimPriority, ActorUserDataView);
				if (ClaimHandle.IsValid())
				{
					InstanceData.SOClaimHandle = ClaimHandle;

					auto ClaimHandlePtr = InstanceData.SOClaimHandleResult.GetMutablePtr<FSmartObjectClaimHandle>(Context);
					if (ClaimHandlePtr)
						*ClaimHandlePtr = ClaimHandle;

					NodeResult = EStateTreeRunStatus::Succeeded;
					break;
				}
			}
		}
	}

	return NodeResult;
}

EStateTreeRunStatus FSTTask_FindSmartObject::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	EStateTreeRunStatus RunStatus = EStateTreeRunStatus::Running;

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.QueryResult)
	{
		if (InstanceData.QueryResult->IsSuccessful() && InstanceData.QueryResult->Items.Num() >= 1)
		{
			if (InstanceData.QueryResult->ItemType->IsChildOf(UEnvQueryItemType_SmartObject::StaticClass()) == false)
			{
				UE_VLOG(Context.GetOwner(), LogStateTree, Error, TEXT("FSTTask_FindSmartObject failed since EQS query did not generate EnvQueryItemType_SmartObject items."));
				RunStatus = EStateTreeRunStatus::Failed;
			}
			else if (USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(Context.GetWorld()))
			{
				const FSmartObjectActorUserData ActorUserData(Cast<AActor>(InstanceData.QueryOwner));
				const FConstStructView ActorUserDataView(FConstStructView::Make(ActorUserData));

				// we could use QueryResult.GetItemAsTypeChecked, but the below implementation is more efficient
				for (int i = 0; i < InstanceData.QueryResult->Items.Num(); ++i)
				{
					const FSmartObjectSlotEQSItem& Item = UEnvQueryItemType_SmartObject::GetValue(InstanceData.QueryResult->GetItemRawMemory(i));
					const FSmartObjectClaimHandle ClaimHandle = SmartObjectSubsystem->MarkSlotAsClaimed(Item.SlotHandle, InstanceData.ClaimPriority, ActorUserDataView);
					if (ClaimHandle.IsValid())
					{
						InstanceData.SOClaimHandle = ClaimHandle;

						auto ClaimHandlePtr = InstanceData.SOClaimHandleResult.GetMutablePtr<FSmartObjectClaimHandle>(Context);
						if (ClaimHandlePtr)
							*ClaimHandlePtr = ClaimHandle;

						// TODO : set ClaimHandle as BlackboardValue
						/*if (UBlackboardComponent* BB = InstanceData.AIController->GetBlackboardComponent())
							BB->SetValue<UBlackboardKeyType_SOClaimHandle>(FName(TEXT("SOClaimHandle")), ClaimHandle);*/

						RunStatus = EStateTreeRunStatus::Succeeded;
						break;
					}
				}
			}
		}
		else
		{
			// Query failed or QueryResult is empty
			RunStatus = EStateTreeRunStatus::Failed;
		}
	}

	return RunStatus;
}

#if WITH_EDITOR

FText FSTTask_FindSmartObject::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText(LOCTEXT("FindSmartObject", "Find SmartObject"));
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
