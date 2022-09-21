// Copyright 2020 UNAmedia. All rights reserved.

#include "UNACardboardVRDevice.h"

#include "UNACardboardVRSubsystem.h"
#include "UNACardboardVRProjectSettings.h"
#include "UNACardboardVRPrivate.h"

#include "GameFramework/PlayerController.h"

#include "Misc/Base64.h"

#include "cardboard.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include <jni.h>
#elif PLATFORM_IOS
#	include "IOS/IOSAppDelegate.h"
#	include "IOS/IOSView.h"

#	include "ios/UNACardboardOverlayView.h"
#endif // PLATFORM_ANDROID

#if PLATFORM_ANDROID || PLATFORM_IOS
#define CARDBOARD_SDK_ENABLED 1
#else
//#define CARDBOARD_SDK_ENABLED 1 // just for development
#endif // PLATFORM_ANDROID || PLATFORM_IOS

static const TCHAR* SettingSectionLabel = TEXT("Cardboard");

FUNACardboardVRDevice::FUNACardboardVRDevice()
#if PLATFORM_IOS
	: OverlayView(nil)
#endif
{
}

FUNACardboardVRDevice& FUNACardboardVRDevice::GetInstance()
{
    static FUNACardboardVRDevice Singleton;
    return Singleton;
}

void FUNACardboardVRDevice::Initialize()
{
#if DO_CHECK
	check(! bIsInitialized);
	bIsInitialized = true;
#endif

#ifdef CARDBOARD_SDK_ENABLED
    qrCodeScanCount = CardboardQrCode_getDeviceParamsChangedCount();

    // if saved device params doesn't match with hardcoded device params, overwrite them.
    {
        const FString& encodedDeviceParams = GetDefault<UUNACardboardVRProjectSettings>()->encodedDeviceParams;

        uint8_t* buffer;
        int size;
        CardboardQrCode_getSavedDeviceParams(&buffer, &size);

       // FUTF8ToTCHAR wide_buffer((const ANSICHAR*)buffer);
       // FString savedDeviceParams(wide_buffer.Length(), wide_buffer.Get());
        FString savedDeviceParams = FBase64::Encode(buffer, size);

        UE_LOG(LogUNACardboardVR, Display, TEXT("Check device params: %s --> %s"), *savedDeviceParams, *encodedDeviceParams);

        if (savedDeviceParams != encodedDeviceParams)
        {
            UE_LOG(LogUNACardboardVR, Display, TEXT("Overwrite device params: %s --> %s"), *savedDeviceParams, *encodedDeviceParams);

            SaveDeviceParams(encodedDeviceParams);
        }
    }
#endif // CARDBOARD_SDK_ENABLED

#if PLATFORM_IOS
	// Setup the native UI overlay on iOS. Showing it is controlled by EnableStereo().
	dispatch_async(
		dispatch_get_main_queue(),
		^{
			//OverlayViewDelegate = [[UNACardboardOverlayView alloc] init];
			NSBundle *bundle = [NSBundle bundleWithPath:[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"UNACardboard.bundle"]];
        	OverlayView = [[bundle loadNibNamed:@"UNACardboardOverlayView" owner:nil options:nil] lastObject];
			NSCAssert(OverlayView != nil, @"UNACardboardOverlayView not found in UNACardboard.bundle");
			//OverlayView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
			[OverlayView setBackgroundColor:[UIColor clearColor]];
			[OverlayView setOpaque:NO];
			//OverlayView.delegate = OverlayViewDelegate;
			OverlayView.frame = [IOSAppDelegate GetDelegate].IOSView.bounds;

			OverlayView.BackButtonCallback = ^(){
				HandleBackEvent();
			};
			OverlayView.SettingsMenuOpenCallback = ^(){
				HandleSettingsMenuOpenEvent();
			};
			OverlayView.StereoModeToggleButtonCallback = ^(){
				ToggleStereoRendering();
			};
			
			[[IOSAppDelegate GetDelegate].IOSView addSubview : OverlayView];
		}
	);
#endif // PLATFORM_IOS
}

