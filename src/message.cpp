#include "globals.h"
#include "defaults.h"
#include "browser_wnd.h"

const int WNDS_MAX = 5;

static WND wnds[WNDS_MAX]; static int wndi = 0;
LPCWSTR MsgToString(UINT Msg);

BOOL IsWindowX(XWND hWnd) {
	return hWnd ? TRUE : FALSE;
}

XWND CreateWindowX(LPCWSTR lpWindowName, int x, int y, int z, int nWidth, int nHeight, int nDepth, XWND hWndParent, LPVOID hMenu, XINSTANCE hInstance, LPVOID lpParam, WNDPROC wndproc) {
	if (x == CW_USEDEFAULT) { x = 0; y = 0; z = -500; }
	if (nWidth == CW_USEDEFAULT) { nWidth = 800; nHeight = 600; nDepth = 1; }
	//log_infof("CreateWindowX[%ls]: (%d,%d,%d).(%d,%d,%d)", lpWindowName, x, y, z, nWidth, nHeight, nDepth);
	XWND hWnd = &wnds[wndi++];
	hWnd->parent = hWndParent;
	hWnd->name = lpWindowName;
	hWnd->size = { nWidth, nHeight, nDepth, 0 };
	hWnd->pose = {
		{ l_itof(x), l_itof(y), l_itof(z) },
		quat_lookat(vec3_zero, vec3_unit_z)
	};
	hWnd->wndproc = wndproc;
	hWnd->aspect = nHeight / (float)nWidth;
	// send create
	CREATESTRUCT c;
	ZeroMemory(&c, sizeof(c));
	c.lpCreateParams = (browser_wnd*)lpParam;
	c.hInstance = (HINSTANCE)hInstance;
	c.hMenu = (HMENU)hMenu;
	SendMessageX(hWnd, WM_CREATE, NULL, (LPARAM)&c);
	// send size
	SendMessageX(hWnd, WM_SIZE, LOWORD(nDepth), LOWORD(nWidth) | HIWORD(nHeight));
	// invalidate
	InvalidateRectX(hWnd, NULL, FALSE);
	return hWnd;
}

BOOL GetClientRectX(XWND hWnd, RECTX* lpRect) {
	if (!hWnd) return FALSE;
	//log_infof("GetClientRectX[%ls]:", hWnd->name);
	RECT s = hWnd->size;
	pose_t p = hWnd->pose;
	*lpRect = {
		l_ftoi(p.position.x),
		l_ftoi(p.position.y),
		l_ftoi(p.position.x) + s.left,
		l_ftoi(p.position.y) + s.top,
		l_ftoi(p.position.z),
		l_ftoi(p.position.z) + s.right
	};
	return TRUE;
}

BOOL SetWindowPosX(XWND hWnd, XWND hWndInsertAfter, int x, int y, int z, int cx, int cy, int cz, UINT uFlags) {
	//log_infof("SetWindowPosX[%ls]: (%d,%d,%d).(%d,%d,%d) %d", hWnd->name, x, y, z, cx, cy, cz, uFlags);
	pose_t* p = &hWnd->pose;
	p->position.x = l_itof(x);
	p->position.y = l_itof(y);
	//p->position.z = l_itof(z);
	RECT* s = &hWnd->size;
	s->left = cx;
	s->top = cy;
	//s->right = cz;
	return TRUE;
}

XWND SetFocusX(XWND hWnd) {
	//log_infof("SetFocusX[%ls]:", hWnd->name);
	return hWnd;
}

BOOL InvalidateRectX(XWND hWnd, const RECTX* lpRect, BOOL bErase) {
	//log_infof("InvalidateRectX[%ls]: %d", hWnd->name, bErase);
	if (lpRect) UnionRectX(&hWnd->rcDirty, &hWnd->rcDirty, lpRect);
	else {
		hWnd->rcDirty.left = 0;
		hWnd->rcDirty.top = 0;
		hWnd->rcDirty.front = 0;
		hWnd->rcDirty.right = LONG_MAX;
		hWnd->rcDirty.bottom = LONG_MAX;
		hWnd->rcDirty.back = LONG_MAX;
	}
	if (bErase) hWnd->bErase = TRUE;
	return TRUE;
}

