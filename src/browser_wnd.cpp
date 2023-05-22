#include "globals.h"
#include "browser_wnd.h"
#include "htmlview_wnd.h"
#include "toolbar_wnd.h"

browser_wnd::browser_wnd(XINSTANCE hInst) {
	m_hInst = hInst;
	m_hWnd = NULL;
#ifndef NO_TOOLBAR
	m_toolbar = new toolbar_wnd(hInst, this);
#endif
	m_view = new htmlview_wnd(hInst, this);
}

browser_wnd::~browser_wnd(void) {
#ifndef NO_TOOLBAR
	if (m_toolbar) delete m_toolbar;
#endif
	if (m_view) delete m_view;
}

void browser_wnd::update() {
	EvaluteWndX(m_hWnd, NULL);
	ui_window_begin("browser", m_hWnd->pose, m_hWnd->size2());
#ifndef NO_TOOLBAR
	m_toolbar->update();
#endif
	m_view->update();
	ui_window_end();
}

LRESULT CALLBACK browser_wnd::WndProc(XWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {
	browser_wnd* pThis = NULL;
	if (IsWindowX(hWnd)) {
		pThis = (browser_wnd*)GetPropX(hWnd, TEXT("browser_this"));
		if (pThis && pThis->m_hWnd != hWnd) {
			pThis = NULL;
		}
	}
	if (pThis || uMessage == WM_CREATE) {
		switch (uMessage) {
		case WM_ERASEBKGND: return TRUE;
		case WM_CREATE: {
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
			pThis = (browser_wnd*)lpcs->lpCreateParams;
			SetPropX(hWnd, TEXT("browser_this"), (HANDLE)pThis);
			pThis->m_hWnd = hWnd;
			pThis->OnCreate();
			break;
		}
		case WM_SIZE:
			pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_DESTROY:
			RemovePropX(hWnd, TEXT("browser_this"));
			pThis->OnDestroy();
			delete pThis;
			return 0;
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
		case WM_ACTIVATE:
			if (LOWORD(wParam) != WA_INACTIVE) {
				SetFocusX(pThis->m_view->wnd());
			}
			return 0;
		}
	}
	return DefWindowProcX(hWnd, uMessage, wParam, lParam);
}

void browser_wnd::OnCreate() {
	RECTX rcClient;
	GetClientRectX(m_hWnd, &rcClient);
#ifndef NO_TOOLBAR
	m_toolbar->create(rcClient.left, rcClient.top, -500, rcClient.right - rcClient.left, m_hWnd);
	m_view->create(rcClient.left, rcClient.top + m_toolbar->height(), -500, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top - m_toolbar->height(), 1, m_hWnd);
#else
	m_view->create(rcClient.left, rcClient.top, -500, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, 1, m_hWnd);
#endif
	SetFocusX(m_view->wnd());
}

void browser_wnd::OnSize(int width, int height) {
	RECTX rcClient;
	GetClientRectX(m_hWnd, &rcClient);
#ifndef NO_TOOLBAR
	int toolbar_height = m_toolbar->set_width(rcClient.right - rcClient.left);
#else
	int toolbar_height = 0;
#endif
	SetWindowPosX(m_view->wnd(), NULL, rcClient.left, rcClient.top + toolbar_height, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top - toolbar_height, 0, SWP_NOZORDER);
	UpdateWindowX(m_view->wnd());
#ifndef NO_TOOLBAR
	SetWindowPosX(m_toolbar->wnd(), NULL, rcClient.left, rcClient.top, 0, rcClient.right - rcClient.left, toolbar_height, 0, SWP_NOZORDER);
	UpdateWindowX(m_toolbar->wnd());
#endif
}

void browser_wnd::OnDestroy() {
}

void browser_wnd::create() {
	m_hWnd = CreateWindowX(L"Light HTML", CW_USEDEFAULT, 0, 0, CW_USEDEFAULT, 0, 0, NULL, NULL, m_hInst, (LPVOID)this, (WNDPROC)browser_wnd::WndProc);
	ShowWindowX(m_hWnd, SW_SHOW);
}

void browser_wnd::open(LPCWSTR path) {
	if (m_view) {
		m_view->open(path, true);
	}
}

void browser_wnd::back() {
	if (m_view) {
		m_view->back();
	}
}

void browser_wnd::forward() {
	if (m_view) {
		m_view->forward();
	}
}

void browser_wnd::reload() {
	if (m_view) {
		m_view->refresh();
	}
}

void browser_wnd::calc_time(int calc_repeat) {
	if (m_view) {
		m_view->render(TRUE, TRUE, calc_repeat);
	}
}

void browser_wnd::calc_redraw(int calc_repeat) {
	if (m_view) {
		m_view->calc_draw(calc_repeat);
	}
}

void browser_wnd::on_page_loaded(LPCWSTR url) {
	if (m_view) {
		SetFocusX(m_view->wnd());
	}
#ifndef NO_TOOLBAR
	m_toolbar->on_page_loaded(url);
#endif
}
