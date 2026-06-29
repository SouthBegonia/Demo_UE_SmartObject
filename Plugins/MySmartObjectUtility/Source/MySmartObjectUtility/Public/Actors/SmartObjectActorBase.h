// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericSmartObject.h"
#include "GameFramework/Actor.h"
#include "Interface/SmartObjectInteracteeInterface.h"
#include "SmartObjectActorBase.generated.h"

struct FSmartObjectEventData;
class USmartObjectRenderingComponent;
class USmartObjectComponent;

/**
 * SmartObjectActorBase: A type of Character that includes the ability to interact with SmartObjects
 *		- feature:
 *			- Listening event of interacting(OnInteractionBegin/OnInteractionEnd). There is no way to trigger SO interaction actively because this is the job of BehaviorTree/StateTree.
 *			- Storing SOClaimHandle data as a backup. So that you can get it in BT/ST easily
 *
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class MYSMARTOBJECTUTILITY_API ASmartObjectActorBase : public AActor, public ISmartObjectInteracteeInterface
{
	GENERATED_BODY()

public:
	ASmartObjectActorBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());;

	UFUNCTION(BlueprintCallable, Category = "SmartObject")
	void SetSmartObjectEnabled(const bool bEnabled);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	UFUNCTION(CallInEditor, Category = SmartObject, meta = (DisplayName = "Enable SmartObject"))
	FORCEINLINE void EnableSmartObject() { SetSmartObjectEnabled(true); }

	UFUNCTION(CallInEditor, Category = SmartObject, meta = (DisplayName = "Disable SmartObject"))
	FORCEINLINE void DisableSmartObject() { SetSmartObjectEnabled(false); }
#endif // WITH_EDITOR

protected:
	/* The Root SceneComp. You can add StaticMeshComp to there if you need it. */
	UPROPERTY(EditAnywhere)
	TObjectPtr<USceneComponent> Root;

	/* Nodes that mount SmartObject Core Comp(USmartObjectComponent/USmartObjectRenderingComponent) */
	UPROPERTY(EditAnywhere)
	TObjectPtr<USceneComponent> RootSOComp;

	UPROPERTY(EditAnywhere, Category = SmartObject, NoClear)
	TObjectPtr<USmartObjectComponent> SOComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY(NoClear)
	TObjectPtr<USmartObjectRenderingComponent> RenderingComponent;
#endif // WITH_EDITORONLY_DATA

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void OnReceiveNativeSmartObjectEvent(const FSmartObjectEventData& EventData, const AActor* Interactor);

	//~ Begin ISmartObjectInteracteeInterface.
	virtual void OnSlotInteractionBegin(const FSmartObjectClaimHandle ClaimHandle, AActor* Interactor) override;
	virtual void OnSlotInteractionEnd() override;
	UFUNCTION(BlueprintPure, Category = "SmartObject")
	virtual USmartObjectComponent* GetSmartObjectComponent() const override;
	//~ End ISmartObjectInteracteeInterface

	UFUNCTION(BlueprintImplementableEvent, DisplayName="OnReceiveSmartObjectEvent")
	void K2_OnReceiveSmartObjectEvent(const FSmartObjectEventData& EventData, const AActor* Interactor);
	UFUNCTION(BlueprintImplementableEvent, DisplayName="OnSlotInteractionBegin")
	void K2_OnSlotInteractionBegin(const FSmartObjectClaimHandle ClaimHandle, AActor* Interactor);
	UFUNCTION(BlueprintImplementableEvent, DisplayName="OnSlotInteractionEnd")
	void K2_OnSlotInteractionEnd();

private:
	FDelegateHandle ReceiveSmartObjectEventDelegateHandle;
};
