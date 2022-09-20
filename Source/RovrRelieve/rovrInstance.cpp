// Fill out your copyright notice in the Description page of Project Settings.


#include "rovrInstance.h"
#include "Containers/Array.h"
#include "Misc/LocalTimestampDirectoryVisitor.h"
#include <string>
#include <iostream>




#define DEBUGMESSAGE(x, colour, ...) if(GEngine){GEngine->AddOnScreenDebugMessage(-1, 10.0f, colour, x);}


TArray<FString> UrovrInstance::GetAllFilesInDirectory(const FString directory, const bool fullPath, const FString onlyFilesStartingWith, const FString onlyFilesWithExtension, const FString SDCardDirectory)
{
	// Get all files in directory
	TArray<FString> directoriesToSkip;
	IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FLocalTimestampDirectoryVisitor Visitor(PlatformFile, directoriesToSkip, directoriesToSkip, false);
	PlatformFile.IterateDirectory(*directory, Visitor);
	TArray<FString> files;

	//DEBUGMESSAGE(getenv("EXTERNAL_STORAGE"));

	for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)
	{
		const FString filePath = TimestampIt.Key();
		const FString fileName = FPaths::GetCleanFilename(filePath);
		//DEBUGMESSAGE(fileName);
		bool shouldAddFile = true;

		// Check if filename starts with required characters
		if (!onlyFilesStartingWith.IsEmpty())
		{
			const FString left = fileName.Left(onlyFilesStartingWith.Len());

			if (!(fileName.Left(onlyFilesStartingWith.Len()).Equals(onlyFilesStartingWith)))
				shouldAddFile = false;
		}

		// Check if file extension is required characters
		if (!onlyFilesWithExtension.IsEmpty())
		{
			if (!(FPaths::GetExtension(fileName, false).Equals(onlyFilesWithExtension, ESearchCase::IgnoreCase)))
				shouldAddFile = false;
		}

		// Add full path to results
		if (shouldAddFile)
		{
			files.Add(fullPath ? filePath : fileName);
		}
	}

	//DEBUGMESSAGE(TEXT("\n"));

	FLocalTimestampDirectoryVisitor Visitor1(PlatformFile, directoriesToSkip, directoriesToSkip, false);
	PlatformFile.IterateDirectory(*SDCardDirectory, Visitor1);
	//bool SDCardDirectoryExists = true;
	//SDCardDirectoryExists;
	//DEBUGMESSAGE(TEXT(SDCardDirectoryExists));
	

	if (!(SDCardDirectory == "") && PlatformFile.DirectoryExists(*SDCardDirectory))
	{

		DEBUGMESSAGE(TEXT("INSIDE"), FColor::Green);
		for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor1.FileTimes); TimestampIt; ++TimestampIt)
		{
			const FString filePath = TimestampIt.Key();
			const FString fileName = FPaths::GetCleanFilename(filePath);
			DEBUGMESSAGE(fileName, FColor::Yellow);
			bool shouldAddFile = true;

			 //Check if filename starts with required characters
			if (!onlyFilesStartingWith.IsEmpty())
			{
				const FString left = fileName.Left(onlyFilesStartingWith.Len());

				if (!(fileName.Left(onlyFilesStartingWith.Len()).Equals(onlyFilesStartingWith)))
					shouldAddFile = false;
			}

			 //Check if file extension is required characters
			if (!onlyFilesWithExtension.IsEmpty())
			{
				if (!(FPaths::GetExtension(fileName, false).Equals(onlyFilesWithExtension, ESearchCase::IgnoreCase)))
					shouldAddFile = false;
			}

			 //Add full path to results
			if (shouldAddFile)
			{
				files.Add(fullPath ? filePath : fileName);
			}
		}
	}
	//files.Sort();

	for (int x = 0; x < files.Num()-1; x++) {
		for (int y = 0; y < files.Num() - x - 1; y++) {

			if (FPaths::GetBaseFilename(files[y]) > FPaths::GetBaseFilename(files[y + 1])) {
				FString temp = files[y];
				files[y] = files[y + 1];
				files[y + 1] = temp;
			}

		}
	}


	return files;
}

UTexture2D* UrovrInstance::testinsal123(int32 imageWidth, int32 imageHeight, TArray<FColor> ColorArray)
{
	//FString testName;
	//FCreateTexture2DParameters params;
	//FImageUtils::CreateTexture2D(imageWidth, imageHeight, ColorArray, outputTexture, "test", EObjectFlags::RF_Transient, params);
	//for (FColor x : ColorArray) {
	//	int red = x.R;
	//	colorOutput = FString::FromInt(red);
	//	DEBUGMESSAGE(colorOutput, FColor::Red);
	//}

	


	UTexture2D* texture = UTexture2D::CreateTransient(imageWidth, imageHeight, EPixelFormat::PF_B8G8R8A8);

	// Create the texture


	// Lock the texture so it can be modified
	uint8* MipData = static_cast<uint8*>(texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	// Create base mip.
	uint8* DestPtr = NULL;
	const FColor* SrcPtr = NULL;
	for (int32 y = 0; y < imageHeight; y++)
	{
		DestPtr = &MipData[(imageHeight - 1 - y) * imageWidth * sizeof(FColor)];
		SrcPtr = const_cast<FColor*>(&ColorArray[(imageHeight - 1 - y) * imageWidth]);
		for (int32 x = 0; x < imageWidth; x++)
		{
			*DestPtr++ = SrcPtr->B;
			*DestPtr++ = SrcPtr->G;
			*DestPtr++ = SrcPtr->R;

			*DestPtr++ = 0xFF;
			SrcPtr++;
		}
	}

	// Unlock the texture
	texture->PlatformData->Mips[0].BulkData.Unlock();
	texture->UpdateResource();
	return texture;
}

