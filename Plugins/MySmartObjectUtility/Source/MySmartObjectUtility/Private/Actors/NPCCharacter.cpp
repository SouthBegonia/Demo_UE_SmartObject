// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/NPCCharacter.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "MotionWarpingComponent.h"
#include "SmartObjectBlueprintFunctionLibrary.h"
#include "SmartObjectUserComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


// Sets default values
ANPCCharacter::ANPCCharacter()
{
	// Replication settings
	bReplicates = true;
	SetReplicatingMovement(true);

	// Movement settings
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->GetNavMovementProperties()->bUseAccelerationForPaths = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);
	// RVO settings
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 100.f;	// must be greater than CapsuleRadius
	GetCharacterMovement()->AvoidanceWeight = 0.5f;		// a neutral value is 0.5f


	// Optimization settings
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;


	// Core Components
	SOUerComp = CreateDefaultSubobject<USmartObjectUserComponent>("SOUerComp");

	// Optional Components
	MotionWarpComp = CreateOptionalDefaultSubobject<UMotionWarpingComponent>("MotionWarpComp");
	ASC = CreateOptionalDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	if (IsValid(ASC))
	{
		ASC->SetIsReplicated(true);
		ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	}
}

// Called when the game starts or when spawned
void ANPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(ASC))
		ASC->InitAbilityActorInfo(this, this);
}

void ANPCCharacter::OnInteractionBegin(const FSmartObjectClaimHandle ClaimHandle, AActor* InteracteeActor)
{
	K2_OnInteractionBegin(ClaimHandle, InteracteeActor);
}

void ANPCCharacter::OnInteractionEnd()
{
	K2_OnInteractionEnd();
}

void ANPCCharacter::SetSOClaimHandle(const FSmartObjectClaimHandle ClaimHandle, const FName& ClaimHandleBlackboardKeyName)
{
	// Save to Blackboard if it exists
	AAIController* AIController = Cast<AAIController>(Controller);
	if (AIController)
	{
		const FName& BBKeyName = !ClaimHandleBlackboardKeyName.IsNone() ? ClaimHandleBlackboardKeyName : DefaultSOClaimHandleBBKeyName;
		USmartObjectBlueprintFunctionLibrary::SetValueAsSOClaimHandle(AIController->GetBlackboardComponent(), BBKeyName, ClaimHandle);
	}

	// Also Save to Self
	SOClaimHandle = ClaimHandle;
}

FSmartObjectClaimHandle ANPCCharacter::GetSOClaimHandle(const FName& ClaimHandleBlackboardKeyName) const
{
	// Try getting from Blackboard if it exists
	AAIController* AIController = Cast<AAIController>(Controller);
	if (AIController)
	{
		const FName& BBKeyName = !ClaimHandleBlackboardKeyName.IsNone() ? ClaimHandleBlackboardKeyName : DefaultSOClaimHandleBBKeyName;
		FSmartObjectClaimHandle SOClaimHandleFromBB = USmartObjectBlueprintFunctionLibrary::GetValueAsSOClaimHandle(AIController->GetBlackboardComponent(), BBKeyName);
		if (SOClaimHandleFromBB.IsValid())
			return SOClaimHandleFromBB;
	}

	// Otherwise getting from Self
	return SOClaimHandle;
}

void ANPCCharacter::InvalidateSOClaimHandle(const FName& ClaimHandleBlackboardKeyName)
{
	AAIController* AIController = Cast<AAIController>(Controller);
	if (AIController)
	{
		const FName& BBKeyName = !ClaimHandleBlackboardKeyName.IsNone() ? ClaimHandleBlackboardKeyName : DefaultSOClaimHandleBBKeyName;
		USmartObjectBlueprintFunctionLibrary::SetValueAsSOClaimHandle(AIController->GetBlackboardComponent(), BBKeyName, {});
	}

	SOClaimHandle.Invalidate();
}

