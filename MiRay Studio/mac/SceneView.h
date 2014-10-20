//
//  SceneView.h
//  MiRay Studio
//
//  Created by Damir Sagidullin on 28.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface SceneView : NSOpenGLView
{
}

@property (assign) mr::SceneView *pSceneView;
@property NSArray *modelFileTypes;
@property NSArray *imageFileTypes;
@property NSString *filename;

@end