void FUNACardboardVRDevice::Deinitialize()
{
#if DO_CHECK
	check(bIsInitialized);
	bIsInitialized = false;
#endif

#if PLATFORM_IOS
	dispatch_async(
		dispatch_get_main_queue(),
		^{
			OverlayView.BackButtonCallback = nil;
			OverlayView.SettingsMenuOpenCallback = nil;
			OverlayView.StereoModeToggleButtonCallback = nil;

			[OverlayView removeFromSuperview];
			OverlayView = nil;
		}
	);
#endif // PLATFORM_IOS
}

bool FUNACardboardVRDevice::GetAndThenReset_IsDeviceParamsChanged()
{
    int currentCount =
#ifdef CARDBOARD_SDK_ENABLED
        CardboardQrCode_getDeviceParamsChangedCount();
#else
        0;
#endif // CARDBOARD_SDK_ENABLED

    bool bDeviceParamsChanged = currentCount != qrCodeScanCount;

    qrCodeScanCount = currentCount;
    return bDeviceParamsChanged;
}

bool FUNACardboardVRDevice::ShouldBeNativeUIVisible() const
{
    //if (!IsStereoEnabled())
    //    return false;

    return GetDefault<UUNACardboardVRProjectSettings>()->bShowNativeUI;
}

bool FUNACardboardVRDevice::ShouldBeSettingsButtonVisible() const
{
    //if (!IsStereoEnabled())
    //    return false;

    return true;
}

bool FUNACardboardVRDevice::ShouldBeBackButtonVisible() const
{
    //if (!IsStereoEnabled())
    //    return false;

    return GetDefault<UUNACardboardVRProjectSettings>()->bShowBackButton;
}

bool FUNACardboardVRDevice::ShouldBeToggleStereoModeButtonVisible() const
{
    //if (!IsStereoEnabled())
    //    return false;

    return GetDefault<UUNACardboardVRProjectSettings>()->bShowToggleStereoModeButton;
}

void FUNACardboardVRDevice::OnApplicationCreated()
{
    UE_LOG(LogUNACardboardVR, Display, TEXT("FUNACardboardVRDevice::OnApplicationCreated"));

#if PLATFORM_ANDROID
    check(IsInGameThread());

    JNIEnv* env = FAndroidApplication::GetJavaEnv();
    if (env == nullptr)
        return;

    JavaVM* jvm;
    env->GetJavaVM(&jvm);

    if (jvm == nullptr)
        return;

    jobject activity = FAndroidApplication::GetGameActivityThis();
    if (activity == NULL)
        return;

    UE_LOG(LogUNACardboardVR, Display, TEXT("Android initialization: %d, %d, %d"), env, jvm, activity);

    Cardboard_initializeAndroid(jvm, activity);
#endif // PLATFORM_ANDROID
}

void FUNACardboardVRDevice::HandleQRScanningAutomatically()
{
#ifdef CARDBOARD_SDK_ENABLED
    // the app must be in stereo
    if (!IsStereoEnabled())
        return;

    // bAlwaysShowQrScannerFirstTime must be true...
    if (!GetDefault<UUNACardboardVRProjectSettings>()->bAlwaysShowQrScannerFirstTime)
        return;

    // and encodedDeviceParams must be empty
    if (!GetDefault<UUNACardboardVRProjectSettings>()->encodedDeviceParams.IsEmpty())
        return;
    
    // Check for device parameters existence in external storage. If they're
    // missing, we must scan a Cardboard QR code and save the obtained parameters.
    uint8_t* buffer;
    int size;
    CardboardQrCode_getSavedDeviceParams(&buffer, &size);
    if (size == 0)
    {
        // As fallback if the app has no permissions to access the camera, save
        // the Cardboard V1 parameters as default. This will avoid entering here again.
        //
        // Indeed, the next call to ScanForQRViewerProfile() could ask the user for permission
        // to access the camera. If it's not granted, every time we'll run the app
        // we'll continuosly enter here, as no device params would be saved.
        // The main problem is that the current Google implementation redirects the
        // user to the Operating System settings view, to let the user grant the permission.
        // This is not a great UX, as the user is moved away from the application.
        // An error message with buttons would be better
        // (e.g. "camera permission required", with buttons Settings/Cancel).
        // Note that this bad UX would still happen on first run if the user deny the permission.
        SaveDeviceParams(GetCardboardV1DeviceParams());
        ScanForQRViewerProfile();
    }
    CardboardQrCode_destroy(buffer);
#endif // CARDBOARD_SDK_ENABLED
}

