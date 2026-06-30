// Fill out your copyright notice in the Description page of Project Settings.


#include "Demo_UE_SmartObject/Public/SO_GameplayBehavior/GameplayBehavior_NPC_PlayMontage.h"
#include "Demo_UE_SmartObject/Public/SO_GameplayBehavior/GameplayBehaviorConfig_NPC_PlayMontage.h"

UGameplayBehavior_NPC_PlayMontage::UGameplayBehavior_NPC_PlayMontage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UGameplayBehavior_NPC_PlayMontage::Trigger(AActor& InAvatar, const UGameplayBehaviorConfig* Config, AActor* SmartObjectOwner)
{
	const UGameplayBehaviorConfig_NPC_PlayMontage* AnimConfig = Cast<const UGameplayBehaviorConfig_NPC_PlayMontage>(Config);
	UAnimMontage* Anim = AnimConfig != nullptr ? AnimConfig->GetMontage() : nullptr;
	if (AnimConfig == nullptr || Anim == nullptr)
	{
		UE_VLOG(&InAvatar, LogGameplayBehavior, Log, TEXT("Failed to trigger behavior %s due to %s being null")
			, *InAvatar.GetName(), AnimConfig ? TEXT("Config->Montage") : TEXT("Config"));
		return false;
	}

	return PlayMontage(InAvatar, *Anim, AnimConfig->GetPlayRate(), AnimConfig->GetStartSectionName(), AnimConfig->IsLooped());
}
