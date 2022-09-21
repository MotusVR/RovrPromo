// Copyright 2020 UNAmedia. All rights reserved.

#include "UNACardboardVRSubsystem.h"


UUNACardboardVRSubsystem& UUNACardboardVRSubsystem::GetInstance(const UObject * context)
{
    check(context != nullptr);
    UWorld * world = context->GetWorld();
    const UGameInstance * gameInst = nullptr;
    if (world != nullptr)
    {
        gameInst = world->GetGameInstance();
    }
    else
    {
        gameInst = Cast<UGameInstance>(context);
    }

    check(gameInst != nullptr);
    UUNACardboardVRSubsystem * result = gameInst->GetSubsystem<UUNACardboardVRSubsystem>();
    check(result != nullptr);
    return *result;
}
