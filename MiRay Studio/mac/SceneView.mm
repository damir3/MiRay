//
//  SceneView.m
//  MiRay Studio
//
//  Created by Damir Sagidullin on 28.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

#import "SceneView.h"

@interface SceneView()

@property NSPoint ptPrevMouse;
@property NSMenu *materialsSubMenu;

@end

@implementation SceneView

- (void)awakeFromNib
{
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

	self.materialsSubMenu = [[NSMenu alloc] init];
	[self createMetarialsMenuRecursive:self.materialsSubMenu atPath:[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"Materials"]];

	self.pSceneView = new mr::SceneView([[[NSBundle mainBundle] resourcePath] UTF8String]);
}

- (void)createMetarialsMenuRecursive: (NSMenu *)menu atPath:(NSString *)path
{
	NSFileManager *fileManager = [[NSFileManager alloc] init];
	NSArray *dirContents = [fileManager contentsOfDirectoryAtPath:path error:nil];
	int index = 0;
	for (NSString *file : dirContents)
	{
		NSString *filePath = [path stringByAppendingPathComponent:file];
		BOOL isDirectory = NO;
		BOOL isFileExists = [fileManager fileExistsAtPath:filePath isDirectory:&isDirectory];
		if (isDirectory)
		{
			NSMenuItem *item = [menu insertItemWithTitle:file action:nil keyEquivalent:@"" atIndex:index++];
			item.submenu = [[NSMenu alloc] init];
			[self createMetarialsMenuRecursive:item.submenu atPath:filePath];
		}
		else if (isFileExists && [[file pathExtension] caseInsensitiveCompare:@"xml"] == NSOrderedSame)
		{
			[menu insertItemWithTitle:[file stringByDeletingPathExtension] action:@selector(onSetMaterial:) keyEquivalent:filePath atIndex:index++];
		}
	}
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
	if (theEvent.clickCount == 1)
	{
		NSPoint pt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
		self.pSceneView->OnMouseClick(pt.x, self.bounds.size.height - pt.y, mr::MOUSE_RIGHT);

		if (self.pSceneView->SetSelection(pt.x, self.bounds.size.height - pt.y, nullptr))
		{
			[self setNeedsDisplay:YES];
			NSMenu *menu = [[NSMenu alloc] init];
			NSMenuItem *menuItem = [menu insertItemWithTitle:@"Material" action:nil keyEquivalent:@"" atIndex:0];
			menuItem.submenu = self.materialsSubMenu;
			[menu insertItem:[NSMenuItem separatorItem] atIndex:1];
			[menu insertItemWithTitle:@"Delete Object" action:@selector(onDeleteObject:) keyEquivalent:@"" atIndex:2];
			[NSMenu popUpContextMenu:menu withEvent:theEvent forView:self];
		}
	}
}

- (void)onSetMaterial:(NSMenuItem *) sender
{
	NSLog(@"%@", sender.keyEquivalent);
	self.pSceneView->SetSelectionMaterial(sender.keyEquivalent.UTF8String);
}

- (void)onDeleteObject:(id) sender
{
	self.pSceneView->DeleteSelection();
}

- (void)otherMouseDown:(NSEvent *)theEvent
{
	NSPoint pt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	self.pSceneView->OnMouseDown(pt.x, self.bounds.size.height - pt.y, mr::MOUSE_MIDDLE);
}

- (void)otherMouseUp:(NSEvent *)theEvent
{
	self.pSceneView->OnMouseUp(mr::MOUSE_MIDDLE);
	if (theEvent.clickCount == 1)
	{
		NSPoint pt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
		self.pSceneView->OnMouseClick(pt.x, self.bounds.size.height - pt.y, mr::MOUSE_MIDDLE);
	}
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
	self.pSceneView->Draw();

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
			self.pSceneView->AppendModel([filename UTF8String]);
			return YES;
		}
		if ([self.imageFileTypes containsObject:extension])
		{
			self.pSceneView->SetEnvironmentImage([filename UTF8String]);
			return YES;
		}
	}

	return NO;
}

@end
