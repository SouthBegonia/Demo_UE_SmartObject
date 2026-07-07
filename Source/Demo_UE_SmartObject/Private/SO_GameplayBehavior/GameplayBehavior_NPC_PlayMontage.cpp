// Fill out your copyright notice in the Description page of Project Settings.


#include "Demo_UE_SmartObject/Public/SO_GameplayBehavior/GameplayBehavior_NPC_PlayMontage.h"
#include "Demo_UE_SmartObject/Public/SO_GameplayBehavior/GameplayBehaviorConfig_NPC_PlayMontage.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "MotionWarpingComponent.h"
#include "SmartObjectSubsystem.h"
#include "Interface/SmartObjectInteractorInterface.h"
#include "Logging/StructuredLog.h"
#include "Utility/SmartObjectBlueprintFunctionLibraryEx.h"

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

	//Add MotionWarping
	AddOrUpdateWarpTargetToSlot(&InAvatar, AnimConfig->GetSlotMotionWarpingName());

	/*
	 * The version of UGameplayBehavior_AnimationBased::Trigger()
	 * However the timer wait for montage finished was not considered the PlayRate, so the montage will be playing in PlayRate correctly, but the timer won't end until MontageLength later
	 * (The core cause of this problem is the code 'Duration = AnimInstance->Montage_Play(NewAnimMontage, InPlayRate, EMontagePlayReturnType::MontageLength, StartTimeSeconds)' in UAbilitySystemComponent::PlayMontage())
	 */
	//return PlayMontage(InAvatar, *Anim, AnimConfig->GetPlayRate(), AnimConfig->GetStartSectionName(), AnimConfig->IsLooped());

	/*
	 * The new method is just fix the problem: consider the PlayRate (You can see my remarks easily within the code)
	 * If you encounter other problems by using the new method, you can replace it with the old one
	 */
	return PlayMontageNew(InAvatar, *Anim, AnimConfig->GetPlayRate(), AnimConfig->GetStartSectionName(), AnimConfig->IsLooped());
}

bool UGameplayBehavior_NPC_PlayMontage::NeedsInstance(const UGameplayBehaviorConfig* Config) const
{
	if (Cast<const UGameplayBehaviorConfig_NPC_PlayMontage>(Config) != nullptr)
		return true;

	return Super::NeedsInstance(Config);
}

bool UGameplayBehavior_NPC_PlayMontage::PlayMontageNew(AActor& InAvatar, UAnimMontage& AnimMontage, const float InPlayRate, const FName StartSectionName, const bool bLoop)
{
	bool bSuccess = false;
	UAbilitySystemComponent* ASC = Cast<UAbilitySystemComponent>(InAvatar.FindComponentByClass(UAbilitySystemComponent::StaticClass()));

	if (ASC)
	{
		FGameplayAbilityActivationInfo ActivationInfo(&InAvatar);
		ActivationInfo.bCanBeEndedByOtherInstance = true;
		const float PlaybackLength = ASC->PlayMontage(/*InAnimatingAbility=*/nullptr, ActivationInfo, &AnimMontage, InPlayRate, StartSectionName);
		if (PlaybackLength > 0)
		{
			FMontagePlaybackData* PlaybackData = ActivePlayback.FindByPredicate([&](const FMontagePlaybackData& Entry) { return Entry == &InAvatar; });
			if (PlaybackData == nullptr)
			{
				PlaybackData = &ActivePlayback.Add_GetRef(FMontagePlaybackData(InAvatar, AnimMontage, InPlayRate, StartSectionName, bLoop));
			}
			check(PlaybackData);
			PlaybackData->AbilityComponent = ASC;

			//UAnimInstance* AnimInstance = AbilityActorInfo.IsValid() ? AbilityActorInfo->GetAnimInstance() : nullptr;

			UWorld* World = InAvatar.GetWorld();
			if (World)
			{
				PlaybackData->TimerDelegate = FTimerDelegate::CreateUObject(this, &UGameplayBehavior_NPC_PlayMontage::OnMontageFinishedNew, &AnimMontage, false, &InAvatar);

				// ======================> The only difference : "PlaybackLength" -> "PlaybackLength / InPlayRate"
				//World->GetTimerManager().SetTimer(PlaybackData->TimerHandle, PlaybackData->TimerDelegate, PlaybackLength, /*bLoop=*/false);
				World->GetTimerManager().SetTimer(PlaybackData->TimerHandle, PlaybackData->TimerDelegate, PlaybackLength / InPlayRate, /*bLoop=*/false);
			}

			bSuccess = true;
		}
	}

	return bSuccess;
}

