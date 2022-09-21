// Copyright 2020 UNAmedia. All rights reserved.

#include "UNACardboardVRStatics.h"

#include "HeadMountedDisplayFunctionLibrary.h"

#include "UNACardboardVRDevice.h"

void UUNACardboardVRStatics::ScanForQRViewerProfile()
{
    FUNACardboardVRDevice::GetInstance().ScanForQRViewerProfile();
}

void UUNACardboardVRStatics::SaveDeviceParams(const FString& encodedDeviceParams)
{
    FUNACardboardVRDevice::GetInstance().SaveDeviceParams(encodedDeviceParams);
}

void UUNACardboardVRStatics::SaveDeviceParamsFromURL(const FString& uri)
{
    FUNACardboardVRDevice::GetInstance().SaveDeviceParamsFromURL(uri);
}

FString UUNACardboardVRStatics::GetSavedDeviceParams()
{
    return FUNACardboardVRDevice::GetInstance().GetSavedDeviceParams();
}

FString UUNACardboardVRStatics::GetCardboardV1DeviceParams()
{
    return FUNACardboardVRDevice::GetInstance().GetCardboardV1DeviceParams();
}

void UUNACardboardVRStatics::RecenterHeadTracker()
{
    UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void UUNACardboardVRStatics::SetBaseOffsetYaw(float OffsetYaw)
{
    if (GEngine->XRSystem.IsValid() && GEngine->XRSystem->IsHeadTrackingAllowed())
    {
        GEngine->XRSystem->SetBaseRotation(FRotator(0.0f, OffsetYaw, 0.0f));
    }
}

void UUNACardboardVRStatics::SetBackButtonVisibility(bool bVisible)
{
    FUNACardboardVRDevice::GetInstance().SetBackButtonVisibility(bVisible);
}

void UUNACardboardVRStatics::SetToggleStereoModeButtonVisibity(bool bVisible)
{
    FUNACardboardVRDevice::GetInstance().SetToggleStereoModeButtonVisibity(bVisible);
}

void UUNACardboardVRStatics::SetNativeUIVisibility(bool bVisible)
{
    FUNACardboardVRDevice::GetInstance().SetNativeUIVisibility(bVisible);
}
