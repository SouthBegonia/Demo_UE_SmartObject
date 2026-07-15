// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayBehaviorConfig_Animation.h"
#include "GameplayBehaviorConfig_NPC_PlayMontage.generated.h"

/**
 * A GameplayBehaviorConfig for defining the parameter of montages
 */
UCLASS()
class DEMO_UE_SMARTOBJECT_API UGameplayBehaviorConfig_NPC_PlayMontage : public UGameplayBehaviorConfig
{
	GENERATED_BODY()

public:
	UGameplayBehaviorConfig_NPC_PlayMontage(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Depending on the specific UGameplayBehavior class returns an instance or CDO of BehaviorClass. */
	//virtual UGameplayBehavior* GetBehavior(UWorld& World) const;

	UAnimMontage* GetMontage() const;
	float GetPlayRate() const { return PlayRate; }
	FName GetStartSectionName() const { return StartSectionName; }
	bool IsLooped() const { return (bLoop != 0); }
	FName GetSlotMotionWarpingName() const { return SlotMotionWarpingName; }
	float GetPlayTimeWithingRandomDeviation() const;


protected:
	UPROPERTY(EditAnywhere, Category = SmartObject, meta=(ShortTooltip = "The single montage will be played"))
	mutable TSoftObjectPtr<UAnimMontage> AnimMontage;

	UPROPERTY(EditAnywhere, Category = SmartObject, meta=(ShortTooltip = "A random montage from list will be played"))
	mutable TArray<TSoftObjectPtr<UAnimMontage>> RandomAnimMontageList;

	UPROPERTY(EditAnywhere, Category = SmartObject)
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, Category = SmartObject)
	FName StartSectionName;

	UPROPERTY(EditAnywhere, Category = SmartObject, meta=(ToolTip = "If true, the montage will be played in loop until the behavior ended"))
	uint32 bLoop : 1;

	UPROPERTY(EditAnywhere, Category = SmartObject, meta=(UIMin = "0.0", ToolTip = "Montage will be played only once if PlatTime is 0.f, otherwise montage will stop after PlatTime seconds.\nUsually recommended for Looping montage (bLoop = true)"))
	float PlayTime = 0.f;

	UPROPERTY(EditAnywhere, Category = SmartObject, meta = (EditCondition = "PlayTime > 0.0", EditConditionHides, ToolTip = "Random deviation in seconds to add to PlayTime. The final PlayTime will be in the range [PlayTime - PlayTimeRandomDeviation, PlayTime + PlayTimeRandomDeviation]"))
	float PlayTimeRandomDeviation = 0.f;

	UPROPERTY(EditAnywhere, Category = SmartObject, meta=(ToolTip = "Warping Avatar to SlotTransform.\nRemember to add a motion warpping window for target montage."))
	FName SlotMotionWarpingName = TEXT("SmartObjectWarp");

	UPROPERTY()
	mutable TObjectPtr<UAnimMontage> TargetAnimMontage;
};
