// Copyright (c) 2018 Isara Technologies. All Rights Reserved.

#pragma once

#include "Engine/Texture2D.h"
#include "AndroidAPITemplateFunctions.generated.h"



UCLASS(NotBlueprintable)
class UAndroidAPITemplateFunctions : public UObject {
	GENERATED_BODY()
	
public:

#if PLATFORM_ANDROID
	static void InitJavaFunctions();
#endif

	/**
	 * Displays a toast text on the screen
	 */

	UFUNCTION(BlueprintCallable, meta = (Keywords = "AndroidAPI ", DisplayName = "Show Toast"), Category = "AndroidAPI")
		static void AndroidAPITemplate_ShowToast(const FString& Content);

	UFUNCTION(BlueprintCallable, meta = (Keywords = "ROVR Relieve External Storage", DisplayName = "Find SD Card Name"), Category = "ROVR Relieve External Storage")
		static FString AndroidAPITemplate_Test();

	UFUNCTION(BlueprintCallable, meta = (Keywords = "Get Video Thumbnail", DisplayName = "Get Video Thumbnail"), Category = "ROVR Relieve External Storage")
		static UTexture2D* AndroidAPITemplate_Test2(int32 thumbNum,bool headset);

};
