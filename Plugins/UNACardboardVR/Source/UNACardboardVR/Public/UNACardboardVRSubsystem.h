// Copyright 2020 UNAmedia. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "Subsystems/GameInstanceSubsystem.h"

#include "UNACardboardVRSubsystem.generated.h"

/**
 * The game instance subsystem of the UNACardboardVR plugin.
 * 
 * Other useful run-time methods are in the UUNACardboardVRStatics class.
 */
UCLASS()
class UNACARDBOARDVR_API UUNACardboardVRSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    /** FStereoModeEvent delegate signature */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStereoModeEvent, bool, bStereoEnabled);
    /** FSettingsMenuEvent delegate signature */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSettingsMenuEvent);

public:

    /**
     * Return the CardboardVR Subsystem instance.
     */
    static UUNACardboardVRSubsystem& GetInstance(const UObject * context);

public:

    /**
     * Delegate called when the stereo mode has changed following a user interaction with the Cardboard UI overlay.
     * This is fired only if the button is shown on screen (see UUNACardboardVRProjectSettings and UUNACardboardVRStatics).
     */
    UPROPERTY(BlueprintAssignable)
    FStereoModeEvent OnStereoModeChanged;

    /**
     * Delegate called when the settings menu is opened by a user interaction with the Cardboard UI overlay.
     * This is fired only if the button is shown on screen (see UUNACardboardVRProjectSettings and UUNACardboardVRStatics).
     */
    UPROPERTY(BlueprintAssignable)
    FSettingsMenuEvent OnSettingsMenuOpened;
};
