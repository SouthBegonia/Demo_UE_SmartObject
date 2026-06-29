// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SmartObjectSubsystem.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SmartObjectBlueprintFunctionLibraryEx.generated.h"

/**
 * The extension of @USmartObjectBlueprintFunctionLibrary
 *
 */
UCLASS()
class MYSMARTOBJECTUTILITY_API USmartObjectBlueprintFunctionLibraryEx : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SmartObject", meta=(WorldContext="WorldContextObject"))
	static bool GetSlotTransformWithSlotHandle(UObject* WorldContextObject, FTransform& OutSlotTransform, const FSmartObjectSlotHandle& SlotHandle);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SmartObject", meta=(WorldContext="WorldContextObject"))
	static bool GetSlotEntranceTransformWithSlotHandle(UObject* WorldContextObject, FTransform& OutEntranceTransform, const FSmartObjectSlotHandle& SlotHandle, FSmartObjectSlotEntranceLocationRequest EntranceRequest = FSmartObjectSlotEntranceLocationRequest());

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SmartObject", meta=(WorldContext="WorldContextObject"))
	static bool GetSlotTransformWithClaimHandle(UObject* WorldContextObject, FTransform& OutSlotTransform, const FSmartObjectClaimHandle& ClaimHandle) { return GetSlotTransformWithSlotHandle(WorldContextObject, OutSlotTransform, ClaimHandle.SlotHandle); }
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SmartObject", meta=(WorldContext="WorldContextObject"))
	static bool GetSlotEntranceTransformWithClaimHandle(UObject* WorldContextObject, FTransform& OutEntranceTransform, const FSmartObjectClaimHandle& ClaimHandle, FSmartObjectSlotEntranceLocationRequest EntranceRequest = FSmartObjectSlotEntranceLocationRequest()) { return GetSlotEntranceTransformWithSlotHandle(WorldContextObject, OutEntranceTransform, ClaimHandle.SlotHandle, EntranceRequest); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SmartObject")
	static bool GetSOClaimHandleFromInteractor(AActor* Interactor, FSmartObjectClaimHandle& OutSOClaimHandle, const FString& ClaimHandleBlackboardKeyName = FString(""));
};