BOOL UpdateWindowX(XWND hWnd) {
	//log_infof("UpdateWindowX[%ls]:", hWnd->name);
	if (!GetUpdateRectX(hWnd, NULL, FALSE)) return FALSE;
	SendMessageX(hWnd, WM_PAINT, NULL, NULL);
	return TRUE;
}

BOOL ShowWindowX(XWND hWnd, int nCmdShow) {
	//log_infof("ShowWindowX[%ls]: %d", hWnd->name, nCmdShow);
	return TRUE;
}

int MessageBoxX(XWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType) {
	log_infof("MessageBoxX[%ls]:", hWnd->name);
	return 0;
}

BOOL ShowScrollBarX(XWND hWnd, int wBar, BOOL bShow) {
	//log_infof("ShowScrollBarX[%ls]:", hWnd->name);
	return TRUE;
}

int SetScrollInfoX(XWND hWnd, int nBar, LPCSCROLLINFO lpsi, BOOL redraw) {
	//log_infof("SetScrollInfoX[%ls]:", hWnd->name);
	return 0;
}

int SetScrollPosX(XWND hWnd, int nBar, int nPos, BOOL bRedraw) {
	//log_infof("SetScrollPosX[%ls]:", hWnd->name);
	return 0;
}

BOOL SetWindowTextX(XWND hWnd, LPCWSTR lpString) {
	//log_infof("SetWindowTextX[%ls]: %ls", hWnd->name, lpString);
	return TRUE;
}

XWND GetParentX(XWND hWnd) {
	//log_infof("GetParentX[%ls]:", hWnd->name);
	return hWnd->parent;
}

BOOL MoveWindowX(XWND hWnd, int x, int y, int z, int nWidth, int nHeight, int nDepth, BOOL bRepaint) {
	//log_infof("MoveWindowX[%ls]: (%d,%d,%d).(%d,%d,%d) %d", hWnd->name, x, y, z, nWidth, nHeight, nDepth, bRepaint);
	pose_t* p = &hWnd->pose;
	p->position.x = l_itof(x);
	p->position.y = l_itof(y);
	p->position.z = l_itof(z);
	RECT* s = &hWnd->size;
	s->left = nWidth;
	s->top = nHeight;
	s->right = nDepth;
	if (bRepaint) UpdateWindowX(hWnd);
	return TRUE;
}

LRESULT SendMessageX(XWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	//log_infof("SendMessageX[%ls]: %ls %d %d", hWnd->name, MsgToString(Msg), wParam, lParam);
	return hWnd->wndproc((HWND)hWnd, Msg, wParam, lParam);
}

BOOL PostMessageX(XWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	//log_infof("PostMessageX[%ls]: %ls %d %d", hWnd->name, MsgToString(Msg), wParam, lParam);
	hWnd->wndproc((HWND)hWnd, Msg, wParam, lParam);
	return TRUE;
}

BOOL OpenClipboardX(XWND hWnd) {
	log_infof("OpenClipboardX[%ls]:", hWnd->name);
	return FALSE;
}

HANDLE GetPropX(XWND hWnd, LPCWSTR lpString) {
	//log_infof("GetPropX[%ls]: %ls", hWnd->name, lpString);
	return hWnd->props[lpString];
}

BOOL SetPropX(XWND hWnd, LPCWSTR lpString, HANDLE hData) {
	//log_infof("SetPropX[%ls]: %ls", hWnd->name, lpString);
	hWnd->props[lpString] = hData;
	return TRUE;
}

HANDLE RemovePropX(XWND hWnd, LPCWSTR lpString) {
	//log_infof("RemovePropX[%ls]: %ls", hWnd->name, lpString);
	hWnd->props.erase(lpString);
	return 0;
}

