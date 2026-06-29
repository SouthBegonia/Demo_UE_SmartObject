// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SmartObjectSubsystem.h"
#include "SmartObjectTypes.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "BTTask_FindSmartObject.generated.h"

struct FSmartObjectClaimHandle;

struct FBTQuerySOMemory
{
	int32 EQSRequestID;
};

/**
 * Task for find and claim slot from SmartObject
 */
UCLASS()
class MYSMARTOBJECTUTILITY_API UBTTask_FindSmartObject : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindSmartObject();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTQuerySOMemory); }
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;

	virtual FString GetStaticDescription() const override;
	virtual FName GetNodeIconName() const override;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	void OnQueryFinished(TSharedPtr<FEnvQueryResult> Result);
protected:

	UPROPERTY(EditAnywhere, Category = SmartObjects)
	FBlackboardKeySelector SOClaimHandleKey;

	/** Additional tag query to filter available smart objects. We'll query for smart
	 *	objects that support activities tagged in a way matching the filter.
	 *	Note that regular tag-base filtering is going to take place as well */
	UPROPERTY(EditAnywhere, Category = SmartObjects)
	FGameplayTagQuery ActivityRequirements;

	UPROPERTY(EditAnywhere, Category = SmartObjects)
	ESmartObjectClaimPriority ClaimPriority = ESmartObjectClaimPriority::Normal;

	/** Used for smart object querying if EQSRequest is not configured */
	UPROPERTY(EditAnywhere, Category = SmartObjects, meta=(DisplayName="QueryBoxRadius", UIMin = "0.0", EditCondition = "EQSRequest.QueryTemplate == nullptr"))
	float Radius;


	UPROPERTY(EditAnywhere, Category = SmartObjects, meta=(EditCondition = "Radius == 0.0"))
	FEQSParametrizedQueryExecutionRequest EQSRequest;




	FQueryFinishedSignature EQSQueryFinishedDelegate;
};
