// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AmongUs : ModuleRules
{
	public AmongUs(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "AITestSuite" });

		PublicIncludePaths.AddRange(new string[] {
			"AmongUs",
			"AmongUs/Variant_Platforming",
			"AmongUs/Variant_Platforming/Animation",
			"AmongUs/Variant_Combat",
			"AmongUs/Variant_Combat/AI",
			"AmongUs/Variant_Combat/Animation",
			"AmongUs/Variant_Combat/Gameplay",
			"AmongUs/Variant_Combat/Interfaces",
			"AmongUs/Variant_Combat/UI",
			"AmongUs/Variant_SideScrolling",
			"AmongUs/Variant_SideScrolling/AI",
			"AmongUs/Variant_SideScrolling/Gameplay",
			"AmongUs/Variant_SideScrolling/Interfaces",
			"AmongUs/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
