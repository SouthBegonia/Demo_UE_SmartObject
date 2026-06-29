// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/NPCAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/StateTreeAIComponent.h"


// Sets default values
ANPCAIController::ANPCAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bStartAILogicOnPossess = false;

	StateTreeAIComp = CreateOptionalDefaultSubobject<UStateTreeAIComponent>("StateTreeAIComp");
	if (StateTreeAIComp)
		StateTreeAIComp->SetStartLogicAutomatically(false);
}

// Called when the game starts or when spawned
void ANPCAIController::BeginPlay()
{
	Super::BeginPlay();
}

void ANPCAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Run AILogic with BT, otherwise with ST
	bool bStartAILogicSuccess = false;
	if (!BehaviorTreeAsset.IsNull())
	{
		UBehaviorTree* BT;
		if (BehaviorTreeAsset.IsPending())
			BT = BehaviorTreeAsset.LoadSynchronous();
		else
			BT = BehaviorTreeAsset.Get();

		bStartAILogicSuccess = RunBehaviorTree(BT);
	}
	else if (StateTreeAIComp && !StateTreeAIComp->IsRunning())
	{
		StateTreeAIComp->StartLogic();
		bStartAILogicSuccess = true;
	}

	checkf(bStartAILogicSuccess, TEXT("Start any AI Logic (BT/ST) failed."));
}
