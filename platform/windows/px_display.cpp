﻿#include <d2d1.h>
#include <d2d1helper.h>
#include <WindowsX.h>
#pragma comment(lib,"d2d1.lib")

typedef struct
{
	UINT uMsg;
	LPARAM lparam;
	WPARAM wparam;
}WM_MESSAGE;

typedef enum
{
	PX_WINODW_STYLE_NORMAL,
	PX_WINODW_STYLE_DESKTOPSPRITE,
	PX_WINODW_STYLE_SIMPLEWINDOW,
	PX_WINODW_STYLE_FIXED,
	PX_WINODW_STYLE_NOBOARD,
}PX_WINODW_STYLE;
extern "C"
{
	BOOL PX_CreateWindow(int windowWidth, int windowHeight, int surfaceWidth, int surfaceHeight, const char* name);
	BOOL PX_WindowResize(int surfaceWidth, int surfaceHeight, int windowWidth, int windowHeight);

	int  PX_GetScreenHeight();
	int  PX_GetScreenWidth();
	int PX_SetWindowResizeable();

	HWND PX_GetWindowHwnd();
	VOID PX_SystemReadDeviceState();
	BOOL PX_SystemLoop();
	BOOL PX_SystemRender(void* raw, int width, int height);
	BOOL PX_SystemisAvtivated();
	BOOL PX_KeyboardDown(unsigned char X);
	char* PX_KeyboardString();
	char* PX_DragfileString();
	BOOL PX_MouseLButtonDown();
	BOOL PX_MouseRButtonDown();
	BOOL PX_MouseMButtonDown();
	POINT PX_MousePosition();
	BOOL PX_KeyDown(unsigned char key);
	BOOL PX_MouseWheel(int* x, int* y, int* delta);
	BOOL PX_GetWinMessage(WM_MESSAGE * Msg);
	double PX_GetWindowScale();
	void PX_SetWindowStyle(PX_WINODW_STYLE style);
	void PX_SetWindowPosition(int x, int y);
	void PX_SetWindowName(const char* name);
}
 
#define         WIN_MAX_INPUT_STRING_LEN   64
#define         WIN_MAX_INPUT_SPECKEY_LEN  0xff


#define         WIN_KEYDOWN_BACKSPACE    1
#define         WIN_KEYDOWN_RETURN	     2
#define         WIN_KEYDOWN_LEFT         3
#define         WIN_KEYDOWN_RIGHT        4
#define         WIN_KEYDOWN_UP           5
#define         WIN_KEYDOWN_DOWN         6


#define			WIN_MESSAGESTACK_SIZE 64



HWND					Win_Hwnd;
int						Win_Height;
int						Win_Width;
double					Win_Scale;
int						Surface_Height;
int						Surface_Width;
BOOL                    Win_Activated;
PX_WINODW_STYLE			Win_Style;

WM_MESSAGE				Win_messageStack[WIN_MESSAGESTACK_SIZE]={0};
int						Win_messageStackWIndex=0;
int						Win_messageStackRIndex=0;

char				   DInput_AccepyFile[MAX_PATH]={0};
POINT				   DInput_MousePosition;
POINT                  DInput_MouseWheelPosition;
POINT                  DInput_MouseWheelDelta;


unsigned int			D2D_ColorConversion;
ID2D1Bitmap			   *D2D_pSurface;
ID2D1Factory*			D2D_pDirect2dFactory;
ID2D1HwndRenderTarget  *D2D_pRenderTarget;





static WNDCLASSEXA      Win_Wcx;

/////////////////////////////////////////////////////
LRESULT CALLBACK AppWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
/////////////////////////////////////////////////////


void DInput_Update()
{
	POINT point;
	GetCursorPos(&point);
	ScreenToClient(Win_Hwnd,&point);
	DInput_MousePosition=point;
}

BOOL PX_D3DReset(HWND hWnd,int Width,int Height,BOOL bfullScreen)
{
	return TRUE;
}

int  PX_GetScreenHeight()
{
	return GetSystemMetrics(SM_CYSCREEN);
}

int  PX_GetScreenWidth()
{
	return GetSystemMetrics(SM_CXSCREEN);
}

