// Copyright 2020 UNAmedia. All rights reserved.

using UnrealBuildTool;
using System.IO;

public class UNACardboardVR : ModuleRules
{
	public UNACardboardVR(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
				"Runtime/Renderer/Private"
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				"./ThirdParty/cardboard-sdk/include"
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Projects",
				"RHI",
				"RenderCore",
				"Renderer",
				"HeadMountedDisplay",
				"DeveloperSettings"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"ApplicationCore",
					"Launch"
				}
			);

			// https://nerivec.github.io/old-ue4-wiki/pages/how-to-add-a-shared-library-so-in-android-project.html
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "../ThirdParty/cardboard-sdk/build/android/armeabi-v7a/libcardboard_api.so"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "../ThirdParty/cardboard-sdk/build/android/arm64-v8a/libcardboard_api.so"));

			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "UNACardboardVR_Android_UPL.xml"));
		}
		else if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"SlateCore"
			});

			// Libraries needed by Cardboard SDK.
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "../ThirdParty/cardboard-sdk/build/ios/cardboard_api.a"));
			// Frameworks needed by Cardboard SDK.
			PublicFrameworks.Add("AVFoundation");
			PublicFrameworks.Add("CoreMotion");
			// Resource bundles needed by Cardboard SDK.
			AdditionalBundleResources.Add(new BundleResource(Path.Combine(ModuleDirectory, "../ThirdParty/cardboard-sdk/build/ios/sdk.bundle")));

			// Resource bundles needed by our plugin.
			AdditionalBundleResources.Add(new BundleResource(Path.Combine(ModuleDirectory, "Resources/ios/UNACardboard.bundle")));

			AdditionalPropertiesForReceipt.Add("IOSPlugin", Path.Combine(ModuleDirectory, "UNACardboardVR_iOS_UPL.xml"));
		}
	}
}
