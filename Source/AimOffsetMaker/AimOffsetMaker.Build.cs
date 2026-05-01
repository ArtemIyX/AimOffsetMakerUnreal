using UnrealBuildTool;

public class AimOffsetMaker : ModuleRules
{
	public AimOffsetMaker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"InputCore",
				"Slate",
				"SlateCore",
				"AssetRegistry",
				"Projects",
				"ToolMenus",
				"AdvancedPreviewScene",
				"EditorFramework",
				"UnrealEd",
				"AssetTools",
				"ContentBrowser",
				"AnimationBlueprintLibrary",
				"PropertyEditor",
				"Persona",
				"AnimationEditor"
			}
		);
	}
}
