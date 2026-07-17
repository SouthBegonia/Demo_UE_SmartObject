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

	// Set Avatar Transform (Teleport directly or Add MotionWarping)
	if (AnimConfig->IsTeleportAvatarToSlotTransform())
		TeleportAvatarToSlot(&InAvatar);
	else if (!AnimConfig->GetSlotMotionWarpingName().IsNone())
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
	bool bPlaySuccess = PlayMontageNew(InAvatar, *Anim, AnimConfig->GetPlayRate(), AnimConfig->GetStartSectionName(), AnimConfig->IsLooped());

	// Add timer for PlayTime
	const float PlayTime = AnimConfig->GetPlayTimeWithingRandomDeviation();
	if (bPlaySuccess && PlayTime > 0.f)
	{
		UAbilitySystemComponent* ASC = Cast<UAbilitySystemComponent>(InAvatar.FindComponentByClass(UAbilitySystemComponent::StaticClass()));
		if (ASC)
		{
			if (UWorld* World = InAvatar.GetWorld())
			{
				PlayTimerDelegate = FTimerDelegate::CreateUObject(this, &UGameplayBehavior_NPC_PlayMontage::OnPlayTimerFinished, Anim, &InAvatar);
				World->GetTimerManager().SetTimer(PlayTimerHandle, PlayTimerDelegate, PlayTime, /*bLoop=*/false);
			}
		}
	}

	return bPlaySuccess;
}

void UGameplayBehavior_NPC_PlayMontage::EndBehavior(AActor& Avatar, const bool bInterrupted)
{
	// Clear PlayTime timer
	if (PlayTimerHandle.IsValid())
	{
		if (UWorld* World = Avatar.GetWorld())
		{
			World->GetTimerManager().ClearTimer(PlayTimerHandle);
		}
	}

	Super::EndBehavior(Avatar, bInterrupted);
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

void UGameplayBehavior_NPC_PlayMontage::OnPlayTimerFinished(UAnimMontage* Montage, AActor* InAvatar)
{
	// Note: You can choose one of logic to EndBehavior, the result of bInterrupted will affect the BT/ST

	// PlanA: Timer of PlayTime finished, regard this situation as a [normal termination]
	FMontagePlaybackData* PlaybackData = ActivePlayback.FindByPredicate([&](const FMontagePlaybackData& Entry) { return Entry == InAvatar; });
	if (PlaybackData)
	{
		UWorld* World = InAvatar->GetWorld();
		if (World && PlaybackData->TimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(PlaybackData->TimerHandle);
		}
		if (PlaybackData->AbilityComponent && PlaybackData->AnimMontage)
		{
			PlaybackData->AbilityComponent->StopMontageIfCurrent(*PlaybackData->AnimMontage);
		}
		ActivePlayback.RemoveSingleSwap(*PlaybackData, EAllowShrinking::No);
	}
	EndBehavior(*InAvatar, false);

	// PlanB: Timer of PlayTime finished, regard this situation as an [interruption]
	//EndBehavior(*InAvatar, true);
}

void UGameplayBehavior_NPC_PlayMontage::TeleportAvatarToSlot(AActor* Avatar, FSmartObjectSlotHandle SlotHandle)
{
	if (Avatar == nullptr)
	{
		UE_LOGFMT(LogTemp, Warning, "[{FUNC}] : InAvatar is nullptr.", __FUNCTION__);
		return;
	}

	// Get SlotTransform
	FTransform SlotTransform;
	if (!GetSlotTransform(SlotTransform, Avatar, SlotHandle))
	{
		return;
	}

	// Set Avatar Transform
	Avatar->SetActorTransform(SlotTransform);
}

void UGameplayBehavior_NPC_PlayMontage::AddOrUpdateWarpTargetToSlot(AActor* Avatar, FName SlotMotionWarpingName, FSmartObjectSlotHandle SlotHandle)
{
	if (SlotMotionWarpingName.IsNone())
		return;

	if (Avatar == nullptr)
	{
		UE_LOGFMT(LogTemp, Warning, "[{FUNC}] : InAvatar is nullptr.", __FUNCTION__);
		return;
	}

	// Get SlotTransform
	FTransform SlotTransform;
	if (!GetSlotTransform(SlotTransform, Avatar, SlotHandle))
	{
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

bool UGameplayBehavior_NPC_PlayMontage::GetSlotTransform(FTransform& ResultSloTransform, AActor* Avatar, FSmartObjectSlotHandle SlotHandle) const
{
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
	if (!SlotHandle.IsValid())
	{
		UE_LOGFMT(LogTemp, Warning, "[{FUNC}] : SlotHandle is invalid.", __FUNCTION__);
		return false;
	}

	// Get SlotTransform
	FTransform SlotTransform;
	if (!USmartObjectBlueprintFunctionLibraryEx::GetSlotTransformWithSlotHandle(Avatar, SlotTransform, SlotHandle))
	{
		UE_LOGFMT(LogTemp, Warning, "[{FUNC}] : failed to get slotTransform.", __FUNCTION__);
		return false;
	}

	ResultSloTransform = SlotTransform;
	return true;
}
