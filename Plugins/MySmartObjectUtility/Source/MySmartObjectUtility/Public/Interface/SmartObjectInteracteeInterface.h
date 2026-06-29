// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SmartObjectRuntime.h"
#include "SmartObjectInteracteeInterface.generated.h"

class USmartObjectComponent;

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class USmartObjectInteracteeInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * ISmartObjectInteracteeInterface: Who can be interacted with
 *		- Implementation Example:
 *			- A SmartObject(Actor who contain USmartObjectComponent is regarded SmartObject)
 *
 *		- When to call SlotEvent(OnSlotInteractionBegin/OnSlotInteractionEnd):
 *			- After Triggered GameplayBehavior successfully and Ended GameplayBehavior successfully
 *
 */
class MYSMARTOBJECTUTILITY_API ISmartObjectInteracteeInterface
{
	GENERATED_BODY()

public:
	virtual void OnSlotInteractionBegin(const FSmartObjectClaimHandle ClaimHandle, AActor* Interactor) {}
	virtual void OnSlotInteractionEnd() {}

	virtual USmartObjectComponent* GetSmartObjectComponent() const = 0;
};
