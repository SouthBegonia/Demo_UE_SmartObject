// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayBehaviorConfig_Animation.h"
#include "GameplayBehaviorConfig_NPC_PlayMontage.generated.h"

/**
 * 
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

protected:
	UPROPERTY(EditAnywhere, Category = SmartObject)
	mutable TSoftObjectPtr<UAnimMontage> AnimMontage;

	UPROPERTY(EditAnywhere, Category = SmartObject)
	mutable TArray<TSoftObjectPtr<UAnimMontage>> RandomAnimMontageList;

	UPROPERTY()
	mutable TObjectPtr<UAnimMontage> TargetAnimMontage;

	UPROPERTY(EditAnywhere, Category = SmartObject)
	float PlayRate = 1.f;

	UPROPERTY(EditAnywhere, Category = SmartObject)
	FName StartSectionName;

	UPROPERTY(EditAnywhere, Category = SmartObject)
	uint32 bLoop : 1;
};
