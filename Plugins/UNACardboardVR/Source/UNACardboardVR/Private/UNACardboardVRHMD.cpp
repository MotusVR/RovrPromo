// Copyright 2020 UNAmedia. All rights reserved.

#include "UNACardboardVRHMD.h"
#include "Misc/App.h"
#include "Modules/ModuleManager.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Runtime/Renderer/Private/RendererPrivate.h"
#include "Runtime/Renderer/Private/ScenePrivate.h"
#include "Runtime/Renderer/Private/SceneRendering.h"
#include "Runtime/Renderer/Private/PostProcess/PostProcessHMD.h"
#include "GameFramework/WorldSettings.h"

#include "UNACardboardVRDevice.h"
#include "UNACardboardVRProjectSettings.h"
#include "UNACardboardVRPrivate.h"

#include "HAL/PlatformApplicationMisc.h"

#include <array>

#define CARDBOARD_VERBOSE_LOGGING 0

#if PLATFORM_ANDROID
#   include "Android/AndroidJNI.h"
#   include "Android/AndroidApplication.h"
#elif PLATFORM_IOS
#   include "Application/SlateApplicationBase.h"
#   include "IOS/IOSAppDelegate.h"
#endif // PLATFORM_ANDROID

#if PLATFORM_ANDROID || PLATFORM_IOS
#   include <time.h>
#   include <unistd.h>
#   define CARDBOARD_SDK_ENABLED 1
#else
//#define CARDBOARD_SDK_ENABLED 1 // just for development
#endif // PLATFORM_ANDROID || PLATFORM_IOS

static TAutoConsoleVariable<float> CVarPreviewSensitivity(
    TEXT("vr.carboard.PreviewSensitivity"),
    1.0f,
    TEXT("Change preview sensitivity of Yaw and Pitch multiplier.\n")
    TEXT("Values are clamped between 0.1 and 10.0\n")
);

static constexpr uint64_t kNanosInSeconds = 1000000000;

int64_t GetBootTimeNano() {
#if PLATFORM_ANDROID
    struct timespec res;
    clock_gettime(CLOCK_BOOTTIME, &res);
    return (res.tv_sec * kNanosInSeconds) + res.tv_nsec;
#elif PLATFORM_IOS
    struct timespec res;
    clock_gettime(CLOCK_UPTIME_RAW, &res);
    return (res.tv_sec * kNanosInSeconds) + res.tv_nsec;
#elif defined(CARDBOARD_SDK_ENABLED)
#   error missing implementation
#else
    return 0;
#endif
}

// Returns the hardware screen size in pixels of the rendered area.
// This is needed by the Google Cardboard SDK to compute the physical
// dimensions of the rendered viewport on screen.
FIntPoint GetScreenSize()
{
    FIntPoint Size = FIntPoint::ZeroValue;
#if PLATFORM_ANDROID
    //FPlatformRect Rect = FAndroidWindow::GetScreenRect();
    //Size.X = Rect.Right;
    //Size.Y = Rect.Bottom;
    FAndroidWindow::CalculateSurfaceSize(Size.X, Size.Y, false);
#elif PLATFORM_IOS
    //FSlateRect Rect = FSlateApplicationBase::Get().GetPreferredWorkArea();
	//FPlatformRect Rect = FSlateApplicationBase::Get().GetPlatformApplication()->GetWorkArea(FPlatformRect());
	// Adjust main window frame to not display on top of notch if exists.
	//
	// This replicates the implementation of "hellocardboard-ios" from the Google Cardboard SDK
	// (see https://github.com/googlevr/cardboard/blob/4382d959a62196405ab75a237b5c39094a137c6e/hellocardboard-ios/HelloCardboardAppDelegate.m#L41 ):
	// the app is rendered using all the available screen space but avoiding any safe area on the left side.
	//
	// See also DrawDistortionMesh_RenderThread().
	//
	// The safe-area insets should be available in UE4 through TitleSafePaddingSize and ActionSafePaddingSize,
	// see: 
	//   - https://github.com/EpicGames/UnrealEngine/blob/5df54b7ef1714f28fb5da319c3e83d96f0bedf08/Engine/Source/Runtime/ApplicationCore/Public/GenericPlatform/GenericApplication.h#L383
	//	 - https://github.com/EpicGames/UnrealEngine/blob/2bf1a5b83a7076a0fd275887b373f8ec9e99d431/Engine/Source/Runtime/ApplicationCore/Private/IOS/IOSApplication.cpp#L185
	// For example:
	//   FDisplayMetrics Metrics;
	//   FSlateApplicationBase::Get().GetDisplayMetrics(Metrics);
	//   Metrics.TitleSafePaddingSize ...
	// if (@available(iOS 11, *)) {...}
	const UIEdgeInsets safeInsets = UIApplication.sharedApplication.keyWindow.safeAreaInsets;
	// NOTE: nativeBounds are always relative to portrait.
	Size.X = UIScreen.mainScreen.nativeBounds.size.height - safeInsets.left * UIScreen.mainScreen.nativeScale;
	Size.Y = UIScreen.mainScreen.nativeBounds.size.width;
#elif defined(CARDBOARD_SDK_ENABLED)
#   error missing implementation
#endif
    return Size;
}

