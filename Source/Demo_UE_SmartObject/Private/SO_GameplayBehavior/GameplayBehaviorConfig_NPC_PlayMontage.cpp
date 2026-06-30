// Fill out your copyright notice in the Description page of Project Settings.


#include "Demo_UE_SmartObject/Public/SO_GameplayBehavior/GameplayBehaviorConfig_NPC_PlayMontage.h"
#include "Demo_UE_SmartObject/Public/SO_GameplayBehavior/GameplayBehavior_NPC_PlayMontage.h"
#include "Kismet/KismetMathLibrary.h"


UGameplayBehaviorConfig_NPC_PlayMontage::UGameplayBehaviorConfig_NPC_PlayMontage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BehaviorClass = UGameplayBehavior_NPC_PlayMontage::StaticClass();
}

UAnimMontage* UGameplayBehaviorConfig_NPC_PlayMontage::GetMontage() const
{
	if (TargetAnimMontage != nullptr)
		return TargetAnimMontage;

	if (!AnimMontage.IsNull())
	{
		if (AnimMontage.IsPending())
			TargetAnimMontage = AnimMontage.LoadSynchronous();
		else
			TargetAnimMontage = AnimMontage.Get();
	}
	else if (RandomAnimMontageList.Num() > 0)
	{
		TSoftObjectPtr<UAnimMontage> RandomAnimMontage = RandomAnimMontageList[UKismetMathLibrary::RandomIntegerInRange(0, RandomAnimMontageList.Num() - 1)];
		if (!RandomAnimMontage.IsNull())
		{
			if (RandomAnimMontage.IsPending())
				TargetAnimMontage = RandomAnimMontage.LoadSynchronous();
			else
				TargetAnimMontage = RandomAnimMontage.Get();
		}
	}

	check(TargetAnimMontage)
	return TargetAnimMontage;
}

