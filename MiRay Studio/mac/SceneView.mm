//
//  SceneView.m
//  MiRay Studio
//
//  Created by Damir Sagidullin on 28.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#import "SceneView.h"

@interface SceneView()

@property () mr::SceneView *pSceneView;
@property () NSPoint ptPrevMouse;
@property () NSArray *modelFileTypes;
@property () NSArray *imageFileTypes;
@property () NSString *filename;

@end

@implementation SceneView

@synthesize pSceneView, ptPrevMouse, modelFileTypes, imageFileTypes;

- (void)awakeFromNib
{
	self.modelFileTypes = [NSArray arrayWithObjects: @"mrs", @"fbx", @"dae", @"dxf", @"obj", @"3ds", nil];
	self.imageFileTypes = [NSArray arrayWithObjects: @"png", @"jpg", @"jpeg", @"tga", @"hdr", @"exr", @"tif", @"tiff", @"psd", @"dds", @"bmp", @"raw", @"gif", @"ico", @"pcx", @"pict", @"crw", @"cr2", @"nef", @"raf", @"dng", @"mos", @"kdc", @"dcr", nil];

	NSOpenGLPixelFormatAttribute attrs[] =
	{
		NSOpenGLPFAWindow,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAMultisample,
		NSOpenGLPFASampleBuffers, (NSOpenGLPixelFormatAttribute)1,
		NSOpenGLPFASamples, (NSOpenGLPixelFormatAttribute)4,
		0
	};
	[self setPixelFormat:[[NSOpenGLPixelFormat alloc] initWithAttributes:attrs]];
	
	if ([self respondsToSelector:@selector(setWantsBestResolutionOpenGLSurface:)])
		[self setWantsBestResolutionOpenGLSurface:YES];
	
	[self registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];

	[[self window] setAcceptsMouseMovedEvents:YES];

	self.pSceneView = new mr::SceneView([[[NSBundle mainBundle] resourcePath] UTF8String]);
}