FVector ConvertCardboardVectorToUnreal(float x, float y, float z, float WorldToMetersScale)
{
    FVector Result;

    // Cardboard: Negative Z is Forward, UE: Positive X is Forward.
    Result.X = -z * WorldToMetersScale;
    // Cardboard: Positive X is Right, UE: Positive Y is Right.
    Result.Y = x * WorldToMetersScale;
    // Cardboard: Positive Y is Up, UE: Positive Z is Up
    Result.Z = y * WorldToMetersScale;

    return Result;
}

//@see https://github.com/googlevr/cardboard/blob/22983a5bfb6a8f6c78b41e1f44ffb0fe9a1dc64a/sdk/unity/xr_provider/math_tools.cc#L111
FQuat ConvertCardboardQuaternionToUnreal(float w, float x, float y, float z)
{   
    // Convert Gvr right handed coordinate system rotation into UE left handed coordinate system.
    FQuat Result = FQuat(-z, x, y, w);
    return Result;
}

FMatrix ConvertCardboardMatrixToUnreal(float* matrixArray, float WorldToMetersScale)
{
    // https://github.com/EpicGames/UnrealEngine/blob/f8f4b403eb682ffc055613c7caf9d2ba5df7f319/Engine/Plugins/Runtime/AR/Google/GoogleARCore/Source/GoogleARCoreBase/Private/GoogleARCoreAPI.cpp#L19
    static const FMatrix OGLToUnrealTransform = FMatrix(
		FPlane(0.0f, 0.0f, -1.0f, 0.0f),
		FPlane(1.0f, 0.0f, 0.0f, 0.0f),
		FPlane(0.0f, 1.0f, 0.0f, 0.0f),
		FPlane(0.0f, 0.0f, 0.0f, 1.0f)
    );
    static const FMatrix OGLToUnrealTransformInverse = OGLToUnrealTransform.InverseFast();
    // https://github.com/EpicGames/UnrealEngine/blob/f8f4b403eb682ffc055613c7caf9d2ba5df7f319/Engine/Plugins/Runtime/AR/Google/GoogleARCore/Source/GoogleARCoreBase/Private/GoogleARCoreAPI.cpp#L57
    FMatrix InputMatrix;
    memcpy(InputMatrix.M, matrixArray, 16 * sizeof(float));
    FMatrix Result = OGLToUnrealTransform * InputMatrix * OGLToUnrealTransformInverse;
    Result.SetOrigin(Result.GetOrigin() * WorldToMetersScale);
    //FTransform Result = FTransform(OGLToUnrealTransform * InputMatrix * OGLToUnrealTransformInverse);
    //Result.SetLocation(Result.GetLocation() * WorldToMeterScale);
    return Result;
}

void FUNACardboardVRHMD::ConvertCardboardMeshToUnrealDistorsionMesh(
    const CardboardMesh& InCardboardMesh, 
    FDistorsionMesh& OutMesh)
{
    const UUNACardboardVRProjectSettings* Settings = GetDefault<UUNACardboardVRProjectSettings>();
    const bool bVignetteEnabled = Settings->bVignetteEnabled;
    const float kVignetteHardness = Settings->VignetteHardness;

    OutMesh.Vertices.SetNumUninitialized(InCardboardMesh.n_vertices);
    for (int i = 0; i < InCardboardMesh.n_vertices; ++i)
    {
        FDistortionVertex& DestV = OutMesh.Vertices[i];

        float x = InCardboardMesh.vertices[i * 2];
        float y = -InCardboardMesh.vertices[i * 2 + 1]; // Y is flipped!

#if PLATFORM_IOS && HAS_METAL
        y *= -1.0f;
#endif // PLATFORM_IOS && HAS_METAL

        DestV.Position = { x, y };

        float u = InCardboardMesh.uvs[i * 2];
        float v = InCardboardMesh.uvs[i * 2 + 1];

#if PLATFORM_IOS && HAS_METAL
        v = 1.0f - v;
#endif // PLATFORM_IOS && HAS_METAL
        
        FVector2D XYNorm = { u, v };
        float Vignette = FMath::Clamp(XYNorm.X * kVignetteHardness, 0.0f, 1.0f)
            * FMath::Clamp((1 - XYNorm.X)* kVignetteHardness, 0.0f, 1.0f)
            * FMath::Clamp(XYNorm.Y * kVignetteHardness, 0.0f, 1.0f)
            * FMath::Clamp((1 - XYNorm.Y)* kVignetteHardness, 0.0f, 1.0f);

        DestV.TexB = DestV.TexG = DestV.TexR = { u, v };
        DestV.VignetteFactor = bVignetteEnabled ? Vignette : 1.0f;
        DestV.TimewarpFactor = 0.0f;
    }

    auto& Indices = OutMesh.Indices;

    // triangle strip to triangle list...
    Indices.Empty();
    Indices.Reserve((InCardboardMesh.n_indices - 2) * 3);
    for (int i = 0; i < InCardboardMesh.n_indices - 2; ++i)
    {
        Indices.Add(InCardboardMesh.indices[i]);
        Indices.Add(InCardboardMesh.indices[i + 1]);
        Indices.Add(InCardboardMesh.indices[i + 2]);
    }
}

//---------------------------------------------------
// FUNACardboardVRHMD Implementation
//---------------------------------------------------

float FUNACardboardVRHMD::GetWorldToMetersScale() const
{
    return WorldToMeters;
}

bool FUNACardboardVRHMD::IsHMDEnabled() const
{
    return true;
}

void FUNACardboardVRHMD::EnableHMD(bool enable)
{
    // no-op
}

