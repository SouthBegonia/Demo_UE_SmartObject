// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Interface/SmartObjectInteractorInterface.h"
#include "NPCCharacter.generated.h"

class UMotionWarpingComponent;
class USmartObjectUserComponent;


/**
 * NPCCharacter: A type of Character that includes the ability to interact with SmartObjects
 *		- feature:
 *			- Listening event of interacting(OnInteractionBegin/OnInteractionEnd). There is no way to trigger SO interaction actively because this is the job of BehaviorTree/StateTree.
 *			- Storing SOClaimHandle data as a backup. So that you can get it in BT/ST easily
 *
 */
UCLASS(BlueprintType, meta=(ShortTooltip="A type of Character that includes the ability to interact with SmartObjects."))
class MYSMARTOBJECTUTILITY_API ANPCCharacter : public ACharacter, public IAbilitySystemInterface, public ISmartObjectInteractorInterface
{
	GENERATED_BODY()

public:
	ANPCCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return ASC; }

	/* mainly use for PlayMontage (Anim can be replicated, or you could implement by RPC */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USmartObjectUserComponent> SOUerComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMotionWarpingComponent> MotionWarpComp;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName DefaultSOClaimHandleBBKeyName = FName(TEXT("SOClaimHandle"));

	#pragma region ISmartObjectInteractorInterface

	virtual void OnInteractionBegin(const FSmartObjectClaimHandle ClaimHandle, AActor* InteracteeActor) override;
	virtual void OnInteractionEnd() override;

	virtual void SetSOClaimHandle(const FSmartObjectClaimHandle ClaimHandle, const FName& ClaimHandleBlackboardKeyName = FName()) override;
	virtual FSmartObjectClaimHandle GetSOClaimHandle(const FName& ClaimHandleBlackboardKeyName = FName()) const override;
	virtual void InvalidateSOClaimHandle(const FName& ClaimHandleBlackboardKeyName = FName()) override;

	#pragma endregion

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName="OnInteractionBegin"))
	void K2_OnInteractionBegin(const FSmartObjectClaimHandle ClaimHandle, AActor* InteracteeActor);
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName="OnInteractionEnd"))
	void K2_OnInteractionEnd();

private:
	UPROPERTY()
	FSmartObjectClaimHandle SOClaimHandle;
};
