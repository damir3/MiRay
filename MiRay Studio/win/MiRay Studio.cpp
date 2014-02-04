//
//  MiRay Studio.cpp
//  MiRay/MiRay Studio/win
//
//  Created by Damir Sagidullin on 17.06.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#include "MiRay Studio.h"
#include "../../ui/SceneView.h"
#include <direct.h>
#include <commdlg.h>
#include <shellapi.h>
#include <io.h>
#include <fcntl.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "OpenCL.lib")
#pragma comment(lib, "libfbxsdk.lib")
#pragma comment(lib, "FreeImage.lib")

#define MAX_LOADSTRING 100

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif

// Global Variables:
HINSTANCE		g_hInst;								// current instance
TCHAR			g_szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR			g_szWindowClass[MAX_LOADSTRING];			// the main window class name
HDC				g_hDC = NULL;
HGLRC			g_hRC = NULL;
mr::SceneView *	g_pSceneView = NULL;
std::string		g_fileName;
POINT			g_ptMousePos;
bool			g_bLeftMouseDown = false;
bool			g_bRightMouseDown = false;
bool			m_bMouseMoved = false;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

void SetStdOutToNewConsole()
{
	// allocate a console for this app
	AllocConsole();

	// redirect unbuffered STDOUT to the console
	long lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	int hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	FILE * fp = _fdopen( hConHandle, "w" );
	*stdout = *fp;

	setvbuf( stdout, NULL, _IONBF, 0 );
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	bool bConsole = false;
	int numArgs;
	LPWSTR * ppArgs = CommandLineToArgvW(lpCmdLine, &numArgs);
	for (int i = 0; i < numArgs; i++)
	{
		if (!wcscmp(ppArgs[i], L"-c"))
			bConsole = true;
	}

	if (bConsole)
		SetStdOutToNewConsole();

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, g_szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_MIRAYSTUDIO, g_szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
		return FALSE;

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MIRAYSTUDIO));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if (g_pSceneView)
	{
		g_pSceneView->Done();
		delete g_pSceneView;
	}

	return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(NULL, IDI_APPLICATION);//LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MIRAYSTUDIO));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_MIRAYSTUDIO);
	wcex.lpszClassName	= g_szWindowClass;
	wcex.hIconSm		= LoadIcon(NULL, IDI_APPLICATION);//LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

bool InitOpenGL(HWND hWnd)
{
	g_hDC = GetDC(hWnd);

	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16;
	pfd.iLayerType = PFD_MAIN_PLANE;
	int iFormat = ChoosePixelFormat(g_hDC, &pfd);
	SetPixelFormat(g_hDC, iFormat, &pfd);

	g_hRC = wglCreateContext(g_hDC);

	wglMakeCurrent(g_hDC, g_hRC);

	GLuint err = glewInit();
	if (GLEW_OK != err)
	{
		printf("glewInit failed:");
		printf(reinterpret_cast<const char *>(glewGetErrorString(err)));
		return false;
	}

	char path[FILENAME_MAX];
	GetModuleFileNameA(g_hInst, path, _countof(path));
	path[sizeof(path) - 1] = '\0';
	char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
	_splitpath_s(path, drive, _countof(drive), dir, _countof(dir), fname, _countof(fname), ext, _countof(ext));
	strcpy_s(path, _countof(path), drive);
	strcat_s(path, _countof(path), dir);

	g_pSceneView = new mr::SceneView(path);
	if (!g_pSceneView)
		return false;

	if (!g_pSceneView->Init())
		return false;

	g_pSceneView->LoadScene((path + std::string("cup.mrs")).c_str());

	return true;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	g_hInst = hInstance; // Store instance handle in our global variable

	RECT rc = {0, 0, 800, 600};
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, TRUE);

	HWND hWnd = CreateWindow(g_szWindowClass, g_szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
		NULL, NULL, hInstance, NULL);
	if (!hWnd)
		return FALSE;

	if (!InitOpenGL(hWnd))
		return FALSE;

	DragAcceptFiles(hWnd, TRUE);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	SetTimer(hWnd, NULL, 100, (TIMERPROC)NULL);

	return TRUE;
}

bool CheckSceneExtension(const char * extension)
{
	return !_stricmp("mrs", extension);
}

bool CheckModelExtension(const char * extension)
{
	static const char * modelFileTypes[] = {"fbx", "dae", "dxf", "obj", "3ds"};
	for (size_t i = 0; i < _countof(modelFileTypes); i++)
	{
		if (!_stricmp(modelFileTypes[i], extension))
			return true;
	}
	return false;
}

bool CheckImageExtension(const char * extension)
{
	static const char * imageFileTypes[] = {"jpg", "jpeg", "png", "tga", "hdr", "exr", "tif", "tiff", "psd", "dds", "bmp", "raw", "gif", "ico", "pcx", "pict", "crw", "cr2", "nef", "raf", "dng", "mos", "kdc", "dcr"};
	for (size_t i = 0; i < _countof(imageFileTypes); i++)
	{
		if (!_stricmp(imageFileTypes[i], extension))
			return true;
	}
	return false;
}