bool FUNACardboardVRHMD::GetHMDMonitorInfo(MonitorInfo& MonitorDesc)
{
    if (!IsStereoEnabled())
    {
        return false;
    }

	MonitorDesc.MonitorName = "UNACardboardVR";
	MonitorDesc.MonitorId = 0;
	MonitorDesc.DesktopX = MonitorDesc.DesktopY = MonitorDesc.ResolutionX = MonitorDesc.ResolutionY = 0;
    MonitorDesc.WindowSizeX = MonitorDesc.WindowSizeY = 0;

#if PLATFORM_ANDROID
    FPlatformRect Rect = FAndroidWindow::GetScreenRect();
    MonitorDesc.ResolutionX = Rect.Right - Rect.Left;
    MonitorDesc.ResolutionY = Rect.Bottom - Rect.Top;
    return true;
#elif PLATFORM_IOS
    FPlatformRect Rect = FSlateApplicationBase::Get().GetPlatformApplication()->GetWorkArea(FPlatformRect());
	MonitorDesc.ResolutionX = Rect.Right - Rect.Left;
	MonitorDesc.ResolutionY = Rect.Bottom - Rect.Top;
    return true;
#elif defined(CARDBOARD_SDK_ENABLED)
#   error missing implementation
#else
	return false;
#endif
}

void FUNACardboardVRHMD::GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const
{
	checkf(false, TEXT("To be implemented"));
	OutHFOVInDegrees = 0.0f;
	OutVFOVInDegrees = 0.0f;
}

void FUNACardboardVRHMD::OnBeginPlay(FWorldContext& InWorldContext)
{
    SetPlayerControllerTouchInterfaceEnabled(!IsStereoEnabled());
}

void FUNACardboardVRHMD::OnEndPlay(FWorldContext& InWorldContext)
{
}

bool FUNACardboardVRHMD::EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type)
{
	if (Type == EXRTrackedDeviceType::Any || Type == EXRTrackedDeviceType::HeadMountedDisplay)
	{
		OutDevices.Add(IXRTrackingSystem::HMDDeviceId);
		return true;
	}
	return false;
}

void FUNACardboardVRHMD::SetInterpupillaryDistance(float NewInterpupillaryDistance)
{
}

float FUNACardboardVRHMD::GetInterpupillaryDistance() const
{
#ifdef CARDBOARD_SDK_ENABLED
    // For simplicity, the interpupillary distance is the distance to the left eye, doubled.
    FQuat Unused;
    FVector Offset;

    GetRelativeHMDEyePose(EStereoscopicPass::eSSP_LEFT_EYE, Unused, Offset);
    return Offset.Size() * 2.0f;
#else
	return 0.064f * GetWorldToMetersScale();
#endif // CARDBOARD_SDK_ENABLED
}

bool FUNACardboardVRHMD::GetRelativeEyePose(int32 DeviceId, EStereoscopicPass Eye, FQuat& OutOrientation, FVector& OutPosition)
{
    if (DeviceId != IXRTrackingSystem::HMDDeviceId || !(Eye == eSSP_LEFT_EYE || Eye == eSSP_RIGHT_EYE))
    {
        return false;
    }
    else
    {
        return GetRelativeHMDEyePose(Eye, OutOrientation, OutPosition);
    }
}

bool FUNACardboardVRHMD::GetRelativeHMDEyePose(EStereoscopicPass Eye, FQuat& OutOrientation, FVector& OutPosition) const
{
#ifdef CARDBOARD_SDK_ENABLED
    const FMatrix& EyeMat = eyeMatrices[(Eye == eSSP_LEFT_EYE ? 0 : 1)];
    OutPosition = -EyeMat.GetOrigin();
    OutOrientation = EyeMat.ToQuat();

    return true;
#else
    OutPosition = FVector(0, (Eye == eSSP_LEFT_EYE ? .5 : -.5) * GetInterpupillaryDistance(), 0);
    OutOrientation = FQuat::Identity;
    return true;
#endif // CARDBOARD_SDK_ENABLED
}

