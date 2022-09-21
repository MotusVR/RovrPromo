// Copyright 2020 UNAmedia. All rights reserved.

#include "UNACardboardOverlayView.h"

#if PLATFORM_IOS

#include <UNACardboardVRStatics.h>

#include "Async/Async.h"

#define UNA_CARDBOARD_RESOURCEBUNDLE "UNACardboard.bundle"


namespace UNACardboardOverlayView_Internal
{

// fire (InFunction to be called on the GameThread) and forget
FORCEINLINE
void CallOnGameThread(TFunction<void()> InFunction)
{
	AsyncTask(ENamedThreads::GameThread, InFunction);
}

} // namespace UNACardboardOverlayView_Internal



@interface UNACardboardOverlayView()
{}
@end



@implementation UNACardboardOverlayView



-(void) updateUIStateWithVisibility: (BOOL) IsUIVisible
	IsStereoEnabled: (BOOL) IsStereoEnabled
    IsSettingsButtonVisible: (BOOL) IsSettingsButtonVisible
	IsBackButtonVisible: (BOOL) IsBackButtonVisible
	IsToggleStereoModeButtonVisible: (BOOL) IsToggleStereoModeButtonVisible
{
	/*
	Visibility scheme:
		| Mode       | bShowNativeUI=T                                                                           | bShowNativeUI=F |
		|------------|-------------------------------------------------------------------------------------------|-----------------|
		| stereo     | A:[settingButton=T, stereoButton=bShowToggleStereoModeButton, backButton=bShowBackButton] | X               |
		| not-stereo | B:[settingButton=F, stereoButton=bShowToggleStereoModeButton, backButton=F]               | X               |
	*/
	[self.backButton setHidden:!(IsUIVisible && IsStereoEnabled && IsBackButtonVisible)];
	[self.settingsButton setHidden:!(IsUIVisible && IsStereoEnabled && IsSettingsButtonVisible)];
	[self.lensSeparatorView setHidden:!(IsUIVisible && IsStereoEnabled)];
	NSString * stereoModeImageBasename = IsStereoEnabled ? @"ic_fullscreen" : @"ic_cardboard";
	UIImage * stereoModeImage = [UIImage imageNamed:[NSString stringWithFormat:@ UNA_CARDBOARD_RESOURCEBUNDLE "/%@", stereoModeImageBasename]];
	NSAssert(stereoModeImage != nil, @"Image not found ('%@' in resource bundle '%@')", stereoModeImageBasename, @ UNA_CARDBOARD_RESOURCEBUNDLE);
	[self.stereoModeButton setImage:stereoModeImage forState:UIControlStateNormal];
	[self.stereoModeButton setHidden:!(IsUIVisible && IsToggleStereoModeButtonVisible)];
}



- (IBAction)onBackButton:(id)sender
{
	if (self.BackButtonCallback)
	{
		UNACardboardOverlayView_Internal::CallOnGameThread([self](){ self.BackButtonCallback(); });
	}
}



- (IBAction)onSettingsButton:(id)sender
{
	if (self.SettingsMenuOpenCallback)
	{
		UNACardboardOverlayView_Internal::CallOnGameThread([self](){ self.SettingsMenuOpenCallback(); });
	}
	
	UUNACardboardVRStatics::ScanForQRViewerProfile();
}



- (IBAction)onStereoModeButton:(id)sender
{
	if(self.StereoModeToggleButtonCallback)
	{
		UNACardboardOverlayView_Internal::CallOnGameThread([self](){ self.StereoModeToggleButtonCallback(); });
	}
	/*
	UIAlertView *alert = [[UIAlertView alloc]
		initWithTitle:@"Stereo mode"
		message:@"Stereo mode button pressed"
		delegate:self
		cancelButtonTitle:@"Ok"
		otherButtonTitles:@"Cancel", nil
	];
	[alert show];
	*/
}

@end

#endif // if PLATFORM_IOS
