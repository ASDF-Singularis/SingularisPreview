using UnrealBuildTool;

public class SingularisPreview : ModuleRules
{
	public SingularisPreview(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;


		PrivateDependencyModuleNames.AddRange(
			[
				"Core",
				"CoreUObject",
				"Engine"
			]
		);
	}
}