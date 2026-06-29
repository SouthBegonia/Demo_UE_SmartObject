// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Demo_UE_SmartObject : ModuleRules
{
	public Demo_UE_SmartObject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",

			"AIModule", "GameplayTasks",
			"SmartObjectsModule", "GameplayBehaviorSmartObjectsModule", "GameplayBehaviorsModule",
			"StateTreeModule", "GameplayStateTreeModule",
			"MotionWarping", "GameplayAbilities", "GameplayTags", "NavigationSystem",

		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