BOOL PX_WindowResize(int surfaceWidth,int surfaceHeight,int windowWidth,int windowHeight)
{
	HRESULT hr;
	D2D1_SIZE_U size;
	RECT rc;

	Win_Width=windowWidth;
	Win_Height=windowHeight;


	GetClientRect(Win_Hwnd, &rc);

	size.width=rc.right - rc.left;
	size.height=rc.bottom - rc.top;

	hr=D2D_pRenderTarget->Resize(size);

	if (FAILED(hr))
	{
		return FALSE;
	}

	size = D2D1::SizeU(
		surfaceWidth,
		surfaceHeight
		);

	Surface_Height=surfaceHeight;
	Surface_Width=surfaceWidth;

	if(D2D_pSurface)
		D2D_pSurface->Release();

	D2D1_BITMAP_PROPERTIES bitmapformat;
	bitmapformat.dpiX=0;
	bitmapformat.dpiY=0;
	bitmapformat.pixelFormat=D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM,D2D1_ALPHA_MODE_IGNORE);
	hr=D2D_pRenderTarget->CreateBitmap(size,bitmapformat,&D2D_pSurface);
	D2D_ColorConversion=0;

	if (FAILED(hr))
	{

		D2D1_BITMAP_PROPERTIES bitmapformat;
		bitmapformat.dpiX=0;
		bitmapformat.dpiY=0;
		bitmapformat.pixelFormat=D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_IGNORE);

		hr=D2D_pRenderTarget->CreateBitmap(size,bitmapformat,&D2D_pSurface);
		D2D_ColorConversion=1;
		if(FAILED(hr))
			return FALSE;
	}

	return TRUE;

}

static BOOL CALLBACK EnumWindowsProc(_In_ HWND hwnd, _In_ LPARAM Lparam)
{
	HWND hDefView = FindWindowEx(hwnd, 0, TEXT("SHELLDLL_DefView"), 0);
	if (hDefView != 0)
	{
		HWND hWorkerw = FindWindowEx(0, hwnd, TEXT("WorkerW"), 0);
		ShowWindow(hWorkerw, SW_HIDE);

		return FALSE;
	}
	return TRUE;
}

BOOL PX_CreateWindow( int surfaceWidth,int surfaceHeight,int windowWidth,int windowHeight,const char *name)
{
	HRESULT hr;
	D2D1_SIZE_U size;
	RECT rc;
	DWORD style;
	int window_x, window_y;
	if (Win_Wcx.cbSize==0)
	{
		////////////////////////////////////////////////////////////////////////////
		// Initialize Window class struct
		///////////////////////////////////////////////////////////////////////////
		Win_Wcx.cbSize        = sizeof(WNDCLASSEX);
		Win_Wcx.style         = CS_CLASSDC;
		Win_Wcx.lpfnWndProc   = AppWindowProc;
		Win_Wcx.cbClsExtra    = 0;
		Win_Wcx.cbWndExtra    = 0;
		Win_Wcx.hInstance     = GetModuleHandle(NULL);
		Win_Wcx.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
		Win_Wcx.hCursor       = LoadCursor(NULL, IDC_ARROW);
		Win_Wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW);
		Win_Wcx.lpszMenuName  = NULL;
		Win_Wcx.lpszClassName = "WindowCls";
		Win_Wcx.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);


		///////////////////////////////////////////////////////////////////////////
		//Class Register
		///////////////////////////////////////////////////////////////////////////
		if(!RegisterClassExA(&Win_Wcx))
			return FALSE;
	}


	Win_Width=windowWidth;
	Win_Height=windowHeight;

	////////////////////////////////////////////////////////////////////////////
	//Create window
	////////////////////////////////////////////////////////////////////////////
	switch (Win_Style)
	{
	case PX_WINODW_STYLE_NOBOARD:
	case PX_WINODW_STYLE_DESKTOPSPRITE:
	{
		style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP;
		window_x = 0;
		window_y = 0;
	}
	break;
	case PX_WINODW_STYLE_SIMPLEWINDOW :
	{
		style = WS_OVERLAPPED | WS_SYSMENU ;
		window_x = CW_USEDEFAULT;
		window_y = CW_USEDEFAULT;
	}
	break;
	case PX_WINODW_STYLE_FIXED:
	{
		style = WS_OVERLAPPED | WS_SYSMENU ;
		window_x = CW_USEDEFAULT;
		window_y = CW_USEDEFAULT;
	}
	break;
	default:
		style = WS_OVERLAPPED | WS_SYSMENU | WS_SIZEBOX;
		window_x = CW_USEDEFAULT;
		window_y = CW_USEDEFAULT;
		break;
	}
	

	
