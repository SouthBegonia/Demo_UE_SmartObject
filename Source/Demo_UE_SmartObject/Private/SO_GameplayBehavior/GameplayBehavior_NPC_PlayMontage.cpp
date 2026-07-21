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
	InstantiationPolicy = EGameplayBehaviorInstantiationPolicy::DontInstantiate;
}

bool UGameplayBehavior_NPC_PlayMontage::Trigger(AActor& InAvatar, const UGameplayBehaviorConfig* Config, AActor* SmartObjectOwner)
{
	bool bTriggerSuccessful = false;

	// Check BehaviorConfig
	const UGameplayBehaviorConfig_NPC_PlayMontage* NPCPlayMontageConfig = Cast<const UGameplayBehaviorConfig_NPC_PlayMontage>(Config);
	if (NPCPlayMontageConfig == nullptr)
	{
		UE_VLOG(&InAvatar, LogGameplayBehavior, Log, TEXT("%s failed to trigger [%s] due to [%s] being null")
			, *InAvatar.GetName() ,
			*UGameplayBehavior_NPC_PlayMontage::StaticClass()->GetName(),
			*UGameplayBehaviorConfig_NPC_PlayMontage::StaticClass()->GetName());
		return false;
	}
	UAnimMontage* Anim = NPCPlayMontageConfig->GetMontage();	// Note: This method cannot be called multiple times
	if (Anim == nullptr)
	{
		UE_VLOG(&InAvatar, LogGameplayBehavior, Log, TEXT("%s failed to trigger [%s] due to GetMontage produced an invalid result.")
			, *InAvatar.GetName() ,
			*UGameplayBehavior_NPC_PlayMontage::StaticClass()->GetName());
		return false;
	}

	// Get info from Avatar
	UAbilitySystemComponent* ASC = Cast<UAbilitySystemComponent>(InAvatar.FindComponentByClass(UAbilitySystemComponent::StaticClass()));
	if (ASC == nullptr)
	{
		UE_VLOG(&InAvatar, LogGameplayBehavior, Log, TEXT("%s failed to trigger [%s] due to UAbilitySystemComponent not found.")
			, *InAvatar.GetName() ,
			*UGameplayBehavior_NPC_PlayMontage::StaticClass()->GetName());
		return false;
	}

	// Create the unique PlaybackData
	FNPCMontagePlaybackData* NPCPlaybackData = NPCActivePlaybackList.FindByPredicate([&](const FNPCMontagePlaybackData& Entry) { return Entry == &InAvatar; });
	if (NPCPlaybackData == nullptr)
	{
		NPCPlaybackData = &NPCActivePlaybackList.Add_GetRef(FNPCMontagePlaybackData(
			InAvatar,
			*Anim,
			NPCPlayMontageConfig->GetPlayRate(), NPCPlayMontageConfig->GetStartSectionName(), NPCPlayMontageConfig->IsLooped()));

		check(NPCPlaybackData);
		NPCPlaybackData->AbilityComponent = ASC;
	}
	else
	{
		UE_VLOG(&InAvatar, LogGameplayBehavior, Log, TEXT("Avatar can only trigger the [%s] at a time.")
			, *UGameplayBehavior_NPC_PlayMontage::StaticClass()->GetName());
		return false;
	}

	// Set Avatar Transform (Teleport directly or Add MotionWarping)
	if (NPCPlayMontageConfig->IsTeleportAvatarToSlotTransform())
		TeleportAvatarToSlot(&InAvatar);
	else if (!NPCPlayMontageConfig->GetSlotMotionWarpingName().IsNone())
		AddOrUpdateWarpTargetToSlot(&InAvatar, NPCPlayMontageConfig->GetSlotMotionWarpingName());

	// Play Montage
	bool bPlaySuccess = PlayMontage(InAvatar, *NPCPlaybackData);

	// Add timer for PlayTime
	const float PlayTime = NPCPlayMontageConfig->GetPlayTimeWithingRandomDeviation();
	if (bPlaySuccess && PlayTime > 0.f)
	{
		if (UWorld* World = InAvatar.GetWorld())
		{
			NPCPlaybackData->PlaySuspendTimerDelegate = FTimerDelegate::CreateUObject(this, &UGameplayBehavior_NPC_PlayMontage::OnPlayTimerFinished, Anim, &InAvatar);
			World->GetTimerManager().SetTimer(NPCPlaybackData->PlaySuspendTimerHandle, NPCPlaybackData->PlaySuspendTimerDelegate, PlayTime, /*bLoop=*/false);
		}
	}

	bTriggerSuccessful = bPlaySuccess;
	if (!bTriggerSuccessful)
	{
		// Clear PlaybackData when trigger behavior failed.
		NPCActivePlaybackList.RemoveSingleSwap(*NPCPlaybackData, EAllowShrinking::No);
	}

	return bTriggerSuccessful;
}

