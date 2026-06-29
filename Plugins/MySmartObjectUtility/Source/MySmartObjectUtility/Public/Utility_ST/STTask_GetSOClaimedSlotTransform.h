#pragma once

#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"
#include "Utility_BT/BTTask_GetClaimedSmartObjectSlotTransform.h"
#include "STTask_GetSOClaimedSlotTransform.generated.h"

UENUM(BlueprintType, DisplayName="GoalLocationType")
enum class EGoalLocationTypeForGetSOClaimedSlotTransformTask : uint8
{
	EntranceOrSlot,

	OnlyEntrance,

	OnlySlot,
};

USTRUCT()
struct FStateTreeGetSOClaimedSlotTransformInstanceData
{
	GENERATED_BODY()

	#pragma region Output

	UPROPERTY(EditAnywhere, Category = Out, meta = (RefType = "/Script/CoreUObject.Transform"))
	FStateTreePropertyRef GoalTransform;

	#pragma endregion

	#pragma region Input/Context

	UPROPERTY(EditAnywhere, Category = Input)
	FSmartObjectClaimHandle SOClaimHandle;

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AActor> QueryOwner = nullptr;

	#pragma endregion

	#pragma region Parameter

	UPROPERTY(EditAnywhere, Category = Parameter)
	FSmartObjectSlotEntranceLocationRequest EntranceRequest;

	UPROPERTY(EditAnywhere, Category = Parameter, DisplayName="Types of GoalTransform")
	EGoalLocationTypeForGetSOClaimedSlotTransformTask GoalLocationType = EGoalLocationTypeForGetSOClaimedSlotTransformTask::EntranceOrSlot;

	#pragma endregion
};

/**
 * Task for getting the transform of a claimed slot from SmartObject
 */
USTRUCT(meta = (DisplayName = "Get Slot Transform", Category = "AI|SmartObject"))
struct FSTTask_GetSOClaimedSlotTransform : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeGetSOClaimedSlotTransformInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	bool GetGoalLocation(FTransform& OutGoalTransform, FStateTreeExecutionContext& Context) const;
	bool GetSlotLocation(FTransform& OutTransform, FStateTreeExecutionContext& Context) const;
	bool GetEntranceLocation(FTransform& OutTransform, FStateTreeExecutionContext& Context, AActor* UserActor) const;


#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = Text) const override;
	virtual FName GetIconName() const override
	{
		return FName("StateTreeEditorStyle|Node.Task");
	}
	virtual FColor GetIconColor() const override
	{
		return UE::StateTree::Colors::Grey;
	}
#endif // WITH_EDITOR
};