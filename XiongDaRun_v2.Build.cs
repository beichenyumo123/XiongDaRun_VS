// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class XiongDaRun_v2 : ModuleRules
{
	public XiongDaRun_v2(ReadOnlyTargetRules Target) : base(Target)
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

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"XiongDaRun_v2",
			"XiongDaRun_v2/Variant_Platforming",
			"XiongDaRun_v2/Variant_Platforming/Animation",
			"XiongDaRun_v2/Variant_Combat",
			"XiongDaRun_v2/Variant_Combat/AI",
			"XiongDaRun_v2/Variant_Combat/Animation",
			"XiongDaRun_v2/Variant_Combat/Gameplay",
			"XiongDaRun_v2/Variant_Combat/Interfaces",
			"XiongDaRun_v2/Variant_Combat/UI",
			"XiongDaRun_v2/Variant_SideScrolling",
			"XiongDaRun_v2/Variant_SideScrolling/AI",
			"XiongDaRun_v2/Variant_SideScrolling/Gameplay",
			"XiongDaRun_v2/Variant_SideScrolling/Interfaces",
			"XiongDaRun_v2/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
