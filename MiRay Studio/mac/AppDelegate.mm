//
//  AppDelegate.m
//  MiRay Studio
//
//  Created by Damir Sagidullin on 28.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#import "AppDelegate.h"
#import "AppController.h"

@interface AppDelegate()

@property (weak) IBOutlet AppController *appController;

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	[self.appController initScene];
}

@end
