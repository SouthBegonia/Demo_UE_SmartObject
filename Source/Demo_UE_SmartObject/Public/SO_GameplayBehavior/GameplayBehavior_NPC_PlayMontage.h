// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayBehavior_AnimationBased.h"
#include "GameplayBehavior_NPC_PlayMontage.generated.h"

/**
 * A GameplayBehavior for playing target montages on Avatar.
 */
UCLASS()
class DEMO_UE_SMARTOBJECT_API UGameplayBehavior_NPC_PlayMontage : public UGameplayBehavior_AnimationBased
{
	GENERATED_BODY()

public:
	UGameplayBehavior_NPC_PlayMontage(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool Trigger(AActor& InAvatar, const UGameplayBehaviorConfig* Config = nullptr, AActor* SmartObjectOwner = nullptr) override;

	bool PlayMontageNew(AActor& InAvatar, UAnimMontage& AnimMontage, const float InPlayRate = 1.f, const FName StartSectionName = NAME_None, const bool bLoop = false);

protected:
	UFUNCTION()
	void OnMontageFinishedNew(UAnimMontage* Montage, bool bInterrupted, AActor* InAvatar);
};
