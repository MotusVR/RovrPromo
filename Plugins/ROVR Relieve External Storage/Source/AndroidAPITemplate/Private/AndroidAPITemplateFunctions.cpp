// Copyright (c) 2018 Isara Technologies. All Rights Reserved.
#include "AndroidAPITemplateFunctions.h"
#include "Misc/Char.h"
#include "Engine/GameEngine.h"
#include "Engine/Engine.h"
#include "AndroidAPITemplatePrivatePCH.h"

#if PLATFORM_ANDROID

#include <string>
#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"

#define INIT_JAVA_METHOD(name, signature) \
if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true)) { \
	name = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, #name, signature, false); \
	check(name != NULL); \
} else { \
	check(0); \
}

#define DECLARE_JAVA_METHOD(name) \
static jmethodID name = NULL;

DECLARE_JAVA_METHOD(AndroidThunkJava_AndroidAPI_ShowToast);
DECLARE_JAVA_METHOD(AndroidThunkJava_AndroidAPI_Test);
DECLARE_JAVA_METHOD(AndroidThunkJava_AndroidAPI_Test2);


void UAndroidAPITemplateFunctions::InitJavaFunctions()
{
	// Same here, but we add the Java signature (the type of the parameters is between the parameters, and the return type is added at the end,
	// here the return type is V for "void")
	// More details here about Java signatures: http://www.rgagnon.com/javadetails/java-0286.html
	INIT_JAVA_METHOD(AndroidThunkJava_AndroidAPI_ShowToast, "(Ljava/lang/String;)V");
	INIT_JAVA_METHOD(AndroidThunkJava_AndroidAPI_Test, "([I)V");
	INIT_JAVA_METHOD(AndroidThunkJava_AndroidAPI_Test2, "([IIZ)V");

}
#undef DECLARE_JAVA_METHOD
#undef INIT_JAVA_METHOD

#endif

void UAndroidAPITemplateFunctions::AndroidAPITemplate_ShowToast(const FString& Content)
{
#if PLATFORM_ANDROID
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true))
	{
		// First step, we convert the FString (UE4) parameter to a JNI parameter that will hold a String
		jstring JavaString = Env->NewStringUTF(TCHAR_TO_UTF8(*Content));

		// Then we call the method, we the Java String parameter
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_AndroidAPI_ShowToast, JavaString);
	}
#endif
}

FString UAndroidAPITemplateFunctions::AndroidAPITemplate_Test() {
#if PLATFORM_ANDROID
	//working
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true))
	{
		FString directory = "";
		std::string y;

		jintArray jarray = Env->NewIntArray(9);

		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_AndroidAPI_Test, jarray);
		jint* ResultArr = Env->GetIntArrayElements(jarray, 0);

		const int ResultFloat = ResultArr[0];
		int length = Env->GetArrayLength(jarray);

		for (int i = 0; i <length; i++){
			y += char(int(ResultArr[i]));
		}

		FString myString(y.c_str());

		Env->DeleteLocalRef(jarray);
		
		return myString;
	}
	else
	{
		UE_LOG(LogAndroid, Warning, TEXT("ERROR: Could not get Java ENV\n"));
		return "Blank";
	}
#else
	return "Blank";
#endif
}


UTexture2D* UAndroidAPITemplateFunctions::AndroidAPITemplate_Test2(int32 thumbNum, bool headset) {
#if PLATFORM_ANDROID
	//working
	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv(true))
	{
		jintArray jarray = Env->NewIntArray(185000);

		jint testjint = thumbNum;

		jboolean testjbool = headset;

		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_AndroidAPI_Test2, jarray, testjint, testjbool);
		
		jint* ResultArr = Env->GetIntArrayElements(jarray, 0);

		const int ResultInt = ResultArr[0];
		int length = Env->GetArrayLength(jarray);


		

		int imageWidth = ResultArr[0];
		int imageHeight = ResultArr[1];

		//uint8 alpha((ResultArr[2] & 0xC0) >> 6) * 64;
		//uint8 red = ((ResultArr[2] & 0x30) >> 4) * 64;
		//uint8 green = ((ResultArr[2] & 0x0C) >> 2) * 64;
		//uint8 blue = ((ResultArr[2] & 0x0C) >> 2) * 64;

		TArray<FColor> colourArray;
		//sizeof(&ResultArr)/sizeof(&ResultArr[0])

		
		for (int x = 2; x < length; x++) {
			//int alpha = ((ResultArr[2] >> 24) & 0xff);
			//int red = ((ResultArr[2] >> 16) & 0xff);
			//int green = ((ResultArr[2] >> 8) & 0xff);
			//int blue = ((ResultArr[2]) & 0xff);

			colourArray.Add(FColor(((ResultArr[x] >> 16) & 0xff), ((ResultArr[x] >> 8) & 0xff), ((ResultArr[x]) & 0xff), ((ResultArr[x] >> 24) & 0xff)));
		}



		//u8 red = ((color & 0xC0) >> 6) * 64;
		//u8 green = ((color & 0x30) >> 4) * 64;
		//u8 blue = ((color & 0x0C) >> 2) * 64;

		//input[i*4+2]   = (store[i] >> 24) & 0xff;  //splitting each ARGB value to 4 separate ints  = *4 and then +1 for each separate value
		//input[i*4+3] = (store[i] >> 16) & 0xff;
		//input[i*4+4] = (store[i] >>  8) & 0xff;  //first two values are height and width  = another +2 in final array
		//input[i*4+5] = (store[i]      ) & 0xff;



		// 
		// 
		// 

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
			SrcPtr = const_cast<FColor*>(&colourArray[(imageHeight - 1 - y) * imageWidth]);
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
		

		Env->DeleteLocalRef(jarray);
		return texture;
	}
	else
	{
		UE_LOG(LogAndroid, Warning, TEXT("ERROR: Could not get Java ENV\n"));
		return UTexture2D::CreateTransient(300, 300, EPixelFormat::PF_B8G8R8A8);
	}
#else
	return UTexture2D::CreateTransient(300, 300, EPixelFormat::PF_B8G8R8A8);
#endif
}
