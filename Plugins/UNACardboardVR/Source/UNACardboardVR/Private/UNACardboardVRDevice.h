// Copyright 2020 UNAmedia. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#if PLATFORM_IOS
	@class UNACardboardOverlayView;
#endif



class FUNACardboardVRDevice final
{
public:

    static FUNACardboardVRDevice& GetInstance();

	void Initialize();
	void Deinitialize();

    bool GetAndThenReset_IsDeviceParamsChanged();
    void ScanForQRViewerProfile();
    void SaveDeviceParams(const FString& encodedDeviceParams);
    void SaveDeviceParamsFromURL(const FString& uri);
    FString GetSavedDeviceParams() const;
    FString GetCardboardV1DeviceParams() const;
    void ToggleStereoRendering();
    void HandleNativeUI(bool bStereoEnabled);
    void HandleBackEvent();
    void HandleSettingsMenuOpenEvent();

    void SetBackButtonVisibility(bool bVisible);
    void SetToggleStereoModeButtonVisibity(bool bVisible);
    void SetNativeUIVisibility(bool bVisible);

    bool ShouldBeNativeUIVisible() const;
    // always returns true in stereo, false otherwise
    bool ShouldBeSettingsButtonVisible() const;
    bool ShouldBeBackButtonVisible() const;
    bool ShouldBeToggleStereoModeButtonVisible() const;

    bool IsStereoEnabled() const;

private:

    FUNACardboardVRDevice();

    friend class FUNACardboardVRAndroidHelper;
    friend class FUNACardboardVRHMD;

    void OnApplicationCreated();

    void HandleQRScanningAutomatically();

private:

    UWorld* GetWorld() const;

private:

    int qrCodeScanCount = 0;

#if DO_CHECK
	bool bIsInitialized = false;
#endif

#if PLATFORM_IOS
	UNACardboardOverlayView * OverlayView;
#endif
};
