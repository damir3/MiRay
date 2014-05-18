//
//  AppController.m
//  MiRay Studio
//
//  Created by Damir Sagidullin on 21/04/14.
//  Copyright (c) 2014 Damir Sagidullin. All rights reserved.
//

#import "AppController.h"
#import "SceneView.h"

@interface AppController()

@property (weak) IBOutlet SceneView *sceneView;
@property (weak) IBOutlet NSTextField *toolbarText;
@property (assign) mr::SceneView *pSceneView;
@property NSArray *modelFileTypes;
@property NSArray *imageFileTypes;

@end

@implementation AppController

- (void)initScene
{
	self.pSceneView = self.sceneView.pSceneView;
	self.modelFileTypes = [NSArray arrayWithObjects: @"mrs", @"fbx", @"dae", @"dxf", @"obj", @"3ds", nil];
	self.imageFileTypes = [NSArray arrayWithObjects: @"png", @"jpg", @"jpeg", @"tga", @"hdr", @"exr", @"tif", @"tiff", @"psd", @"dds", @"bmp", @"raw", @"gif", @"ico", @"pcx", @"pict", @"crw", @"cr2", @"nef", @"raf", @"dng", @"mos", @"kdc", @"dcr", nil];
	self.sceneView.modelFileTypes = self.modelFileTypes;
	self.sceneView.imageFileTypes = self.imageFileTypes;
	
	[self.sceneView.openGLContext makeCurrentContext];
	self.pSceneView->LoadScene([[[NSBundle mainBundle] pathForResource:@"cup" ofType:@"mrs"] UTF8String]);
	[NSOpenGLContext clearCurrentContext];
	
	NSTimer *timer = [NSTimer timerWithTimeInterval:1.0/10.0 target:self selector:@selector(idle:) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
}

- (void)idle:(NSTimer *)timer
{
    if (![[NSApplication sharedApplication] isHidden] && self.pSceneView && self.pSceneView->ShouldRedraw())
	{
//		[self.toolbarText setStringValue:framesCount > 0 ? [NSString stringWithFormat:@"%d frames / %g ms", framesCount, self.pSceneView->FramesRenderTime() * 1000.0 / framesCount] : @""];
		[self updateToolbarText];

		[self.sceneView setNeedsDisplay:YES];
	}
}

- (void)updateToolbarText
{
	NSMutableAttributedString *tableString = [[NSMutableAttributedString alloc] init];
	NSTextTable *table = [[NSTextTable alloc] init];
	[table setNumberOfColumns:3];

	int framesCount = std::max(self.pSceneView->FramesCount(), 0);
	double renderTime = self.pSceneView->FramesRenderTime();
	NSString *cell1 = [NSString stringWithFormat:@"%d frames\n", framesCount];
	int secs = (int)renderTime;
	int minutes = secs / 60;
	int hours = minutes / 60;
	NSString *cell2 = [NSString stringWithFormat:@"%02d:%02d:%02d\n", hours, minutes % 60, secs % 60];
	NSString *cell3 = framesCount > 0 ? [NSString stringWithFormat:@"%g ms\n", self.pSceneView->FramesRenderTime() * 1000.0 / framesCount] : @"\n";

	[tableString appendAttributedString:[self tableCellAttributedStringWithString:cell1
																			table:table
																			  row:0
																		   column:0
																		alignment:NSLeftTextAlignment]];

	[tableString appendAttributedString:[self tableCellAttributedStringWithString:cell2
																			table:table
																			  row:0
																		   column:1
																		alignment:NSCenterTextAlignment]];

	[tableString appendAttributedString:[self tableCellAttributedStringWithString:cell3
																			table:table
																			  row:0
																		   column:2
																		alignment:NSRightTextAlignment]];

	[self.toolbarText setAttributedStringValue:tableString];
	[self.toolbarText setAutoresizesSubviews:YES];
	[self.toolbarText setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
}

- (NSMutableAttributedString *) tableCellAttributedStringWithString:(NSString *)string
															  table:(NSTextTable *)table
																row:(int)row
															 column:(int)column
														  alignment:(NSTextAlignment)alignment

{
	NSTextTableBlock *block = [[NSTextTableBlock alloc] initWithTable:table
														  startingRow:row
															  rowSpan:1
													   startingColumn:column
														   columnSpan:1];

//	[block setWidth:4.0 type:NSTextBlockAbsoluteValueType forLayer:NSTextBlockBorder];
//	[block setWidth:6.0 type:NSTextBlockAbsoluteValueType forLayer:NSTextBlockPadding];

	NSMutableParagraphStyle *paragraphStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
	[paragraphStyle setTextBlocks:[NSArray arrayWithObjects:block, nil]];
	[paragraphStyle setAlignment:alignment];

	NSMutableAttributedString *cellString = [[NSMutableAttributedString alloc] initWithString:string];
	[cellString addAttribute:NSParagraphStyleAttributeName
					   value:paragraphStyle
					   range:NSMakeRange(0, [cellString length])];

	[cellString addAttribute:NSFontAttributeName
					   value:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]
					   range:NSMakeRange(0, [cellString length])];


//	NSRect rc = [cellString boundingRectWithSize:NSMakeSize(CGFLOAT_MAX, CGFLOAT_MAX) options:0];

    return cellString;
}

- (IBAction)onFileNew:(id)sender
{
	self.sceneView.filename = nil;
	[self.sceneView.openGLContext makeCurrentContext];
	self.pSceneView->ResetScene();
	[NSOpenGLContext clearCurrentContext];
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
			[self.sceneView.openGLContext makeCurrentContext];
			if ([[url pathExtension] compare:@"mrs"] == NSOrderedSame)
			{
				self.sceneView.filename = [url path];
				self.pSceneView->LoadScene([self.sceneView.filename UTF8String]);
			}
			else
				self.pSceneView->AppendModel([[url path] UTF8String]);
			
			[NSOpenGLContext clearCurrentContext];
			break;
		}
	}
}

- (IBAction)onFileSave:(id)sender
{
	if (self.sceneView.filename != nil)
		self.pSceneView->SaveScene([self.sceneView.filename UTF8String]);
	else
		[self onFileSaveAs:sender];
}

- (IBAction)onFileSaveAs:(id)sender
{
	NSSavePanel* panel = [NSSavePanel savePanel];
	[panel setTitle:@"Save As..."];
	[panel setNameFieldStringValue:@"untitled"];
	[panel setAllowedFileTypes:[NSArray arrayWithObjects: @"mrs", nil]];
	
	if ([panel runModal] == NSOKButton)
	{
		self.sceneView.filename = [[panel URL] path];
		self.pSceneView->SaveScene([self.sceneView.filename UTF8String]);
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
			[self.sceneView.openGLContext makeCurrentContext];
			self.pSceneView->SetEnvironmentImage([[url path] UTF8String]);
			[NSOpenGLContext clearCurrentContext];
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

- (IBAction)onEditMove:(id)sender {
	self.pSceneView->SetGizmo(mr::GIZMO_MOVE);
}

- (IBAction)onEditRotate:(id)sender {
	self.pSceneView->SetGizmo(mr::GIZMO_ROTATE);
}

- (IBAction)onEditScale:(id)sender {
	self.pSceneView->SetGizmo(mr::GIZMO_SCALE);
}

- (IBAction)onEditDelete:(id)sender {
	self.pSceneView->DeleteSelection();
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