void FUNACardboardVRDevice::ScanForQRViewerProfile()
{
	check(bIsInitialized);
    UE_LOG(LogUNACardboardVR, Display, TEXT("FUNACardboardVRDevice::ScanForQRViewerProfile"));

#ifdef CARDBOARD_SDK_ENABLED
#	if PLATFORM_IOS
	// On iOS, all UI stuff MUST run in the main thread.
	// ATTENTION: ENamedThreads::GameThread is *not* the iOS main thread.
	dispatch_async(dispatch_get_main_queue(), ^{
#	endif // PLATFORM_IOS

		CardboardQrCode_scanQrCodeAndSaveDeviceParams();

#	if PLATFORM_IOS
	});
#	endif // PLATFORM_IOS
#endif // CARDBOARD_SDK_ENABLED
}

void FUNACardboardVRDevice::SaveDeviceParams(const FString& encodedDeviceParams)
{
    check(bIsInitialized);
    UE_LOG(LogUNACardboardVR, Display, TEXT("FUNACardboardVRDevice::SaveDeviceParams"));

#ifdef CARDBOARD_SDK_ENABLED
    FString uri;
    if (encodedDeviceParams.IsEmpty())
        uri = "https://g.co/cardboard";
    else
        uri = "https://google.com/cardboard/cfg?p=" + encodedDeviceParams;

    FTCHARToUTF8 uri_utf8(*uri);
    CardboardQrCode_saveDeviceParams((uint8*)uri_utf8.Get(), uri_utf8.Length());
#endif // CARDBOARD_SDK_ENABLED
}

void FUNACardboardVRDevice::SaveDeviceParamsFromURL(const FString& uri)
{
    check(bIsInitialized);
    UE_LOG(LogUNACardboardVR, Display, TEXT("FUNACardboardVRDevice::SaveDeviceParamsFromURL"));

#ifdef CARDBOARD_SDK_ENABLED
    FTCHARToUTF8 uri_utf8(*uri);
    CardboardQrCode_saveDeviceParams((uint8*)uri_utf8.Get(), uri_utf8.Length());
#endif // CARDBOARD_SDK_ENABLED
}

FString FUNACardboardVRDevice::GetSavedDeviceParams() const
{
    check(bIsInitialized);

#ifdef CARDBOARD_SDK_ENABLED
    uint8_t *buffer;
    int size;
    CardboardQrCode_getSavedDeviceParams(&buffer, &size);

    FString encodedParams = FBase64::Encode(buffer, size);
    encodedParams.ReplaceCharInline(L'/', L'_');
    encodedParams.ReplaceCharInline(L'+', L'-');

    CardboardQrCode_destroy(buffer);

    return encodedParams;
#else
    return "";
#endif // CARDBOARD_SDK_ENABLED
}

FString FUNACardboardVRDevice::GetCardboardV1DeviceParams() const
{
#ifdef CARDBOARD_SDK_ENABLED
    uint8_t *buffer;
    int size;
    CardboardQrCode_getCardboardV1DeviceParams(&buffer, &size);

    FString encodedParams = FBase64::Encode(buffer, size);
    encodedParams.ReplaceCharInline(L'/', L'_');
    encodedParams.ReplaceCharInline(L'+', L'-');

    return encodedParams;
#else
    return "";
#endif // CARDBOARD_SDK_ENABLED
}

void FUNACardboardVRDevice::ToggleStereoRendering()
{
    if (GEngine && GEngine->StereoRenderingDevice.IsValid())
    {
        const bool IsStereoEnabled = GEngine->StereoRenderingDevice->IsStereoEnabled();        
        const bool bResult = GEngine->StereoRenderingDevice->EnableStereo(!IsStereoEnabled);
        const bool bChanged = bResult != IsStereoEnabled;

        if (bChanged)
        {
            UUNACardboardVRSubsystem::GetInstance(GetWorld()).OnStereoModeChanged.Broadcast(bResult);
        }
    }
    else
    {
        UE_LOG(LogUNACardboardVR, Warning, TEXT("Warning - unable to ToggleStereoRendering because StereoRenderingDevice is not valid"));
    }
}

