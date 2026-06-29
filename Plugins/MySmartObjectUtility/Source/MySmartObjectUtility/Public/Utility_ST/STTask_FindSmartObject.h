#pragma once

#include "SmartObjectTypes.h"
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "Utility_BT/BTTask_GetClaimedSmartObjectSlotTransform.h"
#include "STTask_FindSmartObject.generated.h"

USTRUCT()
struct FStateTreeFindSmartObjectInstanceData
{
	GENERATED_BODY()

	#pragma region Output

	//UPROPERTY(EditAnywhere, Category = Output)
	FSmartObjectClaimHandle SOClaimHandle;

	UPROPERTY(EditAnywhere, Category = Out, meta = (RefType = "/Script/SmartObjectsModule.SmartObjectClaimHandle"))
	FStateTreePropertyRef SOClaimHandleResult;

	#pragma endregion

	#pragma region Input/Context

	// The query will be run with this actor has the owner object.
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AActor> QueryOwner = nullptr;

	#pragma endregion

	#pragma region Parameter

	UPROPERTY(EditAnywhere, Category = Parameter)
	FGameplayTagQuery ActivityRequirements;


	UPROPERTY(EditAnywhere, Category = Parameter)
	ESmartObjectClaimPriority ClaimPriority = ESmartObjectClaimPriority::Normal;

	/** Used for smart object querying if QueryTemplate is not configured */
	UPROPERTY(EditAnywhere, Category = Parameter , meta=(DisplayName = "QueryBoxRadius", UIMin = "0.0", EditCondition = "QueryTemplate == nullptr"))
	float Radius;


	// The query template to run
	UPROPERTY(EditAnywhere, Category = Parameter, meta=(EditCondition = "Radius == 0.0"))
	TObjectPtr<UEnvQuery> QueryTemplate;

	// Query config associated with the query template.
	UPROPERTY(EditAnywhere, EditFixedSize, Category = Parameter, meta=(EditCondition = "QueryTemplate != nullptr", EditConditionHides))
	TArray<FAIDynamicParam> QueryConfig;

	/** determines which item will be stored (All = only first matching) */
	UPROPERTY(EditAnywhere, Category = Parameter, meta=(EditCondition = "QueryTemplate != nullptr", EditConditionHides))
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = EEnvQueryRunMode::SingleResult;

	#pragma endregion


	TSharedPtr<FEnvQueryResult> QueryResult = nullptr;

	int32 RequestId = INDEX_NONE;
};


/**
 * Task for find and claim slot from SmartObject
 */
USTRUCT(meta = (DisplayName = "Find SmartObject", Category = "AI|SmartObject"))
struct FSTTask_FindSmartObject : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeFindSmartObjectInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = Text) const override;
	virtual FName GetIconName() const override
	{
		return FName("StateTreeEditorStyle|Node.Find");
	}
	virtual FColor GetIconColor() const override
	{
		return UE::StateTree::Colors::Grey;
	}
#endif // WITH_EDITOR
};
