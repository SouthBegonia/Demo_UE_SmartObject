// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayBehavior.h"
#include "SmartObjectTypes.h"
#include "GameplayBehavior_NPC_PlayMontage.generated.h"

class AActor;
class UAnimMontage;
class UAbilitySystemComponent;

USTRUCT()
struct FNPCMontagePlaybackData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> Avatar;

	UPROPERTY()
	TObjectPtr<UAnimMontage> AnimMontage;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilityComponent;

	float PlayRate;
	FName SectionName;
	uint8 bLoop : 1;


	// TimerHandle for Montage PlaybackLength
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate;

	/* TimerHandle for UGameplayBehaviorConfig_NPC_PlayMontage.PlayTime (not for Montage PlaybackLength) */
	FTimerHandle PlaySuspendTimerHandle;
	FTimerDelegate PlaySuspendTimerDelegate;

	FNPCMontagePlaybackData()
	{
		FMemory::Memzero(*this);
	}

	FNPCMontagePlaybackData(AActor& InAvatar, UAnimMontage& InAnimMontage, const float InPlayRate = 1.f, const FName InSectionName = NAME_None, const bool bInLoop = false)
		: Avatar(&InAvatar), AnimMontage(&InAnimMontage)
		, AbilityComponent(nullptr), PlayRate(InPlayRate)
		, SectionName(InSectionName), bLoop(bInLoop)
	{}

	bool operator==(const FNPCMontagePlaybackData& Other) const
	{
		return Avatar == Other.Avatar && AnimMontage == Other.AnimMontage;
	}
	bool operator==(const AActor* InAvatar) const
	{
		return Avatar == InAvatar;
	}
};

/**
 * A GameplayBehavior for playing target montages on Avatar.
 *
 * Note: Similar to @UGameplayBehavior_AnimationBased
 */
UCLASS()
class DEMO_UE_SMARTOBJECT_API UGameplayBehavior_NPC_PlayMontage : public UGameplayBehavior//UGameplayBehavior_AnimationBased
{
	GENERATED_BODY()

public:
	UGameplayBehavior_NPC_PlayMontage(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool Trigger(AActor& InAvatar, const UGameplayBehaviorConfig* Config = nullptr, AActor* SmartObjectOwner = nullptr) override;
	virtual void EndBehavior(AActor& Avatar, const bool bInterrupted = false) override;

	bool PlayMontage(AActor& InAvatar, FNPCMontagePlaybackData& PlaybackData);

protected:

	UFUNCTION()
	void OnMontageFinished(UAnimMontage* Montage, bool bInterrupted, AActor* InAvatar);
	UFUNCTION()
	void OnPlayTimerFinished(UAnimMontage* Montage, AActor* InAvatar);

	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay="SlotHandle"))
	void TeleportAvatarToSlot(AActor* Avatar, FSmartObjectSlotHandle SlotHandle = FSmartObjectSlotHandle());

	UFUNCTION(BlueprintCallable, meta=(AdvancedDisplay="SlotHandle"))
	void AddOrUpdateWarpTargetToSlot(AActor* Avatar, FName SlotMotionWarpingName, FSmartObjectSlotHandle SlotHandle = FSmartObjectSlotHandle());

	/*
	 * Record all the runtime data from this behavior being triggered
	 *
	 * Note: Implementation by NPCActivePlaybackList because the Behavior was DontInstantiate (NPCActivePlaybackList will be shared by all behaviors)
	 *		 You can also change it to Instantiate, so that each behavior will own the unique data, but it may increase memory consumption
	 */
	UPROPERTY()
	mutable TArray<FNPCMontagePlaybackData> NPCActivePlaybackList;

private:
	bool GetSlotTransform(FTransform& ResultSloTransform, AActor* Avatar, FSmartObjectSlotHandle SlotHandle = FSmartObjectSlotHandle()) const;
};
