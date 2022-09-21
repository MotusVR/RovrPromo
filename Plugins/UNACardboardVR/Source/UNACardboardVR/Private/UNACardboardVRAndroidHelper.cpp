// Copyright 2020 UNAmedia. All rights reserved.

#include "UNACardboardVRAndroidHelper.h"

#include "UNACardboardVRDevice.h"

#include "Async/Async.h"

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#endif

#if PLATFORM_ANDROID
extern "C"
{
	// Functions that are called on Android lifecycle events.

	JNI_METHOD void Java_com_unamedia_cardboard_unreal_UNACardboardVRJavaHelper_onApplicationCreated(JNIEnv*, jobject)
	{
        FUNACardboardVRAndroidHelper::OnApplicationCreated();
	}

	JNI_METHOD void Java_com_unamedia_cardboard_unreal_UNACardboardVRJavaHelper_onApplicationDestroyed(JNIEnv*, jobject)
	{
        FUNACardboardVRAndroidHelper::OnApplicationDestroyed();
	}

	JNI_METHOD void Java_com_unamedia_cardboard_unreal_UNACardboardVRJavaHelper_onApplicationPause(JNIEnv*, jobject)
	{
        FUNACardboardVRAndroidHelper::OnApplicationPause();
	}

	JNI_METHOD void Java_com_unamedia_cardboard_unreal_UNACardboardVRJavaHelper_onApplicationResume(JNIEnv*, jobject)
	{
        FUNACardboardVRAndroidHelper::OnApplicationResume();
	}

	JNI_METHOD void Java_com_unamedia_cardboard_unreal_UNACardboardVRJavaHelper_onApplicationStop(JNIEnv*, jobject)
	{
        FUNACardboardVRAndroidHelper::OnApplicationStop();
	}

	JNI_METHOD void Java_com_unamedia_cardboard_unreal_UNACardboardVRJavaHelper_onApplicationStart(JNIEnv*, jobject)
	{
        FUNACardboardVRAndroidHelper::OnApplicationStart();
	}

    JNI_METHOD void Java_com_unamedia_cardboard_unreal_UNACardboardVRJavaHelper_onDisplayOrientationChanged(JNIEnv*, jobject)
    {
        FUNACardboardVRAndroidHelper::OnDisplayOrientationChanged();
    }

    JNI_METHOD void Java_com_unamedia_cardboard_unreal_UNACardboardVRJavaHelper_onBackButtonPressed(JNIEnv*, jobject)
    {
        FUNACardboardVRAndroidHelper::OnBackButtonPressed();
    }

    JNI_METHOD void Java_com_unamedia_cardboard_unreal_UNACardboardVRJavaHelper_onSettingsMenuOpened(JNIEnv*, jobject)
    {
        FUNACardboardVRAndroidHelper::OnSettingsMenuOpened();
    }

    JNI_METHOD void Java_com_unamedia_cardboard_unreal_UNACardboardVRJavaHelper_onSwitchViewerButtonPressed(JNIEnv*, jobject)
    {
        FUNACardboardVRAndroidHelper::SwitchViewer();
    }

    JNI_METHOD void Java_com_unamedia_cardboard_unreal_UNACardboardVRJavaHelper_onToggleStereoModeButtonPressed(JNIEnv*, jobject)
    {
        FUNACardboardVRAndroidHelper::ToggleStereoRendering();
    }
}

// Redirect events to FUNACardboardVRDevice class:

void FUNACardboardVRAndroidHelper::OnApplicationCreated()
{
    FUNACardboardVRDevice::GetInstance().OnApplicationCreated();
}

void FUNACardboardVRAndroidHelper::OnApplicationDestroyed()
{
}

void FUNACardboardVRAndroidHelper::OnApplicationPause()
{
}

void FUNACardboardVRAndroidHelper::OnApplicationStart()
{
}

void FUNACardboardVRAndroidHelper::OnApplicationStop()
{
}

void FUNACardboardVRAndroidHelper::OnApplicationResume()
{
}

void FUNACardboardVRAndroidHelper::OnDisplayOrientationChanged()
{
}

void FUNACardboardVRAndroidHelper::OnBackButtonPressed()
{
    AsyncTask(ENamedThreads::GameThread, []() {
        FUNACardboardVRDevice::GetInstance().HandleBackEvent();
    });
}

void FUNACardboardVRAndroidHelper::OnSettingsMenuOpened()
{
    AsyncTask(ENamedThreads::GameThread, []() {
        FUNACardboardVRDevice::GetInstance().HandleSettingsMenuOpenEvent();
    });
}

void FUNACardboardVRAndroidHelper::SwitchViewer()
{
    AsyncTask(ENamedThreads::GameThread, []() {
        FUNACardboardVRDevice::GetInstance().ScanForQRViewerProfile();
    });
}

void FUNACardboardVRAndroidHelper::ToggleStereoRendering()
{
    AsyncTask(ENamedThreads::GameThread, []() {
        FUNACardboardVRDevice::GetInstance().ToggleStereoRendering();
    });
}

#endif