#ifdef _UNICODE
	do 
	{
		WCHAR wname[128]={0};
		MultiByteToWideChar(CP_ACP, 0,name, (int)strlen(name), wname,128);
		Win_Hwnd = CreateWindowA("WindowCls", (LPCSTR)wname,
			style,
			window_x, window_y,
			Win_Width, Win_Height,
			NULL, NULL, GetModuleHandle(NULL), NULL);
	} while (0);
#else
	Win_Hwnd = CreateWindowA("WindowCls", name,
		style,
		window_x, window_y,
		Win_Width, Win_Height,
		NULL, NULL, GetModuleHandle(NULL), NULL);
#endif
	
	

	if(!Win_Hwnd)
	{
		MessageBoxA(0,"create window failed","error",MB_OK);
		return FALSE;
	}
	

	////////////////////////////////////////////////////////////////////////////
	//Show window
	////////////////////////////////////////////////////////////////////////////
	ShowWindow(Win_Hwnd, SW_SHOWNORMAL);
	UpdateWindow(Win_Hwnd);
	//CoInitialize(NULL);
	DragAcceptFiles(Win_Hwnd,TRUE);
	//////////////////////////////////////////////////////////////////////////
	//
	
	if((Win_Hwnd) == NULL)
		return FALSE;

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,&D2D_pDirect2dFactory);
	if (FAILED(hr))
	{
		MessageBoxA(0,"could not create d2d factory","error",MB_OK);
		return FALSE;
	}
	
	GetClientRect(Win_Hwnd, &rc);
	SetWindowPos(Win_Hwnd,0,0,0,windowWidth+windowWidth-(rc.right - rc.left),windowHeight+windowHeight-(rc.bottom - rc.top),SWP_NOMOVE);
	GetClientRect(Win_Hwnd, &rc);

	
	size.width=rc.right - rc.left;
	size.height=rc.bottom - rc.top;

	hr=D2D_pDirect2dFactory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(), 96.0F, 96.0F, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT),
		D2D1::HwndRenderTargetProperties(Win_Hwnd, size),
		&D2D_pRenderTarget
		);

	if (FAILED(hr))
	{
		MessageBoxA(0,"could not create d2d render target","error",MB_OK);
		return FALSE;
	}


	size = D2D1::SizeU(
		surfaceWidth,
		surfaceHeight
		);
	Surface_Height=surfaceHeight;
	Surface_Width=surfaceWidth;



	D2D1_BITMAP_PROPERTIES bitmapformat;
	bitmapformat.dpiX=0;
	bitmapformat.dpiY=0;
	bitmapformat.pixelFormat=D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM,D2D1_ALPHA_MODE_IGNORE);
	hr=D2D_pRenderTarget->CreateBitmap(size,bitmapformat,&D2D_pSurface);
	D2D_ColorConversion=0;

	if (FAILED(hr))
	{

		D2D1_BITMAP_PROPERTIES bitmapformat;
		bitmapformat.dpiX=0;
		bitmapformat.dpiY=0;
		bitmapformat.pixelFormat=D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_IGNORE);

		hr=D2D_pRenderTarget->CreateBitmap(size,bitmapformat,&D2D_pSurface);
		D2D_ColorConversion=1;
		if(FAILED(hr))
			return FALSE;
	}


	if (FAILED(hr))
	{
		MessageBoxA(0,"could not create d2d bitmap","error",MB_OK);
		return FALSE;
	}

	PX_GetWindowScale();

	do 
	{
		if (Win_Style==PX_WINODW_STYLE_DESKTOPSPRITE)
		{
			STARTUPINFO si = { 0 };
			PROCESS_INFORMATION pi = { 0 };
			HWND hProgman = FindWindow(TEXT("Progman"), 0);
			if (!hProgman)
				break;
			SendMessageTimeout(hProgman, 0x52C, 0, 0, 0, 100, 0);

			SetParent(Win_Hwnd, hProgman);
			EnumWindows(EnumWindowsProc, 0);
		}
	} while (0);
	
	return TRUE;
}
/////////////////////////////////////////////////////////////
char			Win_Str[WIN_MAX_INPUT_STRING_LEN]={0};
char			Win_SpecKey[WIN_MAX_INPUT_SPECKEY_LEN]={0};
int				Win_CurrentIndex=0;
#include "stdio.h"
LRESULT CALLBACK AppWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

