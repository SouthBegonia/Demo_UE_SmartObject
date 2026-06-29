// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NPCAIController.generated.h"

/**
 * NPCAIController: A type of AIController that includes the ability to run AI Logic (StateTree or BehaviorTree)
 *		- feature:
 *			- Run AI Logic by BehaviorTree or StateTree.
 *
 */
UCLASS(BlueprintType, Blueprintable, meta=(ShortTooltip="A type of AIController that includes the ability to run AI Logic (StateTree or BehaviorTree)."))
class MYSMARTOBJECTUTILITY_API ANPCAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANPCAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	/* StateTree. You must choose one from StateTree and BehaviorTree to run AI logic.  */
	UPROPERTY(EditDefaultsOnly, Category = AI)
	TObjectPtr<class UStateTreeAIComponent> StateTreeAIComp;

	/* BehaviorTree. You must choose one from StateTree and BehaviorTree to run AI logic.  */
	UPROPERTY(EditDefaultsOnly, Category = AI)
	TSoftObjectPtr<UBehaviorTree> BehaviorTreeAsset;
};
