// Copyright 2020 UNAmedia. All rights reserved.

#pragma once

#ifndef UNACardboardOverlayView_h
#define UNACardboardOverlayView_h

#if PLATFORM_IOS

#import <UIKit/UIKit.h>

@interface UNACardboardOverlayView : UIView
{
}

#if __has_feature(objc_arc)
#	error Below properties should be 'weak'
#endif
@property (nonatomic, assign) IBOutlet UIButton * backButton;
@property (nonatomic, assign) IBOutlet UIButton * settingsButton;
@property (nonatomic, assign) IBOutlet UIButton * stereoModeButton;
@property (nonatomic, assign) IBOutlet UIView * lensSeparatorView;

@property (nonatomic, copy) void (^BackButtonCallback)();
@property (nonatomic, copy) void (^SettingsMenuOpenCallback)();
@property (nonatomic, copy) void (^StereoModeToggleButtonCallback)();

-(void) updateUIStateWithVisibility: (BOOL) IsUIVisible
	IsStereoEnabled: (BOOL) IsStereoEnabled
    IsSettingsButtonVisible: (BOOL) IsSettingsButtonVisible
	IsBackButtonVisible: (BOOL) IsBackButtonVisible
	IsToggleStereoModeButtonVisible: (BOOL) IsToggleStereoModeButtonVisible;

- (IBAction)onBackButton:(id)sender;
- (IBAction)onSettingsButton:(id)sender;
- (IBAction)onStereoModeButton:(id)sender;
@end

#endif // PLATFORM_IOS

#endif /* UNACardboardOverlayView_h */
