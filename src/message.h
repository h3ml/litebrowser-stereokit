#pragma once

typedef int XINSTANCE;

typedef struct tagRECTX {
	LONG    left;
	LONG    top;
	LONG    right;
	LONG    bottom;
	LONG    front;
	LONG    back;
} RECTX, * PRECTX, NEAR* NPRECTX, FAR* LPRECTX;
#define RECTX_EMPTY(rc) (rc.left == rc.top == rc.front == rc.right == rc.bottom == rc.back)

typedef struct tagPAINTSTRUCTX {
	HDC         hdc;
	BOOL        fErase;
	RECTX       rcPaint;
	BOOL        fRestore;
	BOOL        fIncUpdate;
	BYTE        rgbReserved[8];
} PAINTSTRUCTX, * PPAINTSTRUCTX, * NPPAINTSTRUCTX, * LPPAINTSTRUCTX;

typedef struct tagWND {
	struct tagWND* parent;
	LPCWSTR			name;
	RECT			size;
	sk::pose_t		pose;
	sk::bounds_t	bounds;
	float			aspect;
	WNDPROC			wndproc;
	BOOL			bErase;
	RECTX			rcDirty;
	std::map<std::wstring, LPVOID> props;
	inline sk::vec2 size2() { return { l_itof(size.left), l_itof(size.top) }; }
	inline sk::vec3 size3() { return { l_itof(size.left), l_itof(size.top), l_itof(size.right) }; }
} WND, * XWND;

typedef struct tagTOUCHPOINT {
	float			x;
	float			y;
	float			z;
	float			rx;
	float			ry;
} TOUCHPOINT, * PTOUCHPOINT, * NPTOUCHPOINT, * LPTOUCHPOINT;

#define GET_X_LPARAMX(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAMX(lp) ((int)(short)HIWORD(lp))

BOOL IsWindowX(XWND hWnd);
XWND CreateWindowX(LPCWSTR lpWindowName, int x, int y, int z, int nWidth, int nHeight, int nDepth, XWND hWndParent, LPVOID hMenu, XINSTANCE hInstance, LPVOID lpParam, WNDPROC wndproc);
BOOL GetClientRectX(XWND hWnd, RECTX* rect);
BOOL SetWindowPosX(XWND hWnd, XWND hWndInsertAfter, int x, int y, int z, int cx, int cy, int cz, UINT uFlags);
XWND SetFocusX(XWND hWnd);
BOOL InvalidateRectX(XWND hWnd, const RECTX* lpRect, BOOL bErase);
BOOL UpdateWindowX(XWND hWnd);
BOOL ShowWindowX(XWND hWnd, int nCmdShow);
int  MessageBoxX(XWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);
BOOL ShowScrollBarX(XWND hWnd, int wBar, BOOL bShow);
int  SetScrollInfoX(XWND hwnd, int nBar, LPCSCROLLINFO lpsi, BOOL redraw);
int  SetScrollPosX(XWND hWnd, int nBar, int nPos, BOOL bRedraw);
BOOL SetWindowTextX(XWND hwnd, LPCWSTR lpString);
XWND GetParentX(XWND hWnd);
BOOL MoveWindowX(XWND hWnd, int x, int y, int z, int nWidth, int nHeight, int nDepth, BOOL bRepaint);
LRESULT SendMessageX(XWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT SendMessageX(XWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL PostMessageX(XWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL OpenClipboardX(XWND hWndNewOwner);
HANDLE GetPropX(XWND hWnd, LPCWSTR lpString);
BOOL SetPropX(XWND hWnd, LPCWSTR lpString, HANDLE hData);
HANDLE RemovePropX(XWND hWnd, LPCWSTR lpString);
HWND SetCaptureX(XWND hWnd);
BOOL ReleaseCaptureX();
LRESULT DefWindowProcX(XWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// paint
HDC BeginPaintX(XWND hWnd, LPPAINTSTRUCTX lpPaint);
BOOL EndPaintX(XWND hWnd, const PAINTSTRUCTX* lpPaint);

// rect
BOOL IntersectRectX(LPRECTX lprcDst, const RECTX* lprcSrc1, const RECTX* lprcSrc2);
BOOL UnionRectX(LPRECTX lprcDst, const RECTX* lprcSrc1, const RECTX* lprcSrc2);
BOOL GetUpdateRectX(XWND hWnd, LPRECTX lpRect, BOOL bErase);

// update
BOOL EvaluteWndX(XWND hWnd, sk::bounds_t* bounds);
BOOL ReserveLayoutX(XWND hWnd, sk::bounds_t* bounds);
BOOL GetTouchPointX(XWND hWnd, sk::bounds_t* bounds, sk::handed_ hand, LPTOUCHPOINT lpPoint);