void UGameplayBehavior_NPC_PlayMontage::EndBehavior(AActor& Avatar, const bool bInterrupted)
{
	FNPCMontagePlaybackData* NPCPlaybackData = NPCActivePlaybackList.FindByPredicate([&](const FNPCMontagePlaybackData& Entry) { return Entry == &Avatar; });
	if (NPCPlaybackData)
	{
		UWorld* World = Avatar.GetWorld();

		// Stop Montage if interrupted
		if (bInterrupted)
		{
			if (NPCPlaybackData->AbilityComponent && NPCPlaybackData->AnimMontage)
				NPCPlaybackData->AbilityComponent->StopMontageIfCurrent(*NPCPlaybackData->AnimMontage);
		}

		// Clear TimerHandle
		if (World && NPCPlaybackData->TimerHandle.IsValid())
			World->GetTimerManager().ClearTimer(NPCPlaybackData->TimerHandle);
		if (World && NPCPlaybackData->PlaySuspendTimerHandle.IsValid())
			World->GetTimerManager().ClearTimer(NPCPlaybackData->PlaySuspendTimerHandle);

		// Clear NPCPlaybackData
		NPCActivePlaybackList.RemoveSingleSwap(*NPCPlaybackData, EAllowShrinking::No);
	}

	Super::EndBehavior(Avatar, bInterrupted);
}

bool UGameplayBehavior_NPC_PlayMontage::PlayMontage(AActor& InAvatar, FNPCMontagePlaybackData& PlaybackData)
{
	bool bSuccess = false;

	FGameplayAbilityActivationInfo ActivationInfo(&InAvatar);
	ActivationInfo.bCanBeEndedByOtherInstance = true;

	UAbilitySystemComponent* ASC = PlaybackData.AbilityComponent.Get();
	UAnimMontage* AnimMontage = PlaybackData.AnimMontage.Get();
	check(ASC && AnimMontage)

	const float PlaybackLength = ASC->PlayMontage(/*InAnimatingAbility=*/nullptr, ActivationInfo, AnimMontage, PlaybackData.PlayRate, PlaybackData.SectionName);
	if (PlaybackLength > 0)
	{
		UWorld* World = InAvatar.GetWorld();
		if (World)
		{
			PlaybackData.TimerDelegate = FTimerDelegate::CreateUObject(this, &UGameplayBehavior_NPC_PlayMontage::OnMontageFinished, AnimMontage, false, &InAvatar);

			/*
			 * The version of UGameplayBehavior_AnimationBased::Trigger()
			 * However the timer wait for montage finished was not considered the PlayRate, so the montage will be playing in PlayRate correctly, but the timer won't end until MontageLength later
			 * (The core cause of this problem is the code 'Duration = AnimInstance->Montage_Play(NewAnimMontage, InPlayRate, EMontagePlayReturnType::MontageLength, StartTimeSeconds)' in UAbilitySystemComponent::PlayMontage())
			 */
			// ======================> The only difference : "PlaybackLength" -> "PlaybackLength / InPlayRate"
			//World->GetTimerManager().SetTimer(PlaybackData->TimerHandle, PlaybackData->TimerDelegate, PlaybackLength, /*bLoop=*/false);

			World->GetTimerManager().SetTimer(PlaybackData.TimerHandle, PlaybackData.TimerDelegate, PlaybackLength / PlaybackData.PlayRate, /*bLoop=*/false);
		}

		bSuccess = true;
	}

	return bSuccess;
}

void UGameplayBehavior_NPC_PlayMontage::OnMontageFinished(UAnimMontage* Montage, bool bInterrupted, AActor* InAvatar)
{
	FNPCMontagePlaybackData* PlaybackData = NPCActivePlaybackList.FindByPredicate([&](const FNPCMontagePlaybackData& Entry) { return Entry == InAvatar; });

	if (PlaybackData != nullptr)
	{
		check(Montage && InAvatar);

		if (bInterrupted == true || PlaybackData->bLoop == false
			|| PlaybackData->AbilityComponent == nullptr)
		{
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
	FNPCMontagePlaybackData* PlaybackData = NPCActivePlaybackList.FindByPredicate([&](const FNPCMontagePlaybackData& Entry) { return Entry == InAvatar; });
	if (PlaybackData != nullptr)
	{
		// Stop Montage
		if (PlaybackData->AbilityComponent && PlaybackData->AnimMontage)
			PlaybackData->AbilityComponent->StopMontageIfCurrent(*PlaybackData->AnimMontage);
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
