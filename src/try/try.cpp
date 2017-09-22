// try.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "try.h"

#ifndef   WS_EX_LAYERED 
#define   WS_EX_LAYERED                       0x00080000
#define   LWA_COLORKEY                         0x00000001 
#define   LWA_ALPHA                               0x00000002 
#endif   //   ndef   WS_EX_LAYERED 


void SetBackground(HWND m_hWnd,const TCHAR* pBackImgFullPath)
{
	//加载图片
	Gdiplus::Image* pImage = Gdiplus::Image::FromFile(pBackImgFullPath);
	if (pImage==NULL)
	{
		assert(false && _T("背景图片打开失败!"));
	}

	RECT windowRect;
	GetWindowRect(m_hWnd,&windowRect);
	SIZE sizeWindow;
	if (windowRect.left==windowRect.right)
	{
		sizeWindow.cx=pImage->GetWidth();
		sizeWindow.cy=pImage->GetHeight();
	}else
	{
		sizeWindow.cx=windowRect.right-windowRect.left;
		sizeWindow.cy=windowRect.bottom-windowRect.top;
	}

	HDC hDC = ::GetDC(m_hWnd);
	HDC hdcMemory = CreateCompatibleDC(hDC);
	RECT rcWindow;
	GetWindowRect(m_hWnd,&rcWindow);

	BITMAPINFOHEADER stBmpInfoHeader = { 0 };   
	int nBytesPerLine = ((sizeWindow.cx * 32 + 31) & (~31)) >> 3;
	stBmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);   
	stBmpInfoHeader.biWidth = sizeWindow.cx;   
	stBmpInfoHeader.biHeight = sizeWindow.cy;   
	stBmpInfoHeader.biPlanes = 1;   
	stBmpInfoHeader.biBitCount = 32;   
	stBmpInfoHeader.biCompression = BI_RGB;   
	stBmpInfoHeader.biClrUsed = 0;   
	stBmpInfoHeader.biSizeImage = nBytesPerLine * sizeWindow.cy;   

	PVOID pvBits = NULL;   
	HBITMAP hbmpMem = ::CreateDIBSection(NULL, (PBITMAPINFO)&stBmpInfoHeader, DIB_RGB_COLORS, &pvBits, NULL, 0);
	assert(hbmpMem != NULL);

	HGDIOBJ hbmpOld = ::SelectObject( hdcMemory, hbmpMem);
	POINT ptWinPos = { rcWindow.left, rcWindow.top };

	Gdiplus::Graphics graph(hdcMemory);
	graph.SetSmoothingMode(Gdiplus::SmoothingModeNone);
	graph.DrawImage(pImage, 0, 0, sizeWindow.cx, sizeWindow.cy);
	graph.FillRectangle(&SolidBrush(Color::Gray),0,0,100,50); //用GDI+在画布上画图

	HMODULE hFuncInst = LoadLibrary(_T("User32.DLL"));
	typedef BOOL (WINAPI *MYFUNC)(HWND, HDC, POINT*, SIZE*, HDC, POINT*, COLORREF, BLENDFUNCTION*, DWORD);          
	MYFUNC UpdateLayeredWindow;
	UpdateLayeredWindow = (MYFUNC)::GetProcAddress(hFuncInst, "UpdateLayeredWindow");
	POINT ptSrc = { 0, 0};
	BLENDFUNCTION blendFunc;
	blendFunc.BlendOp = 0;
	blendFunc.BlendFlags = 0;
	blendFunc.AlphaFormat = 1;
	blendFunc.SourceConstantAlpha = 255;//AC_SRC_ALPHA
	//不会发送 WM_SIZE和WM_MOVE消息
	if(!UpdateLayeredWindow(m_hWnd, hDC, &ptWinPos, &sizeWindow, hdcMemory, &ptSrc, 0, &blendFunc, ULW_ALPHA))
	{
		assert(L"UpdateLayeredWindow 调用失败");
		TCHAR tmp[255] = {_T('\0')};
	}


	delete pImage;
	graph.ReleaseHDC(hdcMemory);
	::SelectObject( hdcMemory, hbmpOld); 

	::DeleteObject(hFuncInst);
	::DeleteObject(hbmpOld);
	::DeleteObject(hbmpMem); 
	::DeleteDC(hdcMemory);
	::DeleteDC(hDC);

}

LRESULT CALLBACK WindowProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam){
	switch(uMsg){
	case WM_DESTROY:
		PostQuitMessage(100);
		break;
	case WM_CREATE:
		{
		LONG styleValue = ::GetWindowLong(hwnd, GWL_STYLE);
		styleValue &= ~WS_CAPTION;
		styleValue &= ~WS_MAXIMIZEBOX; 
		styleValue &= ~WS_MINIMIZEBOX; 
		styleValue &= ~WS_THICKFRAME; 
		styleValue &= ~WS_BORDER; 
		styleValue &= ~WS_CAPTION;
		::SetWindowLong(hwnd, GWL_STYLE, styleValue | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

		PostMessageW(hwnd,WM_PAINT,NULL,NULL);
		}
		break;
	case WM_PAINT:
		SetBackground(hwnd,L"bg.png");
		break;
	case WM_LBUTTONDOWN:
		SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
		break;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}

BOOL Register(WNDPROC fWndProc,HINSTANCE hInstance,LPCTSTR szClassName){
	WNDCLASSEX wce={0};
	wce.cbSize=sizeof(wce);
	wce.style=CS_HREDRAW|CS_VREDRAW;
	wce.lpfnWndProc=fWndProc;
	wce.cbClsExtra=0;
	wce.cbWndExtra=0;
	wce.hInstance=hInstance;
	wce.hIcon=NULL;
	wce.hCursor=LoadCursor(NULL,IDC_ARROW);
	wce.hbrBackground=(HBRUSH)(6);//(HBRUSH)(COLOR_WINDOW+1);
	wce.lpszMenuName=NULL;
	wce.lpszClassName=szClassName;
	wce.hIconSm=NULL;
	ATOM nAtom=RegisterClassEx(&wce);
	if(nAtom==0) return false;
	return true;
}

HWND Create(LPCTSTR lpClassName,LPCTSTR lpWindowName,HINSTANCE hInstance){
	HWND m_hWnd = ::CreateWindowEx(WS_EX_LAYERED, L"transparent", _T(""),WS_POPUPWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 
		CW_USEDEFAULT, CW_USEDEFAULT, 
		NULL, NULL, (HINSTANCE)::GetModuleHandle(NULL), 0);

	if(m_hWnd == NULL || !::IsWindow(m_hWnd))
		return NULL;

	return m_hWnd;

}

void Display(HWND hwnd){
	ShowWindow(hwnd,SW_SHOWNORMAL);    
	UpdateWindow(hwnd);
}

void Message(){
	MSG msg={0};
	while(GetMessage(&msg,NULL,0,0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
ULONG_PTR gdiplusToken = 0;
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd){
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	Register(WindowProc,hInstance,L"transparent");
	HWND hwnd=Create(L"transparent",L"TEST",hInstance);
	Display(hwnd);
	Message();

	Gdiplus::GdiplusShutdown(gdiplusToken);
	return 0;
}