// 	if(uMsg==0x246)
// 	printf("%x\n",uMsg);
	WM_MESSAGE message;


	switch(uMsg) 
	{
	case WM_MOUSEMOVE:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		break;
	case WM_KILLFOCUS:
		{
			Win_Activated=FALSE;
		}
		break;
	case WM_SETFOCUS:
		{
			Win_Activated=TRUE;
		}
		break;
	case WM_KEYDOWN:
		{
			Win_SpecKey[wParam]=1;
		}
		break;
	case WM_MOUSEWHEEL:
		{
			DInput_MouseWheelPosition.x=GET_X_LPARAM(lParam); 
			DInput_MouseWheelPosition.y=GET_Y_LPARAM(lParam); 
			DInput_MouseWheelDelta.x=0;
			DInput_MouseWheelDelta.y=GET_WHEEL_DELTA_WPARAM(wParam);
		}
		break;
	case WM_CHAR:
		{
			if(Win_CurrentIndex<WIN_MAX_INPUT_STRING_LEN-1)
			{
				if(wParam>=32||wParam==8)
				{
				Win_Str[Win_CurrentIndex++]=(char)wParam;
				Win_Str[Win_CurrentIndex]='\0';
				}
			}
		}
		break;
	case WM_DROPFILES:
		{
			HDROP hdrop = (HDROP)wParam;
			DragQueryFileA(hdrop, 0, DInput_AccepyFile, MAX_PATH);
			DragFinish(hdrop);
		}
		break;
	case WM_CLOSE:
		{
			PostQuitMessage(0);
			exit(0);
		}
		break;
	case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		break;
	case WM_SIZE:
		{
			
		}
	break;
	default: return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}

	message.uMsg=uMsg;
	message.wparam=wParam;
	message.lparam=lParam;
	
	if (Win_messageStack[Win_messageStackWIndex].uMsg==0)
	{
		Win_messageStack[Win_messageStackWIndex% WIN_MESSAGESTACK_SIZE]=message;
		Win_messageStackWIndex++;
		if (Win_messageStackWIndex>= WIN_MESSAGESTACK_SIZE)
		{
			Win_messageStackWIndex = 0;
		}
	}
	

	return TRUE;
}

BOOL PX_SystemLoop()
{
	MSG Msg;
	ZeroMemory(&Msg, sizeof(MSG));

	if(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) 
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	} 
	else
	{
		Sleep(10);
	}
	return TRUE;
}


BOOL PX_SystemRender(void *raw,int width,int height)
{
	D2D1_RECT_F screenSize;
	D2D1_RECT_U size;
	size.left=0;
	size.right=width;
	size.top=0;
	size.bottom=height;

	screenSize.left=0;
	screenSize.top=0;
	screenSize.right=(FLOAT)(Win_Width/*Win_Scale*/);
	screenSize.bottom=(FLOAT)(Win_Height/*Win_Scale*/);

	D2D_pRenderTarget->BeginDraw();
	D2D_pRenderTarget->Clear();
	if (D2D_ColorConversion)
	{
		int i;
		DWORD *pData=(DWORD *)raw;
		for (i=0;i<width*height;i++)
		{
			pData[i]=(pData[i]&0xFF00FF00)+((pData[i]>>16)&0x000000FF)+((pData[i]<<16)&0x00FF0000);
		}
	}
	if (D2D_pSurface)
	{
		D2D_pSurface->CopyFromMemory(&size, raw, width * sizeof(DWORD));
		D2D_pRenderTarget->DrawBitmap(D2D_pSurface, screenSize);
	}
	
	
	D2D_pRenderTarget->EndDraw();
	
	return TRUE;
}

