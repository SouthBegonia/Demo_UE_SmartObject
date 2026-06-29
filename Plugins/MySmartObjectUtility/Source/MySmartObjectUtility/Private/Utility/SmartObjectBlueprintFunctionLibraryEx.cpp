// Fill out your copyright notice in the Description page of Project Settings.

#include "Utility/SmartObjectBlueprintFunctionLibraryEx.h"
#include "Logging/StructuredLog.h"
#include "Interface/SmartObjectInteractorInterface.h"

bool USmartObjectBlueprintFunctionLibraryEx::GetSlotTransformWithSlotHandle(UObject* WorldContextObject, FTransform& OutSlotTransform, const FSmartObjectSlotHandle& SlotHandle)
{
             	USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(WorldContextObject->GetWorld());
	if (SmartObjectSubsystem == nullptr)
	{
		UE_LOGFMT(LogTemp, Warning, "[{FUNC}] : SmartObjectSubsystem is invalid.", __FUNCTION__);
		return false;
	}

	const TOptional<FTransform> GoalTransform = SmartObjectSubsystem->GetSlotTransform(SlotHandle);
	if (GoalTransform.IsSet())
	{
		OutSlotTransform = GoalTransform.GetValue();
		return true;
	}
	else
	{
		UE_LOGFMT(LogTemp, Warning, "[{FUNC}] : Get SlotTransform failed.", __FUNCTION__);
	}

	return false;
}

bool USmartObjectBlueprintFunctionLibraryEx::GetSlotEntranceTransformWithSlotHandle(UObject* WorldContextObject, FTransform& OutEntranceTransform, const FSmartObjectSlotHandle& SlotHandle,
	FSmartObjectSlotEntranceLocationRequest EntranceRequest)
{
	USmartObjectSubsystem* SmartObjectSubsystem = USmartObjectSubsystem::GetCurrent(WorldContextObject->GetWorld());
	if (SmartObjectSubsystem == nullptr)
	{
		UE_LOGFMT(LogTemp, Warning, "[{FUNC}] : SmartObjectSubsystem is invalid.", __FUNCTION__);
		return false;
	}

	FSmartObjectSlotEntranceLocationRequest& Request = EntranceRequest;
	FSmartObjectSlotEntranceLocationResult Result;
	if (SmartObjectSubsystem->FindEntranceLocationForSlot(SlotHandle, Request, Result))
	{
		OutEntranceTransform = FTransform(Result.Rotation, Result.Location);
		return true;
	}

	return false;
}

bool USmartObjectBlueprintFunctionLibraryEx::GetSOClaimHandleFromInteractor(AActor* Interactor, FSmartObjectClaimHandle& OutSOClaimHandle, const FString& ClaimHandleBlackboardKeyName)
{
	if (Interactor == nullptr)
		return false;

	if (Interactor->Implements<USmartObjectInteractorInterface>())
	{
		ISmartObjectInteractorInterface* InteractorInterface = Cast<ISmartObjectInteractorInterface>(Interactor);
		OutSOClaimHandle = InteractorInterface->GetSOClaimHandle(FName(*ClaimHandleBlackboardKeyName));
		return true;
	}

	return false;
}

