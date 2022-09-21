// Copyright 2020 UNAmedia. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "UNACardboardVRStatics.generated.h"

/**
Static UNACardboardVR methods that can be called from both Blueprint and C++.

Some features (mainly *Delegates*) are available using the UUNACardboardVRSubsystem subsystem.
*/
UCLASS()
class UNACARDBOARDVR_API UUNACardboardVRStatics : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /**
     * Open the QR scanner to scan the viewer profile. At the end of the procedure device params are updated.
     */
    UFUNCTION(BlueprintCallable, Category="CardboardVR")
    static void ScanForQRViewerProfile();

    /**
     * Set the device parameters and saves them.
     * 
     * @param encodedDeviceParams The encoded device parameters.
     *   Encoded device params can be generated through the "viewer profile generator" (https://wwgc.firebaseapp.com/).
     *   They are usually a string similar to `CgZHb29nbGUSEkNhcmRib2FyZCBJL08gMjAxNR0rGBU9JQHegj0qEAAASEIAAEhCAABIQgAASEJYADUpXA89OggeZnc-Ej6aPlAAYAM`.
     *   If the input string is empty, the *Cardboard Viewer v1* default params will be loaded.
     *
     * @see SaveDeviceParamsFromURL()
     * @see UUNACardboardVRProjectSettings::encodedDeviceParams
     * @see [CardboardQrCode_saveDeviceParams()](https://github.com/googlevr/cardboard/blob/067049726eeb270ff6c537de02f2161135bd8eff/sdk/include/cardboard.h#L602)
     */
    UFUNCTION(BlueprintCallable, Category = "CardboardVR")
    static void SaveDeviceParams(const FString& encodedDeviceParams);

    /**
     * Set the device parameters from an input URL and saves them.
     * 
     * @param uri   An URI as documented by [CardboardQrCode_saveDeviceParams()](https://github.com/googlevr/cardboard/blob/067049726eeb270ff6c537de02f2161135bd8eff/sdk/include/cardboard.h#L602).
     * URI embedding custom device params can be generated through the "viewer profile generator" (https://wwgc.firebaseapp.com/).
     *
     * @see SaveDeviceParams()
     * @see [CardboardQrCode_saveDeviceParams()](https://github.com/googlevr/cardboard/blob/067049726eeb270ff6c537de02f2161135bd8eff/sdk/include/cardboard.h#L602)
     */
    UFUNCTION(BlueprintCallable, Category = "CardboardVR")
    static void SaveDeviceParamsFromURL(const FString& uri);

    /**
     * Gets currently saved device parameters.
     * 
     * @return The encoded device parameters, in a format compatible with SaveDeviceParams().
     * 
     * @see [CardboardQrCode_getSavedDeviceParams()](https://github.com/googlevr/cardboard/blob/067049726eeb270ff6c537de02f2161135bd8eff/sdk/include/cardboard.h#L565)
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CardboardVR")
    static FString GetSavedDeviceParams();

    /**
     * Gets Cardboard V1 device parameters.
     * 
     * @return The encoded device parameters, in a format compatible with SaveDeviceParams().
     * 
     * @see [CardboardQrCode_getCardboardV1DeviceParams()](https://github.com/googlevr/cardboard/blob/067049726eeb270ff6c537de02f2161135bd8eff/sdk/include/cardboard.h#L632)
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CardboardVR")
    static FString GetCardboardV1DeviceParams();

    /**
     * Recenters the head tracker.
     *
     * @see [CardboardHeadTracker_recenter()](https://github.com/googlevr/cardboard/blob/067049726eeb270ff6c537de02f2161135bd8eff/sdk/include/cardboard.h#L540)
     */
    UFUNCTION(BlueprintCallable, Category = "CardboardVR")
    static void RecenterHeadTracker();

    /**
     * Set a custom Offset Yaw angle that it's added to the head-tracked orientation.
     * 
     * @param OffsetYaw Yaw in degrees.
     */
    UFUNCTION(BlueprintCallable, Category = "CardboardVR")
    static void SetBaseOffsetYaw(float OffsetYaw);

    /**
     * Set the back button visibility in the native UI.
     * @note for the button to be visible, also the native UI must be visible (see SetNativeUIVisibility()) and the stereo mode enabled.
     */
    UFUNCTION(BlueprintCallable, Category = "CardboardVR|UI")
    static void SetBackButtonVisibility(bool bVisible);

    /**
     * Set the toggle stereo mode button visibility in the native UI.
     * @note for the toggle button to be visible, also the native UI must be visible (see SetNativeUIVisibility()).
     */
    UFUNCTION(BlueprintCallable, Category = "CardboardVR|UI")
    static void SetToggleStereoModeButtonVisibity(bool bVisible);

    /**
     * Set the native overlay UI visibility.
     * @param bVisible If `true`, then the Cardboard overlay UI is shown on screen. Its parts can be configured through
     *  the dedicated methods (see SetToggleStereoModeButtonVisibity() and SetBackButtonVisibility()).
     */
    UFUNCTION(BlueprintCallable, Category = "CardboardVR|UI")
    static void SetNativeUIVisibility(bool bVisible);
};