HWND SetCaptureX(XWND hWnd) {
	//log_infof("SetCaptureX[%ls]:", hWnd->name);
	return 0;
}

BOOL ReleaseCaptureX() {
	//log_infof("ReleaseCaptureX:");
	return TRUE;
}

LRESULT DefWindowProcX(XWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	//log_infof("DefWindowProcX[%ls]: %ls %d %d", hWnd->name, MsgToString(Msg), wParam, lParam);
	switch (Msg) {
	case WM_PAINT:
		ZeroMemory(&hWnd->rcDirty, sizeof(RECTX));
		hWnd->bErase = FALSE;
		break;
	}
	return 0;
}

HDC BeginPaintX(XWND hWnd, LPPAINTSTRUCTX lpPaint) {
	//log_infof("BeginPaintX[%ls]:", hWnd->name);
	RECTX rcPaint;
	if (!GetUpdateRectX(hWnd, &rcPaint, FALSE)) return NULL;
	HDC hdc = CreateCompatibleDC(NULL);
	lpPaint->hdc = hdc;
	lpPaint->fErase = hWnd->bErase;
	lpPaint->rcPaint = rcPaint;
	lpPaint->fRestore = false;
	lpPaint->fIncUpdate = false;
	ZeroMemory(&hWnd->rcDirty, sizeof(RECTX));
	hWnd->bErase = FALSE;
	return hdc;
}

BOOL EndPaintX(XWND hWnd, const PAINTSTRUCTX* lpPaint) {
	//log_infof("EndPaintX[%ls]:", hWnd->name);
	DeleteDC(lpPaint->hdc);
	return TRUE;
}

BOOL IntersectRectX(LPRECTX lprcDst, const RECTX* lprcSrc1, const RECTX* lprcSrc2) {
	return IntersectRect((LPRECT)lprcDst, (RECT*)lprcSrc1, (RECT*)lprcSrc2);
}

BOOL UnionRectX(LPRECTX lprcDst, const RECTX* lprcSrc1, const RECTX* lprcSrc2) {
	return UnionRect((LPRECT)lprcDst, (RECT*)lprcSrc1, (RECT*)lprcSrc2);
}

BOOL GetUpdateRectX(XWND hWnd, LPRECTX lpRect, BOOL bErase) {
	//log_infof("GetUpdateRectX[%ls]:", hWnd->name);
	if (!lpRect) return !RECTX_EMPTY(hWnd->rcDirty);
	if (RECTX_EMPTY(hWnd->rcDirty)) return FALSE;
	
	if (bErase) SendMessageX(hWnd, WM_ERASEBKGND, NULL, NULL);
	RECT s = hWnd->size;
	pose_t p = hWnd->pose;
	*lpRect = {
		l_ftoi(p.position.x),
		l_ftoi(p.position.y),
		l_ftoi(p.position.x) + s.left,
		l_ftoi(p.position.y) + s.top,
		l_ftoi(p.position.z),
		l_ftoi(p.position.z) + s.right
	};
	return TRUE;
}

BOOL ReserveLayoutX(XWND hWnd, bounds_t* bounds) {
	//log_infof("ReserveLayoutX[%ls]:", hWnd->name);
	*bounds = ui_layout_reserve(hWnd->size2());
	bounds->center.z += 0.01f;
	bounds->dimensions.z += 0.01f;
	return TRUE;
}

