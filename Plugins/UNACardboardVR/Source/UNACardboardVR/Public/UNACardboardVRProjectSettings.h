// Copyright 2020 UNAmedia. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"

#include "UNACardboardVRProjectSettings.generated.h"

/**
UNACardboardVR project settings.

Most of the settings can be updated at run-time using the UUNACardboardVRStatics class methods.
*/
UCLASS(Config=Game, DefaultConfig, Meta=(DisplayName="Cardboard VR"))
class UNACARDBOARDVR_API UUNACardboardVRProjectSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:

    /**
     * Whether or not to show the Cardboard native UI.
     * This is a native overlay showing commonly useful buttons to allow the user to enable/disable the stereo view and
     * to open the QR scanner to retrieve the cardboard viewer parameters.
     * Several features of this native UI can be configured with dedicated settings (see #bShowBackButton and #bShowToggleStereoModeButton).
     * You can also configure it at run-time using methods from UUNACardboardVRStatics.
     */
    UPROPERTY(config, EditAnywhere, Category = "Native UI")
    bool bShowNativeUI = true;

    /**
     * Whether or not to show the back button in the Cardboard native UI.
     * When pressed, the following standard UE4 events will be triggered:
     *   - on Android: [`EKeys::Android_Back`](https://docs.unrealengine.com/en-US/API/Runtime/InputCore/EKeys/Android_Back/index.html) will be pressed and released;
     *   - on iOS: [`EKeys::Global_Back`](https://docs.unrealengine.com/en-US/API/Runtime/InputCore/EKeys/Global_Back/index.html) will be pressed and released.
     */
    UPROPERTY(config, EditAnywhere, Category = "Native UI", meta = (EditCondition = "bShowNativeUI"))
    bool bShowBackButton = false;

    /**
     * Whether or not to show the stereo mode toggle button in the Cardboard native UI
     * When pressed, the stereo rendering for HMD devices will be automatically toggled,
     * and the UUNACardboardVRSubsystem::OnStereoModeChanged delegate will be triggered.
     */
    UPROPERTY(config, EditAnywhere, Category = "Native UI", meta = (EditCondition = "bShowNativeUI"))
    bool bShowToggleStereoModeButton = true;

    /**
     * Whether or not to automatically open the QR scanner at the first launch of the app (i.e. if no previous viewer device parameters were already cached).
     * This procedure is required to properly setups the stereoscopy rendering as it retrieves the correct viewer device parameters.
     * Alternatively, you can manually call UUNACardboardVRStatics::ScanForQRViewerProfile() when desired.
     */
    UPROPERTY(config, EditAnywhere, Category = "QR Code")
    bool bAlwaysShowQrScannerFirstTime = true;

    /** 
     * Encoded device params to be loaded at startup (this is an alternative to the QRCode scanning to hardcode the device params in the app).
     * If not empty the system act as if #bAlwaysShowQrScannerFirstTime is false.
     * 
     * Encoded device params can be generated through the "viewer profile generator" (https://wwgc.firebaseapp.com/).
     * See UUNACardboardVRStatics::SaveDeviceParams for more details about the supported values.
     */
    UPROPERTY(config, EditAnywhere, Category = "Device Params")
    FString encodedDeviceParams;

    /**
     * Enable or not the *HMD Vignette* effect.
     */
    UPROPERTY(config, EditAnywhere, Category = "Rendering")
    bool bVignetteEnabled = true;

    /**
     * The *HMD Vignette* hardness. Larger values reduce the vignette effect.
     */
    UPROPERTY(config, EditAnywhere, Category = "Rendering", meta = (EditCondition = "bVignetteEnabled"))
    float VignetteHardness = 25.0f;
};