void FUNACardboardVRHMD::GetHMDPose(FVector& CurrentPosition, FQuat& CurrentOrientation)
{
#ifdef CARDBOARD_SDK_ENABLED
	if (HeadTracker != nullptr)
	{
        //Default prediction excess time in nano seconds.
        static constexpr int64_t kPredictionTimeWithoutVsyncNanos = 50000000;

        std::array<float, 4> out_orientation;
        std::array<float, 3> out_position;

        CardboardHeadTracker_getPose(HeadTracker, GetBootTimeNano() + kPredictionTimeWithoutVsyncNanos, kLandscapeLeft, &out_position[0], &out_orientation[0]);

        //UE_LOG(LogUNACardboardVR, Display, TEXT("[FUNACardboardVRHMD::GetHMDPose] sdk-pos: %f %f %f"), out_position[0], out_position[1], out_position[2]);
        //UE_LOG(LogUNACardboardVR, Display, TEXT("[FUNACardboardVRHMD::GetHMDPose] sdk-ori: %f %f %f %f"), out_orientation[0], out_orientation[1], out_orientation[2], out_orientation[3]);

        //@see https://github.com/googlevr/cardboard/blob/22983a5bfb6a8f6c78b41e1f44ffb0fe9a1dc64a/sdk/unity/xr_provider/math_tools.cc
        CurrentPosition = ConvertCardboardVectorToUnreal(out_position[0], out_position[1], out_position[2], WorldToMeters);
        CurrentOrientation = ConvertCardboardQuaternionToUnreal(out_orientation[3], out_orientation[0], out_orientation[1], out_orientation[2]);

        //UE_LOG(LogUNACardboardVR, Display, TEXT("[FUNACardboardVRHMD::GetHMDPose] ue4-pos: %s"), *CurrentPosition.ToString());
        //UE_LOG(LogUNACardboardVR, Display, TEXT("[FUNACardboardVRHMD::GetHMDPose] ue4-ori: %s"), *CurrentOrientation.ToString());

        // NOTE: it seems Cardboard SDK is supporting only the UIDeviceOrientationLandscapeLeft device orientation
        // (The device is in landscape mode, with the device held upright and the home button on the right side.)
        // https://github.com/googlevr/cardboard/blob/22983a5bfb6a8f6c78b41e1f44ffb0fe9a1dc64a/sdk/head_tracker.cc#L82
	}
	else
#endif // CARDBOARD_SDK_ENABLED
	{
        // Simulating head rotation using mouse in Editor.
        ULocalPlayer* Player = GEngine->GetDebugLocalPlayer();
        float PreviewSensitivity = FMath::Clamp(CVarPreviewSensitivity.GetValueOnAnyThread(), 0.1f, 10.0f);

        if (Player != NULL && Player->PlayerController != NULL && GWorld)
        {
            float MouseX = 0.0f;
            float MouseY = 0.0f;
            Player->PlayerController->GetInputMouseDelta(MouseX, MouseY);

            double CurrentTime = FApp::GetCurrentTime();
            double DeltaTime = GWorld->GetDeltaSeconds();

            PoseYaw += (FMath::RadiansToDegrees(MouseX * DeltaTime * 4.0f) * PreviewSensitivity);
            PosePitch += (FMath::RadiansToDegrees(MouseY * DeltaTime * 4.0f) * PreviewSensitivity);
            PosePitch = FMath::Clamp(PosePitch, -90.0f + KINDA_SMALL_NUMBER, 90.0f - KINDA_SMALL_NUMBER);

            CurrentPosition = FVector::ZeroVector;
            CurrentOrientation = BaseOrientation * FQuat(FRotator(PosePitch, PoseYaw, 0.0f));
        }
        else
        {
            CurrentPosition = FVector::ZeroVector;
            CurrentOrientation = FQuat(FRotator::ZeroRotator);
        }
	}
}

bool FUNACardboardVRHMD::GetCurrentPose(int32 DeviceId, FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	if (DeviceId != IXRTrackingSystem::HMDDeviceId)
	{
		return false;
	}

	GetHMDPose(CurrentPosition, CurrentOrientation);

    // apply the BaseOrientation to the HMD pose.
    CurrentOrientation = BaseOrientation * CurrentOrientation;
    CurrentPosition = BaseOrientation.RotateVector(CurrentPosition);

	return true;
}

bool FUNACardboardVRHMD::IsChromaAbCorrectionEnabled() const
{
	return false;
}

void FUNACardboardVRHMD::ResetOrientationAndPosition(float yaw)
{
	ResetOrientation(yaw);
	ResetPosition();
}

void FUNACardboardVRHMD::ResetOrientation(float Yaw)
{
#ifdef CARDBOARD_SDK_ENABLED
    if (HeadTracker != nullptr)
        CardboardHeadTracker_recenter(HeadTracker);
#endif // CARDBOARD_SDK_ENABLED

    PoseYaw = 0.f;
}

void FUNACardboardVRHMD::ResetPosition()
{
}

void FUNACardboardVRHMD::SetBaseRotation(const FRotator& BaseRot)
{
    SetBaseOrientation(FRotator(0.0f, BaseRot.Yaw, 0.0f).Quaternion());
}

FRotator FUNACardboardVRHMD::GetBaseRotation() const
{
    return GetBaseOrientation().Rotator();
}

void FUNACardboardVRHMD::SetBaseOrientation(const FQuat& BaseOrient)
{
    BaseOrientation = BaseOrient;
}

FQuat FUNACardboardVRHMD::GetBaseOrientation() const
{
    return BaseOrientation;
}

