// Copyright 2020 UNAmedia. All rights reserved.

#include "UNACardboardVR.h"
#include "Core.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#include "UNACardboardVRHMD.h"

#define LOCTEXT_NAMESPACE "FUNACardboardVRModule"

//---------------------------------------------------
// CardboardHMD Plugin Implementation
//---------------------------------------------------

class FUNACardboardVRModule : public IUNACardboardVRModule
{
    /** IHeadMountedDisplayModule implementation */
    virtual TSharedPtr< class IXRTrackingSystem, ESPMode::ThreadSafe > CreateTrackingSystem() override;

    FString GetModuleKeyName() const override
    {
        return FString(TEXT("UNACardboardVR"));
    }
};

IMPLEMENT_MODULE(FUNACardboardVRModule, UNACardboardVR)

TSharedPtr< class IXRTrackingSystem, ESPMode::ThreadSafe > FUNACardboardVRModule::CreateTrackingSystem()
{
    auto CardboardHMD = FSceneViewExtensions::NewExtension<FUNACardboardVRHMD>();
    if (CardboardHMD->IsInitialized())
    {
        return CardboardHMD;
    }
    return nullptr;
}

#undef LOCTEXT_NAMESPACE
