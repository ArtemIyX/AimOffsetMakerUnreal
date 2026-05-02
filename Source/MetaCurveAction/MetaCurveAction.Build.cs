using UnrealBuildTool;

public class MetaCurveAction : ModuleRules
{
	public MetaCurveAction(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"ContentBrowser",
				"UnrealEd",
				"AssetRegistry",
				"AnimationBlueprintLibrary"
			}
		);
	}
}