void FUNACardboardVRHMD::DrawDistortionMesh_RenderThread(struct FRenderingCompositePassContext& Context, const FIntPoint& TextureSize)
{
	float ClipSpaceQuadZ = 0.0f;
	FMatrix QuadTexTransform = FMatrix::Identity;
	FMatrix QuadPosTransform = FMatrix::Identity;
	const FViewInfo& View = Context.View;
	const FIntRect SrcRect = View.UnscaledViewRect;

	FRHICommandListImmediate& RHICmdList = Context.RHICmdList;
	const FSceneViewFamily& ViewFamily = *(View.Family);
	// NOTE: ViewportSize is the size in points multiplied by a factor determined from r.MobileContentScaleFactor.
	// In general, this is different from the size in points and the size in hardware pixels.
	FIntPoint ViewportSize = ViewFamily.RenderTarget->GetSizeXY();
#if PLATFORM_IOS
	// Adjust main window frame to not display on top of notch if exists.
	// See comments in GetScreenSize().
	// ATTENTION: iOS contentScaleFactor is modified by UE4! See: https://github.com/EpicGames/UnrealEngine/blob/2bf1a5b83a7076a0fd275887b373f8ec9e99d431/Engine/Source/Runtime/ApplicationCore/Private/IOS/IOSView.cpp#L303
	// So retrieve the currently applied value.
	// if (@available(iOS 11, *)) {...}
	float ContentScaleFactor = [[IOSAppDelegate GetDelegate].IOSView contentScaleFactor];
	const UIEdgeInsets safeInsets = UIApplication.sharedApplication.keyWindow.safeAreaInsets;
	RHICmdList.SetViewport(
		safeInsets.left * ContentScaleFactor,	// MinX
		0.0f,		// MinY
		0.0f,		// MinZ
		ViewportSize.X,		// MaxX
		ViewportSize.Y,		// MaxY
		1.0f		// MaxZ
	);
#else
	RHICmdList.SetViewport(0, 0, 0.0f, ViewportSize.X, ViewportSize.Y, 1.0f);
#endif

#ifdef CARDBOARD_SDK_ENABLED
    const FDistorsionMesh& DistorsionMesh = View.StereoPass == eSSP_LEFT_EYE ? DistorsionMeshes_RenderThread[0] : DistorsionMeshes_RenderThread[1];

    const uint32 NumVerts = DistorsionMesh.Vertices.Num();

	FRHIResourceCreateInfo CreateInfo;
	FVertexBufferRHIRef VertexBufferRHI = RHICreateVertexBuffer(sizeof(FDistortionVertex) * NumVerts, BUF_Volatile, CreateInfo);
	void* VoidPtr = RHILockVertexBuffer(VertexBufferRHI, 0, sizeof(FDistortionVertex) * NumVerts, RLM_WriteOnly);
	FPlatformMemory::Memcpy(VoidPtr, DistorsionMesh.Vertices.GetData(), sizeof(FDistortionVertex) * NumVerts);
	RHIUnlockVertexBuffer(VertexBufferRHI);

    const int NumIndices = DistorsionMesh.Indices.Num();
    const int NumTris = DistorsionMesh.Indices.Num() / 3;

	FIndexBufferRHIRef IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), sizeof(uint16) * NumIndices, BUF_Volatile, CreateInfo);
	void* VoidPtr2 = RHILockIndexBuffer(IndexBufferRHI, 0, sizeof(uint16) * NumIndices, RLM_WriteOnly);
	FPlatformMemory::Memcpy(VoidPtr2, DistorsionMesh.Indices.GetData(), sizeof(uint16) * NumIndices);
	RHIUnlockIndexBuffer(IndexBufferRHI);
#else
    static const uint32 NumVerts = 4;
    static const uint32 NumTris = 2;

    static const FDistortionVertex LeftVerts[] =
    {
        // left eye
        { FVector2D(-0.9f, -0.9f), FVector2D(0.0f, 1.0f), FVector2D(0.0f, 1.0f), FVector2D(0.0f, 1.0f), 1.0f, 0.0f },
        { FVector2D(-0.1f, -0.9f), FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f), 1.0f, 0.0f },
        { FVector2D(-0.1f, 0.9f), FVector2D(0.5f, 0.0f), FVector2D(0.5f, 0.0f), FVector2D(0.5f, 0.0f), 1.0f, 0.0f },
        { FVector2D(-0.9f, 0.9f), FVector2D(0.0f, 0.0f), FVector2D(0.0f, 0.0f), FVector2D(0.0f, 0.0f), 1.0f, 0.0f },
    };

    static const FDistortionVertex RightVerts[] =
    {
        // right eye
        { FVector2D(0.1f, -0.9f), FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f), 1.0f, 0.0f },
        { FVector2D(0.9f, -0.9f), FVector2D(1.0f, 1.0f), FVector2D(1.0f, 1.0f), FVector2D(1.0f, 1.0f), 1.0f, 0.0f },
        { FVector2D(0.9f, 0.9f), FVector2D(1.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(1.0f, 0.0f), 1.0f, 0.0f },
        { FVector2D(0.1f, 0.9f), FVector2D(0.5f, 0.0f), FVector2D(0.5f, 0.0f), FVector2D(0.5f, 0.0f), 1.0f, 0.0f },
    };

    FRHIResourceCreateInfo CreateInfo;
    FVertexBufferRHIRef VertexBufferRHI = RHICreateVertexBuffer(sizeof(FDistortionVertex) * NumVerts, BUF_Volatile, CreateInfo);
    void* VoidPtr = RHILockVertexBuffer(VertexBufferRHI, 0, sizeof(FDistortionVertex) * NumVerts, RLM_WriteOnly);
    FPlatformMemory::Memcpy(VoidPtr, View.StereoPass == eSSP_RIGHT_EYE ? RightVerts : LeftVerts, sizeof(FDistortionVertex) * NumVerts);
    RHIUnlockVertexBuffer(VertexBufferRHI);

    static const uint16 Indices[] = { 0, 1, 2, 0, 2, 3 };

    FIndexBufferRHIRef IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), sizeof(uint16) * 12, BUF_Volatile, CreateInfo);
    void* VoidPtr2 = RHILockIndexBuffer(IndexBufferRHI, 0, sizeof(uint16) * 12, RLM_WriteOnly);
    FPlatformMemory::Memcpy(VoidPtr2, Indices, sizeof(uint16) * 12);
    RHIUnlockIndexBuffer(IndexBufferRHI);
#endif // CARDBOARD_SDK_ENABLED

	RHICmdList.SetStreamSource(0, VertexBufferRHI, 0);
	RHICmdList.DrawIndexedPrimitive(IndexBufferRHI, 0, 0, NumVerts, 0, NumTris, 1);

	IndexBufferRHI.SafeRelease();
	VertexBufferRHI.SafeRelease();
}

bool FUNACardboardVRHMD::IsStereoEnabled() const
{
    return bStereoEnabled;
}