vec2 startAt, prevAt;
BOOL EvaluteWndX(XWND hWnd, bounds_t* bounds) {
	//log_infof("EvaluteWndX[%ls]:", hWnd->name);
	if (bounds) {
		ReserveLayoutX(hWnd, bounds);
		handed_ hand;
		button_state_ state = ui_volumei_at_16((const char16_t*)hWnd->name, *bounds, ui_confirm_push, &hand);
		if (state & button_state_just_active) {
			TOUCHPOINT pt;
			GetTouchPointX(hWnd, bounds, hand, &pt);
			startAt = prevAt = { pt.x, pt.y };
			SendMessageX(hWnd, WM_MOUSEMOVE, LOWORD(pt.z), LOWORD(pt.x) | HIWORD(pt.y));
			//log_infof("TouchStart[%ls]: (%d, %d, %d)", hWnd->name, pt.x, pt.y, pt.z);
		}
		if (state & button_state_active) {
			TOUCHPOINT pt;
			GetTouchPointX(hWnd, bounds, hand, &pt);
			vec2 currAt = { pt.x, pt.y };
			if (!vec2_in_radius(currAt, startAt, 6) && !vec2_in_radius(currAt, prevAt, 1)) {
				prevAt = currAt;
				SendMessageX(hWnd, WM_MOUSEMOVE, LOWORD(pt.z), LOWORD(pt.x) | HIWORD(pt.y));
				//log_infof("TouchMove[%ls]: (%d, %d, %d)", hWnd->name, pt.x, pt.y, pt.z);
			}
		}
		if (state & button_state_just_inactive) {
			TOUCHPOINT pt = { prevAt.x, prevAt.y, 0.0f, 0.0f, 0.0f };
			SendMessageX(hWnd, WM_MOUSELEAVE, LOWORD(pt.z), LOWORD(pt.x) | HIWORD(pt.y));
			//log_infof("TouchEnd[%ls]: (%d, %d, %d)", hWnd->name, pt.x, pt.y, pt.z);
		}
	}
	if (!GetUpdateRectX(hWnd, NULL, FALSE)) return FALSE;
	SendMessageX(hWnd, WM_PAINT, NULL, NULL);
	return TRUE;
}

BOOL GetTouchPointX(XWND hWnd, bounds_t* bounds, handed_ hand, LPTOUCHPOINT lpPoint) {
	const hand_t* h = input_hand(hand);
	const hand_joint_t j = h->fingers[1][4];
	plane_t p = { vec3_forward, -vec3_dot({ bounds->center.x, bounds->center.y, bounds->center.z - bounds->dimensions.z / 2}, vec3_forward) };
	vec3 at = plane_point_closest(p, hierarchy_to_local_point(j.position));

	mesh_draw(defaults::meshSphere, defaults::material, matrix_trs(at, quat_identity, { 0.01f, 0.01f, 0.01f }));

	vec3 pt = (at - (bounds->center + (bounds->dimensions * 0.5f)));
	pt = { -pt.x / bounds->dimensions.x, -pt.y / bounds->dimensions.y, 0.0f };

	RECT s = hWnd->size;
	*lpPoint = {
		pt.x * s.left,
		pt.y * s.top,
		0.0f,
		j.radius,
		j.radius
	};
	return TRUE;
}

LPCWSTR MsgToString(UINT Msg) {
	switch (Msg) {
	case WM_NULL: return L"WM_NULL";
	case WM_CREATE: return L"WM_CREATE";
	case WM_DESTROY: return L"WM_DESTROY";
	case WM_MOVE: return L"WM_MOVE";
	case WM_SIZE: return L"WM_SIZE";
	case WM_ACTIVATE: return L"WM_ACTIVATE";
	case WM_SETFOCUS: return L"WM_SETFOCUS";
	case WM_KILLFOCUS: return L"WM_KILLFOCUS";
		//case WM_ENABLE: return L"WM_ENABLE";
		//case WM_SETREDRAW: return L"WM_SETREDRAW";
		//case WM_SETTEXT: return L"WM_SETTEXT";
		//case WM_GETTEXT: return L"WM_GETTEXT";
		//case WM_GETTEXTLENGTH: return L"WM_GETTEXTLENGTH";
	case WM_PAINT: return L"WM_PAINT";
	case WM_QUIT: return L"WM_QUIT";
		//case WM_ERASEBKGND: return L"WM_ERASEBKGND";
		//case WM_SYSCOLORCHANGE: return L"WM_SYSCOLORCHANGE";
		//case WM_SHOWWINDOW: return L"WM_SHOWWINDOW";
		//case WM_WININICHANGE: return L"WM_WININICHANGE";
	default: return std::to_wstring(Msg).c_str();
	}
}