void OnFileNew(HWND hWnd)
{
	g_fileName.clear();
	g_pSceneView->ResetScene();
}

void OnFileOpen(HWND hWnd)
{
	OPENFILENAMEA ofn;
	char fileName[MAX_PATH] = "";
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "MiRay Scene Files\0*.mrs\0003D Model Files\0*.mrs;*.fbx;*.dae;*.dxf;*.obj;*.3ds\0All Files\0*.*\0\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "";
	if (GetOpenFileNameA(&ofn))
	{
		wglMakeCurrent(g_hDC, g_hRC);
		const char * p = strrchr(fileName, '.');
		const char * extension = p ? ++p : fileName;
		if (CheckSceneExtension(extension))
		{
			g_fileName = fileName;
			g_pSceneView->LoadScene(g_fileName.c_str());
		}
		else if (CheckModelExtension(extension))
			g_pSceneView->AppendModel(fileName);
		wglMakeCurrent(NULL, NULL);
	}
}

void OnFileSaveAs(HWND hWnd)
{
	OPENFILENAMEA ofn;
	char fileName[MAX_PATH];
	strcpy_s(fileName, _countof(fileName), g_fileName.empty() ? "untitled.mrs" : g_fileName.c_str());
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lpstrTitle = "Save Scene";
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "MiRay Scene Files\0*.mrs\0\0";
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = "";
	if (GetSaveFileNameA(&ofn))
	{
		g_fileName = fileName;
		g_pSceneView->SaveScene(g_fileName.c_str());
	}
}

void OnFileSave(HWND hWnd)
{
	if (g_fileName.empty())
		OnFileSaveAs(hWnd);
	else
		g_pSceneView->SaveScene(g_fileName.c_str());
}

static const char * strImageFilter = "Image Files\0*.jpg;*.;*.jpeg;*.png;*.tga;*.hdr;*.exr;*.tif;*.tiff;*.psd;*.dds;"\
	"*.bmp;*.raw;*.gif;*.ico;*.pcx;*.pict;*.crw;*.cr2;*.nef;*.raf;*.dng;*.mos;*.kdc;*.dcr\0All files\0*.*\0\0";

void OnEnvironmentImage(HWND hWnd)
{
	OPENFILENAMEA ofn;
	char fileName[MAX_PATH] = "";
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lpstrTitle = "Environment Image";
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = strImageFilter;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "";
	if (GetOpenFileNameA(&ofn))
		g_pSceneView->SetEnvironmentImage(fileName);
}

void OnSaveImage(HWND hWnd)
{
	OPENFILENAMEA ofn;
	char fileName[MAX_PATH] = "untitled.png";
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lpstrTitle = "Save Image";
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = strImageFilter;
	ofn.lpstrFile = fileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = "";
	if (GetSaveFileNameA(&ofn))
		g_pSceneView->SaveImage(fileName);
}