bool FUNACardboardVRHMD::EnableStereo(bool stereo)
{
    check(IsInGameThread());

    bStereoEnabled = stereo;
    GEngine->bForceDisableFrameRateSmoothing = bStereoEnabled;

    FUNACardboardVRDevice::GetInstance().HandleNativeUI(stereo);
    SetPlayerControllerTouchInterfaceEnabled(!stereo);

    if (stereo)
    {
        FUNACardboardVRDevice::GetInstance().HandleQRScanningAutomatically();
    }
    else
    {
        ResetControlRotation();
    }

    return bStereoEnabled;
}

APlayerController* FUNACardboardVRHMD::FindPlayerController() const
{
    if (GEngine && GEngine->GameViewport)
        return GEngine->GameViewport->GetWorld()->GetFirstPlayerController();
    return nullptr;
}

void FUNACardboardVRHMD::SetPlayerControllerTouchInterfaceEnabled(bool bEnabled) const
{
    APlayerController* pc = FindPlayerController();
    if (pc)
    {
        if (bEnabled)
            pc->CreateTouchInterface();
        else
            pc->ActivateTouchInterface(nullptr);
    }
}

void FUNACardboardVRHMD::ResetControlRotation() const
{
    // Switching back to non-stereo mode: reset player rotation and aim.
    // Should we go through all playercontrollers here?
    APlayerController* pc = FindPlayerController();
    if (pc)
    {
        // Reset Aim? @todo
        FRotator r = pc->GetControlRotation();
        r.Normalize();
        // Reset roll and pitch of the player
        r.Roll = 0;
        r.Pitch = 0;
        pc->SetControlRotation(r);
    }
}

void FUNACardboardVRHMD::AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
{
	SizeX = SizeX / 2;
	if( StereoPass == eSSP_RIGHT_EYE )
	{
		X += SizeX;
	}
}

void FUNACardboardVRHMD::CalculateStereoViewOffset(const enum EStereoscopicPass StereoPassType, FRotator& ViewRotation, const float InWorldToMeters, FVector& ViewLocation)
{
    // Forward to the base implementation (that in turn will call the DefaultXRCamera implementation)
    FHeadMountedDisplayBase::CalculateStereoViewOffset(StereoPassType, ViewRotation, WorldToMeters, ViewLocation);
}

FMatrix FUNACardboardVRHMD::GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType) const
{
#ifdef CARDBOARD_SDK_ENABLED
    float fov[4];
    CardboardLensDistortion_getFieldOfView(lensDistortion, (StereoPassType == eSSP_LEFT_EYE) ? CardboardEye::kLeft : CardboardEye::kRight, fov);

    // Have to flip left/right and top/bottom to match UE4 expectations
    //fov is left, right, bottom, top (for cardboard)
    float Left = -FMath::Tan(fov[1]);
    float Right = FMath::Tan(fov[0]);
    float Bottom = -FMath::Tan(fov[3]);
    float Top = FMath::Tan(fov[2]);

    float ZNear = GNearClippingPlane;

    float SumRL = (Right + Left);
    float SumTB = (Top + Bottom);
    float InvRL = (1.0f / (Right - Left));
    float InvTB = (1.0f / (Top - Bottom));

    FPlane Plane0 = FPlane((2.0f * InvRL), 0.0f, 0.0f, 0.0f);
    FPlane Plane1 = FPlane(0.0f, (2.0f * InvTB), 0.0f, 0.0f);
    FPlane Plane2 = FPlane((SumRL * InvRL), (SumTB * InvTB), 0.0f, 1.0f);
    FPlane Plane3 = FPlane(0.0f, 0.0f, ZNear, 0.0f);

    FMatrix Result = FMatrix(
        Plane0,
        Plane1,
        Plane2,
        Plane3
    );

    return Result;
#else
	const float ProjectionCenterOffset = 0.151976421f;
	const float PassProjectionOffset = (StereoPassType == eSSP_LEFT_EYE) ? ProjectionCenterOffset : -ProjectionCenterOffset;

	const float HalfFov = 2.19686294f / 2.f;
	const float InWidth = 640.f;
	const float InHeight = 480.f;
	const float XS = 1.0f / tan(HalfFov);
	const float YS = InWidth / tan(HalfFov) / InHeight;

	const float InNearZ = GNearClippingPlane;
	FMatrix ProjMatrix = FMatrix(
		FPlane(XS,                      0.0f,								    0.0f,							0.0f),
		FPlane(0.0f,					YS,	                                    0.0f,							0.0f),
		FPlane(0.0f,	                0.0f,								    0.0f,							1.0f),
		FPlane(0.0f,					0.0f,								    InNearZ,						0.0f))

		* FTranslationMatrix(FVector(PassProjectionOffset,0,0));

    return ProjMatrix;
#endif // CARDBOARD_SDK_ENABLED
}

void FUNACardboardVRHMD::GetEyeRenderParams_RenderThread(const FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const
{
    if (Context.View.StereoPass == eSSP_LEFT_EYE)
    {
        EyeToSrcUVOffsetValue.X = 0.0f;
        EyeToSrcUVOffsetValue.Y = 0.0f;

        EyeToSrcUVScaleValue.X = 0.5f;
        EyeToSrcUVScaleValue.Y = 1.0f;
    }
    else
    {
        EyeToSrcUVOffsetValue.X = 0.5f;
        EyeToSrcUVOffsetValue.Y = 0.0f;

        EyeToSrcUVScaleValue.X = 0.5f;
        EyeToSrcUVScaleValue.Y = 1.0f;
    }
}


void FUNACardboardVRHMD::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	InViewFamily.EngineShowFlags.MotionBlur = 0;
	InViewFamily.EngineShowFlags.HMDDistortion = true;
	InViewFamily.EngineShowFlags.StereoRendering = IsStereoEnabled();

	if (UWorld* World = GWorld)
	{
		WorldToMeters = World->GetWorldSettings()->WorldToMeters;
	}
}