- (void)dealloc
{
	[self.openGLContext makeCurrentContext];
	self.pSceneView->Done();
	[NSOpenGLContext clearCurrentContext];
	
	delete self.pSceneView;
	self.pSceneView = NULL;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (void)prepareOpenGL

{
	self.pSceneView->Init();
//	glEnable(GL_MULTISAMPLE);

	self.pSceneView->LoadScene([[[NSBundle mainBundle] pathForResource:@"cup" ofType:@"mrs"] UTF8String]);

	NSTimer *timer = [NSTimer timerWithTimeInterval:1.0/10.0 target:self selector:@selector(idle:) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
}

- (void)idle:(NSTimer *)timer
{
    if (![[NSApplication sharedApplication] isHidden] && self.pSceneView->ShouldRedraw())
		[self setNeedsDisplay:YES];
}

- (void)reshape
{
	NSRect backingBounds = [self convertRectToBacking:[self bounds]];
	self.pSceneView->Resize(backingBounds.size.width, backingBounds.size.height, self.bounds.size.width, self.bounds.size.height);
}

- (void)keyUp:(NSEvent *)theEvent
{
	if (theEvent.modifierFlags & NSControlKeyMask)
	{
		switch (theEvent.keyCode)
		{
			case kVK_ANSI_1: self.pSceneView->ResetCamera(0); break;
			case kVK_ANSI_2: self.pSceneView->ResetCamera(1); break;
			case kVK_ANSI_3: self.pSceneView->ResetCamera(2); break;
			case kVK_ANSI_4: self.pSceneView->ResetCamera(3); break;
			case kVK_ANSI_5: self.pSceneView->ResetCamera(4); break;
			case kVK_ANSI_6: self.pSceneView->ResetCamera(5); break;
			case kVK_ANSI_7: self.pSceneView->ResetCamera(6); break;
			case kVK_ANSI_8: self.pSceneView->ResetCamera(7); break;
			case kVK_ANSI_9: self.pSceneView->ResetCamera(8); break;
			case kVK_ANSI_0: self.pSceneView->ResetCamera(9); break;
			default: return;
		}
	}
	else
	{
		switch (theEvent.keyCode)
		{
			case kVK_Delete: self.pSceneView->DeleteSelection(); break;
			default: return;
		}
	}

	//	if (theEvent.keyCode == kVK_Space)
	//	{
	//		float tm = self.pSceneView->RenderScene();
	//		NSLog(@"%f", tm);
	//	}
}

- (void)mouseDown:(NSEvent *)theEvent
{
	self.ptPrevMouse = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	self.pSceneView->OnMouseDown(self.ptPrevMouse.x, self.bounds.size.height - self.ptPrevMouse.y, mr::MOUSE_LEFT);
}

- (void)mouseUp:(NSEvent *)theEvent
{
	self.pSceneView->OnMouseUp(mr::MOUSE_LEFT);
	if (theEvent.clickCount == 1)
	{
		NSPoint pt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
		self.pSceneView->OnMouseClick(pt.x, self.bounds.size.height - pt.y, mr::MOUSE_LEFT);
	}
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
	self.ptPrevMouse = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	self.pSceneView->OnMouseDown(self.ptPrevMouse.x, self.bounds.size.height - self.ptPrevMouse.y, mr::MOUSE_RIGHT);
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
	self.pSceneView->OnMouseUp(mr::MOUSE_RIGHT);
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
	NSPoint pt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	self.pSceneView->OnMouseDown(pt.x, self.bounds.size.height - pt.y, mr::MOUSE_MIDDLE);
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
	self.pSceneView->OnMouseUp(mr::MOUSE_MIDDLE);
}

- (void)mouseMoved:(NSEvent *)theEvent
{
	NSPoint pt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	float dx = pt.x - self.ptPrevMouse.x;
	float dy = pt.y - self.ptPrevMouse.y;
	self.ptPrevMouse = pt;
	self.pSceneView->OnMouseMove(pt.x, self.bounds.size.height - pt.y, dx, -dy, mr::MOUSE_NONE);
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	NSPoint pt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	float dx = pt.x - self.ptPrevMouse.x;
	float dy = pt.y - self.ptPrevMouse.y;
	self.ptPrevMouse = pt;
	self.pSceneView->OnMouseMove(pt.x, self.bounds.size.height - pt.y, dx, -dy, mr::MOUSE_LEFT);
	if (self.pSceneView->ShouldRedraw())
        [self setNeedsDisplay:YES];
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{
	NSPoint pt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	float dx = pt.x - self.ptPrevMouse.x;
	float dy = pt.y - self.ptPrevMouse.y;
	self.ptPrevMouse = pt;
	self.pSceneView->OnMouseMove(pt.x, self.bounds.size.height - pt.y, dx, -dy, mr::MOUSE_RIGHT);
	if (self.pSceneView->ShouldRedraw())
        [self setNeedsDisplay:YES];
}

- (void)otherMouseDragged:(NSEvent *)theEvent
{
	NSPoint pt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	float dx = pt.x - self.ptPrevMouse.x;
	float dy = pt.y - self.ptPrevMouse.y;
	self.ptPrevMouse = pt;
	self.pSceneView->OnMouseMove(pt.x, self.bounds.size.height - pt.y, dx, -dy, mr::MOUSE_MIDDLE);
	if (self.pSceneView->ShouldRedraw())
        [self setNeedsDisplay:YES];
}

- (void)scrollWheel:(NSEvent *)theEvent
{
	NSPoint pt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	self.pSceneView->Zoom(pt.x, self.bounds.size.height - pt.y, theEvent.deltaY);
	if (self.pSceneView->ShouldRedraw())
        [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect
{
	pSceneView->Draw();

	[self.openGLContext flushBuffer];
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
    NSArray *draggedFilenames = [[sender draggingPasteboard] propertyListForType:NSFilenamesPboardType];

	for (id filename in draggedFilenames)
	{
		NSString * extension = [[filename pathExtension] lowercaseString];
		if ([self.modelFileTypes containsObject:extension] || [self.imageFileTypes containsObject:extension])
			return NSDragOperationGeneric;
	}

	return NSDragOperationNone;
}

- (BOOL)performDragOperation:(id < NSDraggingInfo >)sender
{
    NSArray *draggedFilenames = [[sender draggingPasteboard] propertyListForType:NSFilenamesPboardType];

	for (id filename in draggedFilenames)
	{
		NSString * extension = [[filename pathExtension] lowercaseString];
		if ([self.modelFileTypes containsObject:extension])
		{
			pSceneView->AppendModel([filename UTF8String]);
			return YES;
		}
		if ([self.imageFileTypes containsObject:extension])
		{
			pSceneView->SetEnvironmentImage([filename UTF8String]);
			return YES;
		}
	}

	return NO;
}

- (IBAction)onFileNew:(id)sender
{
	self.filename = nil;
	pSceneView->ResetScene();
}

- (IBAction)onFileOpen:(id)sender
{
	NSOpenPanel* panel = [NSOpenPanel openPanel];
	[panel setTitle:@"Open"];
	[panel setCanChooseFiles:YES];
	[panel setCanChooseDirectories:NO];
	[panel setAllowedFileTypes:self.modelFileTypes];

	if ([panel runModal] == NSOKButton)
	{
		for (NSURL* url in [panel URLs])
		{
			if ([[url pathExtension] compare:@"mrs"] == NSOrderedSame)
			{
				self.filename = [url path];
				self.pSceneView->LoadScene([self.filename UTF8String]);
			}
			else
				self.pSceneView->AppendModel([[url path] UTF8String]);
			break;
		}
	}
}

- (IBAction)onSave:(id)sender
{
	if (self.filename != nil)
		self.pSceneView->SaveScene([self.filename UTF8String]);
	else
		[self onSaveAs:sender];
}

- (IBAction)onSaveAs:(id)sender
{
	NSSavePanel* panel = [NSSavePanel savePanel];
	[panel setTitle:@"Save As..."];
	[panel setNameFieldStringValue:@"untitled"];
	[panel setAllowedFileTypes:[NSArray arrayWithObjects: @"mrs", nil]];

	if ([panel runModal] == NSOKButton)
	{
		self.filename = [[panel URL] path];
		self.pSceneView->SaveScene([self.filename UTF8String]);
	}
}

- (IBAction)onEnvironmentImage:(id)sender
{
	NSOpenPanel* panel = [NSOpenPanel openPanel];
	[panel setTitle:@"Environment Image"];
	[panel setCanChooseFiles:YES];
	[panel setCanChooseDirectories:NO];
	[panel setAllowedFileTypes:self.imageFileTypes];

	if ([panel runModal] == NSOKButton)
	{
		for (NSURL* url in [panel URLs])
		{
			self.pSceneView->SetEnvironmentImage([[url path] UTF8String]);
			break;
		}
	}
}

- (IBAction)onSaveImage:(id)sender
{
	NSSavePanel* panel = [NSSavePanel savePanel];
	[panel setTitle:@"Save Image"];
	[panel setNameFieldStringValue:@"untitled.png"];
	[panel setAllowedFileTypes:self.imageFileTypes];
	
	if ([panel runModal] == NSOKButton)
		self.pSceneView->SaveImage([[[panel URL] path] UTF8String]);
}

- (IBAction)onResetCamera:(id)sender
{
	self.pSceneView->ResetCamera();
}

- (IBAction)onShowGrid:(id)sender
{
	self.pSceneView->SetShowGrid(!self.pSceneView->ShowGrid());
}

- (IBAction)onShowWireframe:(id)sender
{
	self.pSceneView->SetShowWireframe(!self.pSceneView->ShowWireframe());
}

- (IBAction)onShowNormals:(id)sender
{
	self.pSceneView->SetShowNormals(!self.pSceneView->ShowNormals());
}

- (IBAction)onShowBVH:(id)sender
{
	self.pSceneView->SetShowBVH(!self.pSceneView->ShowBVH());
}

- (IBAction)onOpenGLMode:(id)sender
{
	self.pSceneView->SetRenderMode(mr::SceneView::RM_OPENGL);
}

- (IBAction)onSoftwareMode:(id)sender
{
	self.pSceneView->SetRenderMode(mr::SceneView::RM_SOFTWARE);
}

- (IBAction)onOpenCLMode:(id)sender
{
	self.pSceneView->SetRenderMode(mr::SceneView::RM_OPENCL);
}

- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item
{
	NSMenuItem *it = (id)item;
	
	SEL s = item.action;
	if (s == @selector(onShowGrid:))
		it.state = self.pSceneView->ShowGrid() ? NSOnState : NSOffState;
	else if (s == @selector(onShowWireframe:))
		it.state = self.pSceneView->ShowWireframe() ? NSOnState : NSOffState;
	else if (s == @selector(onShowNormals:))
		it.state = self.pSceneView->ShowNormals() ? NSOnState : NSOffState;
	else if (s == @selector(onShowBVH:))
		it.state = self.pSceneView->ShowBVH() ? NSOnState : NSOffState;
	else if (s == @selector(onOpenGLMode:))
		it.state = self.pSceneView->RenderMode() == mr::SceneView::RM_OPENGL ? NSOnState : NSOffState;
	else if (s == @selector(onSoftwareMode:))
		it.state = self.pSceneView->RenderMode() == mr::SceneView::RM_SOFTWARE ? NSOnState : NSOffState;
	else if (s == @selector(onOpenCLMode:))
		it.state = self.pSceneView->RenderMode() == mr::SceneView::RM_OPENCL ? NSOnState : NSOffState;
	
	return YES;
}

@end
