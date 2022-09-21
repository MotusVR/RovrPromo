// Copyright 2020 UNAmedia. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#endif

/** Wrappers for accessing Android Java stuff */
class FUNACardboardVRAndroidHelper
{
public:

#if PLATFORM_ANDROID
	// Helpers for redirecting Android events.
	static void OnApplicationCreated();
	static void OnApplicationDestroyed();
	static void OnApplicationPause();
	static void OnApplicationResume();
	static void OnApplicationStop();
	static void OnApplicationStart();
	static void OnDisplayOrientationChanged();
    static void OnBackButtonPressed();
    static void OnSettingsMenuOpened();
    static void SwitchViewer();
    static void ToggleStereoRendering();
#endif
};