BOOL PX_SystemisAvtivated()
{
	return Win_Activated;
}

BOOL PX_KeyboardDown(unsigned char X)
{
	return (GetAsyncKeyState(X)&0x8000?TRUE:FALSE);
}

BOOL PX_MouseLButtonDown()
{
	return GetAsyncKeyState(VK_LBUTTON) & 0xFF00;
}

BOOL PX_MouseRButtonDown()
{
	return GetAsyncKeyState(VK_RBUTTON) & 0xFF00;
}

BOOL PX_MouseMButtonDown()
{
	return GetAsyncKeyState(VK_MBUTTON) & 0xFF00;
}

POINT PX_MousePosition()
{
	return DInput_MousePosition;
}

char * PX_KeyboardString()
{
	if(Win_CurrentIndex!=0)
	{
	Win_CurrentIndex=0;
	return Win_Str;
	}
	else
	{
		return 0;
	}
}

VOID PX_SystemReadDeviceState()
{
	DInput_Update();
}

char * PX_DragfileString()
{
	return DInput_AccepyFile;
}

HWND PX_GetWindowHwnd()
{
	return Win_Hwnd;
}

BOOL PX_KeyDown(unsigned char key)
{
	if (Win_SpecKey[key])
	{
		Win_SpecKey[key]=0;
		return TRUE;
	}
	return FALSE;
}

BOOL PX_MouseWheel(int *x,int *y,int *delta)
{
	if (DInput_MouseWheelDelta.y!=0)
	{
		*x=DInput_MouseWheelPosition.x;
		*y=DInput_MouseWheelPosition.y;
		*delta=DInput_MouseWheelDelta.y;
		DInput_MouseWheelDelta.y=0;
		return TRUE;
	}
	return FALSE;
}

BOOL PX_GetWinMessage(WM_MESSAGE *Msg)
{

	if (Win_messageStack[Win_messageStackRIndex].uMsg!=0)
	{
		*Msg = Win_messageStack[Win_messageStackRIndex];
		memset(&Win_messageStack[Win_messageStackRIndex], 0, sizeof(WM_MESSAGE));
		Win_messageStackRIndex++;
		if (Win_messageStackRIndex >= WIN_MESSAGESTACK_SIZE)
		{
			Win_messageStackRIndex = 0;
		}
		return TRUE;
	}

	return FALSE;
}




int PX_SetWindowResizeable()
{
	return SetWindowLong(Win_Hwnd,GWL_STYLE,WS_OVERLAPPED|WS_SYSMENU);
}

double PX_GetWindowScale()
{
	
	HWND hWnd = GetDesktopWindow();
	HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

	
	MONITORINFOEX miex;
	miex.cbSize = sizeof(miex);
	GetMonitorInfo(hMonitor, &miex);
	int cxLogical = (miex.rcMonitor.right - miex.rcMonitor.left);
	int cyLogical = (miex.rcMonitor.bottom - miex.rcMonitor.top);

	
	DEVMODE dm;
	dm.dmSize = sizeof(dm);
	dm.dmDriverExtra = 0;
	EnumDisplaySettings(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm);
	int cxPhysical = dm.dmPelsWidth;
	int cyPhysical = dm.dmPelsHeight;

	
	double horzScale = ((double)cxPhysical / (double)cxLogical);
	double vertScale = ((double)cyPhysical / (double)cyLogical);
	
	Win_Scale=horzScale;
	return horzScale;
}



void PX_SetWindowStyle(PX_WINODW_STYLE style)
{
	Win_Style = style;
}

void PX_SetWindowPosition(int x,int y)
{
	SetWindowPos(Win_Hwnd,HWND_TOPMOST,x,y,0,0,SWP_NOSIZE);
}

void PX_SetWindowName(const char* name)
{
	SetWindowTextA(Win_Hwnd, (LPCSTR)name);
}