void OnDropFiles(HWND hWnd, HDROP hDrop)
{
	UINT uFile = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, NULL);
	for (UINT i = 0; i < uFile; i++)
	{
		char lpszFile[MAX_PATH] = {0};
		if (DragQueryFileA(hDrop, 0, lpszFile, MAX_PATH))
		{
			wglMakeCurrent(g_hDC, g_hRC);
			const char * p = strrchr(lpszFile, '.');
			const char * extension = p ? ++p : lpszFile;
			if (CheckSceneExtension(extension))
			{
				g_fileName = lpszFile;
				g_pSceneView->LoadScene(g_fileName.c_str());
			}
			else if (CheckModelExtension(extension))
				g_pSceneView->AppendModel(lpszFile);
			else if (CheckImageExtension(extension))
				g_pSceneView->SetEnvironmentImage(lpszFile);
			wglMakeCurrent(NULL, NULL);
		}
	}
	DragFinish(hDrop);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{// Parse the menu selections:
		case ID_ABOUT:						DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About); break;
		case ID_FILE_NEW:					OnFileNew(hWnd); break;
		case ID_FILE_OPEN:					OnFileOpen(hWnd); break;
		case ID_FILE_SAVE:					OnFileSave(hWnd); break;
		case ID_FILE_SAVE_AS:				OnFileSaveAs(hWnd); break;
		case ID_FILE_ENVIRONMENT_IMAGE:		OnEnvironmentImage(hWnd); break;
		case ID_FILE_SAVE_IMAGE:			OnSaveImage(hWnd); break;
		case ID_EXIT:						DestroyWindow(hWnd); break;
		case ID_VIEW_RESET_CAMERA:			g_pSceneView->ResetCamera(); 	break;
		case ID_VIEW_SHOW_GRID:				g_pSceneView->SetShowGrid(!g_pSceneView->ShowGrid()); break;
		case ID_VIEW_SHOW_WIREFRAME:		g_pSceneView->SetShowWireframe(!g_pSceneView->ShowWireframe()); break;
		case ID_VIEW_SHOW_NORMALS:			g_pSceneView->SetShowNormals(!g_pSceneView->ShowNormals()); break;
		case ID_VIEW_SHOW_BHV:				g_pSceneView->SetShowBVH(!g_pSceneView->ShowBVH()); break;
		case ID_MODE_OPENGL:				g_pSceneView->SetRenderMode(mr::SceneView::RM_OPENGL); break;
		case ID_MODE_SOFTWARE:				g_pSceneView->SetRenderMode(mr::SceneView::RM_SOFTWARE); break;
		case ID_MODE_OPENCL:				g_pSceneView->SetRenderMode(mr::SceneView::RM_OPENCL); break;
		default:	
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_SIZE:
		g_pSceneView->Resize(static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)),
			static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)));
		break;
	case WM_LBUTTONDOWN:
		g_bLeftMouseDown = true;
		if (!g_bRightMouseDown)
		{
			SetCapture(hWnd);
			m_bMouseMoved = false;
			g_ptMousePos.x = GET_X_LPARAM(lParam);
			g_ptMousePos.y = GET_Y_LPARAM(lParam);
			g_pSceneView->OnMouseDown((float)g_ptMousePos.x, (float)g_ptMousePos.y, mr::MOUSE_LEFT);
		}
		break;
	case WM_LBUTTONUP:
		g_bLeftMouseDown = false;
		if (!g_bRightMouseDown)
		{
			ReleaseCapture();
			g_pSceneView->OnMouseUp(mr::MOUSE_LEFT);
			if (!m_bMouseMoved)
				g_pSceneView->OnMouseClick((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam), mr::MOUSE_LEFT);
		}
		break;
	case WM_RBUTTONDOWN:
		g_bRightMouseDown = true;
		if (!g_bLeftMouseDown)
		{
			SetCapture(hWnd);
			m_bMouseMoved = false;
			g_ptMousePos.x = GET_X_LPARAM(lParam);
			g_ptMousePos.y = GET_Y_LPARAM(lParam);
			g_pSceneView->OnMouseDown((float)g_ptMousePos.x, (float)g_ptMousePos.y, mr::MOUSE_RIGHT);
		}
		break;
	case WM_RBUTTONUP:
		g_bRightMouseDown = false;
		if (!g_bLeftMouseDown)
		{
			ReleaseCapture();
			g_pSceneView->OnMouseUp(mr::MOUSE_RIGHT);
			if (!m_bMouseMoved)
				g_pSceneView->OnMouseClick((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam), mr::MOUSE_RIGHT);
		}
		break;
	case WM_MOUSEMOVE:
		{
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			float dx = (float)(pt.x - g_ptMousePos.x);
			float dy = (float)(pt.y - g_ptMousePos.y);
			g_ptMousePos = pt;
			m_bMouseMoved = true;
			mr::eMouseButton button = g_bLeftMouseDown ? mr::MOUSE_LEFT : (g_bRightMouseDown ? mr::MOUSE_RIGHT : mr::MOUSE_NONE);
			g_pSceneView->OnMouseMove((float)pt.x, (float)pt.y, dx, dy, button);
			if (g_pSceneView->ShouldRedraw())
				InvalidateRect(hWnd, NULL, FALSE);
		}
		break;
	case WM_MOUSEWHEEL:
		g_pSceneView->Zoom((float)g_ptMousePos.x, (float)g_ptMousePos.y, (float)GET_WHEEL_DELTA_WPARAM(wParam)/(float)WHEEL_DELTA);
		if (g_pSceneView->ShouldRedraw())
			InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_KEYUP:
		if (GetKeyState(VK_CONTROL) & 0x80)
		{
			switch (wParam)
			{
			case 'N': OnFileNew(hWnd); break;
			case 'O': OnFileOpen(hWnd); break;
			case 'S': if (GetKeyState(VK_SHIFT) & 0x80) OnFileSaveAs(hWnd); else OnFileSave(hWnd); break;
			case 'E': OnEnvironmentImage(hWnd); break;
			case 'I': OnSaveImage(hWnd); break;
			case '1': g_pSceneView->SetRenderMode(mr::SceneView::RM_OPENGL); break;
			case '2': g_pSceneView->SetRenderMode(mr::SceneView::RM_SOFTWARE); break;
			case '3': g_pSceneView->SetRenderMode(mr::SceneView::RM_OPENCL); break;
			}
		}
		else
		{
			switch (wParam)
			{
			case 'R': g_pSceneView->ResetCamera(); break;
			case 'G': g_pSceneView->SetShowGrid(!g_pSceneView->ShowGrid()); break;
			case 'W': g_pSceneView->SetShowWireframe(!g_pSceneView->ShowWireframe()); break;
			case 'N': g_pSceneView->SetShowNormals(!g_pSceneView->ShowNormals()); break;
			case 'B': g_pSceneView->SetShowBVH(!g_pSceneView->ShowBVH()); break;
			}
		}
		break;
	case WM_DROPFILES:
		OnDropFiles(hWnd, (HDROP)wParam);
		break;
	case WM_TIMER:
		if (g_pSceneView->ShouldRedraw())
			InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_ERASEBKGND:
		return 1;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		wglMakeCurrent(ps.hdc, g_hRC);

		g_pSceneView->Draw();
		SwapBuffers(ps.hdc);

		wglMakeCurrent(NULL, NULL);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
