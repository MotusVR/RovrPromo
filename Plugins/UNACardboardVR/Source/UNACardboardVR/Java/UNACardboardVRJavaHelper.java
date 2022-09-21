// Copyright 2020 UNAmedia. All rights reserved.

package com.unamedia.cardboard.unreal;

public class UNACardboardVRJavaHelper {

    public static native void onApplicationCreated();
    public static native void onApplicationDestroyed();
    public static native void onApplicationPause();
    public static native void onApplicationResume();
    public static native void onApplicationStop();
    public static native void onApplicationStart();
    public static native void onDisplayOrientationChanged();

	public static native void onBackButtonPressed();
	public static native void onSettingsMenuOpened();
	public static native void onSwitchViewerButtonPressed();
	public static native void onToggleStereoModeButtonPressed();
}
