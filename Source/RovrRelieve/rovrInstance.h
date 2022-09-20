// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManagerGeneric.h"
#include "Misc/Paths.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"


#include "rovrInstance.generated.h"



/**
 * 
 */
UCLASS()
class ROVRRELIEVE_API UrovrInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = FileManager, meta = (AdvancedDisplay = 1))
		TArray<FString> GetAllFilesInDirectory(FString directory, bool fullPath, FString onlyFilesStartingWith, FString onlyFilesWithExtension = "mp4", FString SDCardDirectory = "");
	UFUNCTION(BlueprintCallable, Category = FileManager, meta = (AdvancedDisplay = 1))
		UTexture2D* testinsal123(int32 imageWidth, int32 imageHeight, TArray<FColor> ColorArray);

};