void FUNACardboardVRHMD::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
    if (FUNACardboardVRDevice::GetInstance().GetAndThenReset_IsDeviceParamsChanged())
    {
        UpdateDeviceParams();
    }
}

void FUNACardboardVRHMD::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
	check(IsInRenderingThread());
}

void FUNACardboardVRHMD::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily)
{
	check(IsInRenderingThread());
}

bool FUNACardboardVRHMD::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	return GEngine && GEngine->IsStereoscopic3D(Context.Viewport);
}

FUNACardboardVRHMD::FUNACardboardVRHMD(const FAutoRegister& AutoRegister) :
	FHeadMountedDisplayBase(nullptr),
	FSceneViewExtensionBase(AutoRegister),
	WorldToMeters(100.0f),
    bInitialized(false),
    bStereoEnabled(false),
    HeadTracker(nullptr),
    lensDistortion(nullptr),
    BaseOrientation(FQuat::Identity),
    PoseYaw(0.0f),
    PosePitch(0.0f)
{
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddRaw(this, &FUNACardboardVRHMD::ApplicationPauseDelegate);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddRaw(this, &FUNACardboardVRHMD::ApplicationResumeDelegate);

#if PLATFORM_ANDROID 
    // By default, go ahead and disable the screen saver. The user can still change this freely
    FPlatformApplicationMisc::ControlScreensaver(FPlatformApplicationMisc::Disable);
#endif // PLATFORM_ANDROID

    Initialize();

    // needed when we run the game in non - stereo otherwise HandleNativeUI is not called
    FUNACardboardVRDevice::GetInstance().HandleNativeUI(false);
}

FUNACardboardVRHMD::~FUNACardboardVRHMD()
{
	FUNACardboardVRDevice::GetInstance().Deinitialize();

#ifdef CARDBOARD_SDK_ENABLED
    if (lensDistortion != nullptr)
        CardboardLensDistortion_destroy(lensDistortion);
    if (HeadTracker != nullptr)
        CardboardHeadTracker_destroy(HeadTracker);
#endif // CARDBOARD_SDK_ENABLED

    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveAll(this);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.RemoveAll(this);
}

bool FUNACardboardVRHMD::IsInitialized() const
{
	return bInitialized;
}

bool FUNACardboardVRHMD::Initialize()
{
    if (bInitialized)
        return false;

    UE_LOG(LogUNACardboardVR, Display, TEXT("FUNACardboardVRHMD::Initialize\n"));

#ifdef CARDBOARD_SDK_ENABLED
    HeadTracker = CardboardHeadTracker_create();
#endif // CARDBOARD_SDK_ENABLED

    UpdateDeviceParams();

    float IPD = GetInterpupillaryDistance();
    UE_LOG(LogUNACardboardVR, Display, TEXT("FUNACardboardVRHMD::Initialize - IPD=%f\n"), IPD);

	FUNACardboardVRDevice::GetInstance().Initialize();

#if PLATFORM_IOS
	// On iOS it seems that the ApplicationHasEnteredForegroundDelegate is not fired on start-up,
	// and we need to call CardboardHeadTracker_resume() to receive head tracking data.
    ApplicationResumeDelegate();
#endif

    bInitialized = true;
    return true;
}