void UGameplayBehavior_NPC_PlayMontage::OnMontageFinishedNew(UAnimMontage* Montage, bool bInterrupted, AActor* InAvatar)
{
	FMontagePlaybackData* PlaybackData = ActivePlayback.FindByPredicate([&](const FMontagePlaybackData& Entry) { return Entry == InAvatar; });
	UE_LOG(LogGameplayBehavior, Error, TEXT("Added log!"));

	if (PlaybackData != nullptr)
	{
		check(Montage && InAvatar);

		if (bInterrupted == true || PlaybackData->bLoop == false
			|| PlaybackData->AbilityComponent == nullptr)
		{
			ActivePlayback.RemoveSingleSwap(*PlaybackData, EAllowShrinking::No);
			EndBehavior(*InAvatar, bInterrupted);
		}
		else
		{
			// request another playback
			FGameplayAbilityActivationInfo ActivationInfo(InAvatar);
			ActivationInfo.bCanBeEndedByOtherInstance = true;
			const float PlaybackLength = PlaybackData->AbilityComponent->PlayMontage(/*InAnimatingAbility=*/nullptr, ActivationInfo, Montage, PlaybackData->PlayRate, PlaybackData->SectionName);

			if (PlaybackLength > 0)
			{
				//UAnimInstance* AnimInstance = AbilityActorInfo.IsValid() ? AbilityActorInfo->GetAnimInstance() : nullptr;

				UWorld* World = InAvatar->GetWorld();
				if (World)
				{
					// ======================> The only difference : "PlaybackLength" -> "PlaybackLength / PlaybackData->PlayRate"
					//World->GetTimerManager().SetTimer(PlaybackData->TimerHandle, PlaybackData->TimerDelegate, PlaybackLength, /*bLoop=*/false);
					World->GetTimerManager().SetTimer(PlaybackData->TimerHandle, PlaybackData->TimerDelegate, PlaybackLength / PlaybackData->PlayRate, /*bLoop=*/false);
				}
			}
		}
	}
}

void UGameplayBehavior_NPC_PlayMontage::AddOrUpdateWarpTargetToSlot(AActor* Avatar, FName SlotMotionWarpingName, FSmartObjectSlotHandle SlotHandle)
{
	check(!SlotMotionWarpingName.IsNone())

	if (Avatar == nullptr)
	{
		UE_LOGFMT(LogTemp, Warning, "[{FUNC}] : InAvatar is nullptr.", __FUNCTION__);
		return;
	}

	// Operate SlotHandle
	if (!SlotHandle.IsValid())
	{
		// Try getting SlotHandle from SOClaimHandle if it's invalid.
		if (Avatar->Implements<USmartObjectInteractorInterface>())
		{
			ISmartObjectInteractorInterface* InteractorInterface = Cast<ISmartObjectInteractorInterface>(Avatar);
			FSmartObjectClaimHandle SOClaimHandle = InteractorInterface->GetSOClaimHandle();

			SlotHandle = SOClaimHandle.SlotHandle;
		}
	}

	// Get SlotTransform
	FTransform SlotTransform;
	if (!USmartObjectBlueprintFunctionLibraryEx::GetSlotTransformWithSlotHandle(Avatar, SlotTransform, SlotHandle))
	{
		UE_LOGFMT(LogTemp, Warning, "[{FUNC}] : failed to get slotTransform.", __FUNCTION__);
		return;
	}

	// Update motion Warping
	UMotionWarpingComponent* MotionWarpingComp = Avatar->FindComponentByClass<UMotionWarpingComponent>();
	if (MotionWarpingComp == nullptr)
	{
		UE_LOGFMT(LogTemp, Warning, "[{FUNC}] : failed to find UMotionWarpingComponent in {obj}.", __FUNCTION__, Avatar->GetName());
		return;
	}
	else
	{
		MotionWarpingComp->AddOrUpdateWarpTargetFromTransform(SlotMotionWarpingName, SlotTransform);
	}
}