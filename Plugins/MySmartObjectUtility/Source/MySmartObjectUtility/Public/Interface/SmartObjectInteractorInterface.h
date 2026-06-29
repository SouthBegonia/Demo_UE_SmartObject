// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SmartObjectRuntime.h"
#include "SmartObjectInteractorInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class USmartObjectInteractorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * ISmartObjectInteractorInterface: Who can interact with others
 *		- Implementation Example:
 *			- NPC(Character who can run AI Logic with BehaviorTree/StateTree)
 *
 *		- When to call InteractionEvent(OnInteractionBegin/OnInteractionEnd):
 *			- After Triggered GameplayBehavior successfully and Ended GameplayBehavior successfully
 *
 *		- When to Save/Reset SOClaimHandle(SetSOClaimHandle/InvalidateSOClaimHandle):
 *			- After Found and claimed SO successfully, then reset it after interaction completed(whether successful or failed)
 *
 */
class MYSMARTOBJECTUTILITY_API ISmartObjectInteractorInterface
{
	GENERATED_BODY()

public:
	virtual void OnInteractionBegin(const FSmartObjectClaimHandle ClaimHandle, AActor* InteracteeActor) {}
	virtual void OnInteractionEnd() {}

	virtual void SetSOClaimHandle(const FSmartObjectClaimHandle ClaimHandle, const FName& ClaimHandleBlackboardKeyName = FName()) = 0;
	virtual FSmartObjectClaimHandle GetSOClaimHandle(const FName& ClaimHandleBlackboardKeyName = FName()) const = 0;
	virtual void InvalidateSOClaimHandle(const FName& ClaimHandleBlackboardKeyName = FName()) = 0;
};