void FUNACardboardVRDevice::HandleNativeUI(bool bStereoEnabled)
{
	check(bIsInitialized);
    const bool bVisible = ShouldBeNativeUIVisible();
    const bool bSettingsButtonVisible = ShouldBeSettingsButtonVisible();
    const bool bBackButtonVisible = ShouldBeBackButtonVisible();
    const bool bToggleStereoModeButtonVisible = ShouldBeToggleStereoModeButtonVisible();

    UE_LOG(LogUNACardboardVR, Display, TEXT("FUNACardboardVRDevice::HandleNativeUI - Stereo=%d Visible=%d SettingsButton=%d BackButton=%d ToggleStereoButton=%d"), 
        bStereoEnabled,
        bVisible,
        bSettingsButtonVisible,
        bBackButtonVisible,
        bToggleStereoModeButtonVisible);

#if PLATFORM_ANDROID
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        static jmethodID UiLayerMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_SetupUILayer", "(ZZZZZ)V", false);
        FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, UiLayerMethod, bVisible, bStereoEnabled, bSettingsButtonVisible, bBackButtonVisible, bToggleStereoModeButtonVisible);
    }
#elif PLATFORM_IOS
	dispatch_async(
		dispatch_get_main_queue(),
		^{
			[OverlayView updateUIStateWithVisibility: bVisible
				IsStereoEnabled: bStereoEnabled
    			IsSettingsButtonVisible: bSettingsButtonVisible
				IsBackButtonVisible: bBackButtonVisible
				IsToggleStereoModeButtonVisible: bToggleStereoModeButtonVisible];
		}
	);
#elif defined(CARDBOARD_SDK_ENABLED)
#   error missing implementation
#endif
}

bool FUNACardboardVRDevice::IsStereoEnabled() const
{
    bool bIsStereoModeEnabled = false;

    if (GEngine && GEngine->StereoRenderingDevice.IsValid())
    {
        bIsStereoModeEnabled = GEngine->StereoRenderingDevice->IsStereoEnabled();
    }
    else
    {
        UE_LOG(LogUNACardboardVR, Warning, TEXT("Warning - unable to query IsStereoEnabled because StereoRenderingDevice is not valid, false is returned"));
    }

    return bIsStereoModeEnabled;
}

UWorld* FUNACardboardVRDevice::GetWorld() const
{
    if (GEngine && GEngine->GameViewport)
    {
        return GEngine->GameViewport->GetWorld();
    }

    return nullptr;
}

void FUNACardboardVRDevice::HandleBackEvent()
{
    UE_LOG(LogUNACardboardVR, Display, TEXT("FUNACardboardVRDevice::HandleBackEvent"));

    UWorld* World = GetWorld();
    APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
    if (!PlayerController)
    {
        return;
    }

#if PLATFORM_ANDROID
    PlayerController->InputKey(EKeys::Android_Back, EInputEvent::IE_Pressed, 1.0f, false);
    PlayerController->InputKey(EKeys::Android_Back, EInputEvent::IE_Released, 0.0f, false);
#elif PLATFORM_IOS
    PlayerController->InputKey(EKeys::Global_Back, EInputEvent::IE_Pressed, 1.0f, false);
    PlayerController->InputKey(EKeys::Global_Back, EInputEvent::IE_Released, 0.0f, false);
#elif defined(CARDBOARD_SDK_ENABLED)
#   error missing implementation
#endif
}

void FUNACardboardVRDevice::HandleSettingsMenuOpenEvent()
{
    UUNACardboardVRSubsystem::GetInstance(GetWorld()).OnSettingsMenuOpened.Broadcast();
}

void FUNACardboardVRDevice::SetBackButtonVisibility(bool bVisible)
{
    GetMutableDefault<UUNACardboardVRProjectSettings>()->bShowBackButton = bVisible;

    HandleNativeUI(IsStereoEnabled());
}

void FUNACardboardVRDevice::SetToggleStereoModeButtonVisibity(bool bVisible)
{
    GetMutableDefault<UUNACardboardVRProjectSettings>()->bShowToggleStereoModeButton = bVisible;

    HandleNativeUI(IsStereoEnabled());
}

void FUNACardboardVRDevice::SetNativeUIVisibility(bool bVisible)
{
    GetMutableDefault<UUNACardboardVRProjectSettings>()->bShowNativeUI = bVisible;

    HandleNativeUI(IsStereoEnabled());
}
