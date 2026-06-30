// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/SmartObjectActorBase.h"

#include "SmartObjectComponent.h"
#include "SmartObjectRenderingComponent.h"
#include "Components/BillboardComponent.h"

#if WITH_EDITOR
#include "ObjectEditorUtils.h"
#endif // WITH_EDITOR

ASmartObjectActorBase::ASmartObjectActorBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	RootSOComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootSOComp"));
	RootSOComp->SetupAttachment(RootComponent);

	SOComponent = CreateDefaultSubobject<USmartObjectComponent>(TEXT("SmartObjectComp"));
	SOComponent->SetupAttachment(RootSOComp);


#if WITH_EDITORONLY_DATA
	UBillboardComponent* SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	RenderingComponent = CreateEditorOnlyDefaultSubobject<USmartObjectRenderingComponent>(TEXT("RenderComp"));

	if (!IsRunningCommandlet())
	{
		// Structure to hold one-time initialization
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> NoteTextureObject;
			FName NotesID;
			FText GenericSOName;
			FConstructorStatics()
				: NoteTextureObject(TEXT("/SmartObjects/S_SmartObject"))
				, NotesID(TEXT("SmartObject"))
				, GenericSOName(NSLOCTEXT("SpriteCategory", "GenericSO", "GenericSO"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;
		if (SpriteComponent)
		{
			SpriteComponent->Sprite = ConstructorStatics.NoteTextureObject.Get();
			SpriteComponent->SetRelativeScale3D_Direct(FVector(0.5f, 0.5f, 0.5f));
			SpriteComponent->SpriteInfo.Category = ConstructorStatics.NotesID;
			SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.GenericSOName;
			SpriteComponent->SetupAttachment(RootSOComp);
			SpriteComponent->Mobility = EComponentMobility::Movable;
		}

		if (RenderingComponent)
		{
			RenderingComponent->SetupAttachment(RootSOComp);
		}
	}

#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITOR
void ASmartObjectActorBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName SmartObjectName = FName(TEXT("SmartObject"));

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		if (FObjectEditorUtils::GetCategoryFName(PropertyChangedEvent.Property) == SmartObjectName)
		{
			if (RenderingComponent)
			{
				MarkComponentsRenderStateDirty();
			}
		}
	}
}
#endif // WITH_EDITOR


// Called when the game starts or when spawned
void ASmartObjectActorBase::BeginPlay()
{
	Super::BeginPlay();

	ReceiveSmartObjectEventDelegateHandle = SOComponent->GetOnSmartObjectEventNative().AddUObject(this, &ASmartObjectActorBase::OnReceiveNativeSmartObjectEvent);
}

void ASmartObjectActorBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ReceiveSmartObjectEventDelegateHandle.IsValid())
	{
		SOComponent->GetOnSmartObjectEventNative().Remove(ReceiveSmartObjectEventDelegateHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ASmartObjectActorBase::SetSmartObjectEnabled(const bool bEnabled)
{
	if (!SOComponent->SetSmartObjectEnabled(bEnabled))
		UE_LOG(LogTemp, Warning, TEXT("Failed to enable/disable SmartObject = %s"), *GetName());
}


void ASmartObjectActorBase::OnSlotInteractionBegin(const FSmartObjectClaimHandle ClaimHandle, AActor* Interactor)
{
	K2_OnSlotInteractionBegin(ClaimHandle, Interactor);
}

void ASmartObjectActorBase::OnSlotInteractionEnd()
{
	K2_OnSlotInteractionEnd();
}

USmartObjectComponent* ASmartObjectActorBase::GetSmartObjectComponent() const
{
	return SOComponent;
}

void ASmartObjectActorBase::OnReceiveNativeSmartObjectEvent(const FSmartObjectEventData& EventData, const AActor* Interactor)
{
	K2_OnReceiveSmartObjectEvent(EventData, Interactor);
}