// Copyright 2020 UNAmedia. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HeadMountedDisplayBase.h"
#include "XRTrackingSystemBase.h"
#include "SceneViewExtension.h"
#include "Runtime/Renderer/Private/PostProcess/PostProcessHMD.h"

#include "cardboard.h"

class APlayerController;
class FSceneView;
class FSceneViewFamily;
class UCanvas;

/**
 * Cardboard Head Mounted Display
 */
class FUNACardboardVRHMD : public FHeadMountedDisplayBase, public FSceneViewExtensionBase
{
public:
#if (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 26 && ENGINE_PATCH_VERSION == 0)
	/*
	Workaround for Engine crash (see https://github.com/EpicGames/UnrealEngine/pull/7620)
	Note that the bug exists since UE4.21, but the issue seems to exhibit itself
	only with changes introduced in UE4.26.0.
	Bug fixed in UE4.26.1, so it's affecting only UE4.26.0.
	*/
	virtual IStereoLayers * GetStereoLayers() override { return nullptr; }
#endif

	/** IXRTrackingSystem interface */
	virtual FName GetSystemName() const override
	{
		static FName DefaultName(TEXT("UNACardboardVR"));
		return DefaultName;
	}
    virtual int32 GetXRSystemFlags() const override { return EXRSystemFlags::IsHeadMounted; }
    virtual void OnBeginPlay(FWorldContext& InWorldContext) override;
    virtual void OnEndPlay(FWorldContext& InWorldContext) override;

	virtual bool EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type = EXRTrackedDeviceType::Any) override;

	virtual void SetInterpupillaryDistance(float NewInterpupillaryDistance) override;
	virtual float GetInterpupillaryDistance() const override;
    virtual bool GetRelativeEyePose(int32 DeviceId, EStereoscopicPass Eye, FQuat& OutOrientation, FVector& OutPosition) override;

	virtual void ResetOrientationAndPosition(float yaw = 0.f) override;
	virtual void ResetOrientation(float Yaw = 0.f) override;
	virtual void ResetPosition() override;

	virtual bool GetCurrentPose(int32 DeviceId, FQuat& CurrentOrientation, FVector& CurrentPosition) override;
	virtual void SetBaseRotation(const FRotator& BaseRot) override;
	virtual FRotator GetBaseRotation() const override;

	virtual void SetBaseOrientation(const FQuat& BaseOrient) override;
	virtual FQuat GetBaseOrientation() const override;

	virtual class IHeadMountedDisplay* GetHMDDevice() override
	{
		return this;
	}

	virtual class TSharedPtr< class IStereoRendering, ESPMode::ThreadSafe > GetStereoRenderingDevice() override
	{
		return SharedThis(this);
	}

protected:
	/** FXRTrackingSystemBase protected interface */
	virtual float GetWorldToMetersScale() const override;

public:
	/** IHeadMountedDisplay interface */
	virtual bool IsHMDConnected() override { return true; }
	virtual bool IsHMDEnabled() const override;
	virtual void EnableHMD(bool allow = true) override;
	virtual bool GetHMDMonitorInfo(MonitorInfo&) override;
	virtual void GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const override;
	virtual bool IsChromaAbCorrectionEnabled() const override;
	virtual void DrawDistortionMesh_RenderThread(struct FRenderingCompositePassContext& Context, const FIntPoint& TextureSize) override;

	/** IStereoRendering interface */
	virtual bool IsStereoEnabled() const override;
	virtual bool EnableStereo(bool stereo = true) override;
	virtual void AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override;
	virtual void CalculateStereoViewOffset(const enum EStereoscopicPass StereoPassType, FRotator& ViewRotation,
		const float InWorldToMeters, FVector& ViewLocation) override;
	virtual FMatrix GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType) const override;
	virtual void GetEyeRenderParams_RenderThread(const struct FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const override;

	/** ISceneViewExtension interface */
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;

public:
	/** Constructor */
    FUNACardboardVRHMD(const FAutoRegister&);

	/** Destructor */
	virtual ~FUNACardboardVRHMD();

	/** @return	True if the HMD was initialized OK */
	bool IsInitialized() const;

private:

    bool GetRelativeHMDEyePose(EStereoscopicPass Eye, FQuat& OutOrientation, FVector& OutPosition) const;

    void GetHMDPose(FVector& CurrentPosition, FQuat& CurrentOrientation);

    void ApplicationPauseDelegate();
    void ApplicationResumeDelegate();

    bool Initialize();

    void UpdateDeviceParams();

    APlayerController* FindPlayerController() const;

    void SetPlayerControllerTouchInterfaceEnabled(bool bEnabled) const;

    void ResetControlRotation() const;

private:

	float					WorldToMeters;

    bool                    bInitialized;
    bool                    bStereoEnabled;

    struct CardboardHeadTracker* HeadTracker;
    struct CardboardLensDistortion* lensDistortion;

    struct FDistorsionMesh
    {
        TArray<FDistortionVertex>   Vertices;
        TArray<uint16>              Indices;
    };

    void ConvertCardboardMeshToUnrealDistorsionMesh(
        const struct CardboardMesh& InCardboardMesh,
        FDistorsionMesh& OutMesh);

    FDistorsionMesh         DistorsionMeshes[2]; // one for each eye
    FDistorsionMesh         DistorsionMeshes_RenderThread[2]; // one for each eye

    FMatrix                 eyeMatrices[2]; // one for each eye

    FQuat                   BaseOrientation;

    float PoseYaw;
    float PosePitch;
};