void FUNACardboardVRHMD::UpdateDeviceParams()
{
#ifdef CARDBOARD_SDK_ENABLED
    // Get saved device parameters
    uint8_t* buffer;
    int size;
    CardboardQrCode_getSavedDeviceParams(&buffer, &size);

    bool bDeallocateBuffer = true;

    // If there are no parameters saved yet, use the V1 device params as fallback
    if (size == 0)
    {
        // free the previous buffer
        CardboardQrCode_destroy(buffer);

        CardboardQrCode_getCardboardV1DeviceParams(&buffer, &size);

        // the buffer returned by CardboardQrCode_getCardboardV1DeviceParams
        // must not be deallocated
        bDeallocateBuffer = false;
    }

    FIntPoint ScreenSize = GetScreenSize();

    if(lensDistortion != nullptr)
        CardboardLensDistortion_destroy(lensDistortion);
	lensDistortion = CardboardLensDistortion_create(buffer, size, ScreenSize.X, ScreenSize.Y);
	// Some QR codes fails to be parsed. The Google SDK raises an exception in this case, but as UE4 has exception disabled by default on mobile,
	// our fork of the SDK returns nullptr instead (cset 521098284da0).
	if (lensDistortion == nullptr)
	{
		UE_LOG(LogUNACardboardVR, Warning, TEXT("FUNACardboardVRHMD::UpdateDeviceParams - Failed to parse the Cardboard configuration. Falling back to the Cardboard V1 parameters."));
		// Use the V1 device params as fallback.
		checkf(bDeallocateBuffer, TEXT("bDeallocateBuffer is false, and it means that the failing buffer is from CardboardQrCode_getCardboardV1DeviceParams(). This shouldn't definitely happen."));
		CardboardQrCode_destroy(buffer);
		CardboardQrCode_getCardboardV1DeviceParams(&buffer, &size);
        bDeallocateBuffer = false;	// the buffer returned by CardboardQrCode_getCardboardV1DeviceParams must not be deallocated
        lensDistortion = CardboardLensDistortion_create(buffer, size, ScreenSize.X, ScreenSize.Y);
	}
	check(lensDistortion != nullptr);

#if CARDBOARD_VERBOSE_LOGGING
    if (buffer && size > 0)
    {
        FString DeviceParamsRawBuffer = BytesToHex(buffer, size);
        UE_LOG(LogUNACardboardVR, Display, TEXT("device params sdk: %s"), *DeviceParamsRawBuffer);
    }
#endif // CARDBOARD_VERBOSE_LOGGING

    if (bDeallocateBuffer)
    {
        CardboardQrCode_destroy(buffer);
    }

    CardboardMesh leftEyeMesh;
    CardboardMesh rightEyeMesh;

    CardboardLensDistortion_getDistortionMesh(lensDistortion, kLeft, &leftEyeMesh);
    CardboardLensDistortion_getDistortionMesh(lensDistortion, kRight, &rightEyeMesh);

    ConvertCardboardMeshToUnrealDistorsionMesh(leftEyeMesh, DistorsionMeshes[0]);
    ConvertCardboardMeshToUnrealDistorsionMesh(rightEyeMesh, DistorsionMeshes[1]);

    ENQUEUE_RENDER_COMMAND(FUpdateDistorsionMeshes)(
        [&](FRHICommandListImmediate& RHICmdList)
    {
        DistorsionMeshes_RenderThread[0] = DistorsionMeshes[0];
        DistorsionMeshes_RenderThread[1] = DistorsionMeshes[1];
    });

    float eye_matrices_[2][16];

    // Get eye matrices from cardboard sdk
    CardboardLensDistortion_getEyeFromHeadMatrix(lensDistortion, kLeft, eye_matrices_[0]);
    CardboardLensDistortion_getEyeFromHeadMatrix(lensDistortion, kRight, eye_matrices_[1]);

    // then convert to unreal...
    eyeMatrices[0] = ConvertCardboardMatrixToUnreal(eye_matrices_[0], WorldToMeters);
    eyeMatrices[1] = ConvertCardboardMatrixToUnreal(eye_matrices_[1], WorldToMeters);

#if CARDBOARD_VERBOSE_LOGGING
    static IConsoleVariable* MobileContentScaleFactorVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MobileContentScaleFactor"));
    float MobileContentScaleFactor = MobileContentScaleFactorVar->GetFloat();

    UE_LOG(LogUNACardboardVR, Display, TEXT("ScreenSize=(%d,%d), MobileContentScaleFactor=%f\n"), ScreenSize.X, ScreenSize.Y, MobileContentScaleFactor);

    FVector EyePos;
    FQuat EyeOrientation;
    GetRelativeHMDEyePose(eSSP_LEFT_EYE, EyeOrientation, EyePos);    
    UE_LOG(LogUNACardboardVR, Display, TEXT("left eye pos and orientation %s / %s"), *EyePos.ToString(), *EyeOrientation.ToString());
    GetRelativeHMDEyePose(eSSP_RIGHT_EYE, EyeOrientation, EyePos);
    UE_LOG(LogUNACardboardVR, Display, TEXT("right eye pos and orientation %s / %s"), *EyePos.ToString(), *EyeOrientation.ToString());

    float fov[4];
    CardboardLensDistortion_getFieldOfView(lensDistortion, CardboardEye::kLeft, fov);
    UE_LOG(LogUNACardboardVR, Display, TEXT("left eye fov sdk: %f %f %f %f"), fov[0], fov[1], fov[2], fov[3]);
    CardboardLensDistortion_getFieldOfView(lensDistortion, CardboardEye::kRight, fov);
    UE_LOG(LogUNACardboardVR, Display, TEXT("right eye fov sdk: %f %f %f %f"), fov[0], fov[1], fov[2], fov[3]);

    FMatrix ProjMatrix = GetStereoProjectionMatrix(eSSP_LEFT_EYE);
    UE_LOG(LogUNACardboardVR, Display, TEXT("left eye proj matrix: %s"), *ProjMatrix.ToString());
    ProjMatrix = GetStereoProjectionMatrix(eSSP_RIGHT_EYE);
    UE_LOG(LogUNACardboardVR, Display, TEXT("right eye proj matrix: %s"), *ProjMatrix.ToString());
#endif // CARDBOARD_VERBOSE_LOGGING

#endif // CARDBOARD_SDK_ENABLED
}

void FUNACardboardVRHMD::ApplicationPauseDelegate()
{
    UE_LOG(LogUNACardboardVR, Display, TEXT("FUNACardboardVRHMD::ApplicationPauseDelegate\n"));

#ifdef CARDBOARD_SDK_ENABLED
    if(HeadTracker != nullptr)
        CardboardHeadTracker_pause(HeadTracker);
#endif // CARDBOARD_SDK_ENABLED
}

void FUNACardboardVRHMD::ApplicationResumeDelegate()
{
    UE_LOG(LogUNACardboardVR, Display, TEXT("FUNACardboardVRHMD::ApplicationResumeDelegate\n"));

#ifdef CARDBOARD_SDK_ENABLED
    if (HeadTracker != nullptr)
        CardboardHeadTracker_resume(HeadTracker);
#endif // CARDBOARD_SDK_ENABLED
}
