#include "globals.h"
#include "defaults.h"
#include "browser_wnd.h"
#include "htmlview_wnd.h"
#include <strsafe.h>

htmlview_wnd::htmlview_wnd(int hInst, browser_wnd* parent) {
	m_parent = parent;
	m_hInst = hInst;
	m_hWnd = NULL;
	m_top = 0;
	m_left = 0;
	m_max_top = 0;
	m_max_left = 0;
	m_page = NULL;
	m_page_next = NULL;

	InitializeCriticalSection(&m_sync);
}

htmlview_wnd::~htmlview_wnd(void) {
	DeleteCriticalSection(&m_sync);
}

void htmlview_wnd::update() {
	bounds_t bounds;
	EvaluteWndX(m_hWnd, &bounds);
	material_set_texture(m_material, "diffuse", m_tex);
	render_add_mesh(defaults::meshQuad, m_material, matrix_trs(bounds.center + vec3{ 0.0f, 0.0f, -0.015f }, quat_identity, bounds.dimensions));
}

LRESULT CALLBACK htmlview_wnd::WndProc(XWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {
	htmlview_wnd* pThis = NULL;
	if (IsWindowX(hWnd)) {
		pThis = (htmlview_wnd*)GetPropX(hWnd, TEXT("htmlview_this"));
		if (pThis && pThis->m_hWnd != hWnd) {
			pThis = NULL;
		}
	}

	if (pThis || uMessage == WM_CREATE) {
		switch (uMessage) {
		case WM_PAGE_LOADED:
			pThis->OnPageReady();
			return 0;
		case WM_IMAGE_LOADED:
			if (wParam) {
				pThis->redraw(NULL, FALSE);
			}
			else {
				pThis->render();
			}
			break;
		case WM_SETCURSOR:
			pThis->update_cursor();
			break;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CREATE: {
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
			pThis = (htmlview_wnd*)(lpcs->lpCreateParams);
			SetPropX(hWnd, TEXT("htmlview_this"), (HANDLE)pThis);
			pThis->m_hWnd = hWnd;
			pThis->OnCreate();
			break;
		}
		case WM_PAINT: {
			RECTX rcClient;
			GetClientRectX(hWnd, &rcClient);
			pThis->create_dib(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);

			PAINTSTRUCTX ps;
			HDC hdc = BeginPaintX(hWnd, &ps);

			simpledib::dib* dib = &pThis->m_dib;
			pThis->OnPaint(dib, &ps.rcPaint);
			tex_set_colors(pThis->m_tex = pThis->m_texs[pThis->m_texi], dib->width(), dib->height(), dib->bits());
			pThis->m_texi = (pThis->m_texi + 1) % 2;

			EndPaintX(hWnd, &ps);
			return 0;
		}
		case WM_SIZE:
			pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_DESTROY:
			RemovePropX(hWnd, TEXT("htmlview_this"));
			pThis->OnDestroy();
			delete pThis;
			return 0;
		case WM_VSCROLL:
			pThis->OnVScroll(HIWORD(wParam), LOWORD(wParam));
			return 0;
		case WM_HSCROLL:
			pThis->OnHScroll(HIWORD(wParam), LOWORD(wParam));
			return 0;
		case WM_MOUSEWHEEL:
			pThis->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
			return 0;
		case WM_KEYDOWN:
			pThis->OnKeyDown((UINT)wParam);
			return 0;
		case WM_MOUSEMOVE: {
			//TRACKMOUSEEVENT tme;
			//ZeroMemory(&tme, sizeof(TRACKMOUSEEVENT));
			//tme.cbSize = sizeof(TRACKMOUSEEVENT);
			//tme.dwFlags = TME_QUERY;
			//tme.hwndTrack = hWnd;
			//TrackMouseEvent(&tme);
			//if (!(tme.dwFlags & TME_LEAVE)) {
			//	tme.dwFlags = TME_LEAVE;
			//	tme.hwndTrack = hWnd;
			//	TrackMouseEvent(&tme);
			//}
			pThis->OnMouseMove(GET_X_LPARAMX(lParam), GET_Y_LPARAMX(lParam));
			return 0;
		}
		case WM_MOUSELEAVE:
			pThis->OnMouseLeave();
			return 0;
		case WM_LBUTTONDOWN:
			pThis->OnLButtonDown(GET_X_LPARAMX(lParam), GET_Y_LPARAMX(lParam));
			return 0;
		case WM_LBUTTONUP:
			pThis->OnLButtonUp(GET_X_LPARAMX(lParam), GET_Y_LPARAMX(lParam));
			return 0;
		}
	}

	return DefWindowProcX(hWnd, uMessage, wParam, lParam);
}

void htmlview_wnd::OnCreate() {
	m_material = material_copy(defaults::materialUnlit);
	m_texi = 0;
	m_texs[0] = tex_create(tex_type_image_nomips, tex_format_bgra32);
	tex_set_address(m_texs[0], tex_address_clamp);
	m_texs[1] = tex_create(tex_type_image_nomips, tex_format_bgra32);
	tex_set_address(m_texs[1], tex_address_clamp);
}

void htmlview_wnd::OnPaint(simpledib::dib* dib, LPRECTX rcDraw) {
	cairo_surface_t* surface = cairo_image_surface_create_for_data((unsigned char*)dib->bits(), CAIRO_FORMAT_ARGB32, dib->width(), dib->height(), dib->width() * 4);
	cairo_t* cr = cairo_create(surface);

	cairo_rectangle(cr, rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top);
	cairo_clip(cr);

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	lock();

	web_page* page = get_page(false);

	if (page) {

		litehtml::position clip(rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top);
		page->m_doc->draw((litehtml::uint_ptr)cr, -m_left, -m_top, &clip);

		page->release();
	}

	unlock();

	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}

void htmlview_wnd::OnSize(int width, int height) {
	lock();
	web_page* page = get_page(false);
	unlock();

	if (page) {
		page->m_doc->media_changed();
		page->release();
	}

	render();
}

void htmlview_wnd::OnDestroy() {
}

void htmlview_wnd::create(int x, int y, int z, int width, int height, int depth, XWND parent) {
	m_hWnd = CreateWindowX(L"htmlview", x, y, z, width, height, depth, parent, NULL, m_hInst, (LPVOID)this, (WNDPROC)htmlview_wnd::WndProc);
}

void htmlview_wnd::open(LPCWSTR url, bool reload) {
	std::wstring hash;
	std::wstring s_url = url;

	std::wstring::size_type hash_pos = s_url.find_first_of(L'#');
	if (hash_pos != std::wstring::npos) {
		hash = s_url.substr(hash_pos + 1);
		s_url.erase(hash_pos);
	}

	bool open_hash_only = false;

	lock();

	if (m_page) {
		if (m_page->m_url == s_url && !reload) {
			open_hash_only = true;
		}
		else {
			m_page->m_http.stop();
		}
	}
	if (!open_hash_only) {
		if (m_page_next) {
			m_page_next->m_http.stop();
			m_page_next->release();
		}
		m_page_next = new web_page(this);
		m_page_next->m_hash = hash;
		m_page_next->load(s_url.c_str());
	}

	unlock();

	if (open_hash_only) {
		show_hash(hash);
		update_scroll();
		redraw(NULL, FALSE);
		update_history();
	}
}

void htmlview_wnd::calc_draw(int calc_repeat) {
	DWORD tic1 = GetTickCount();
	for (int i = 0; i < calc_repeat; i++) {
		RECTX rcClient;
		GetClientRectX(m_hWnd, &rcClient);
		//rcClient.bottom = rcClient.top + (rcClient.bottom - rcClient.top) / 10;
		redraw(&rcClient, TRUE);
	}
	DWORD tic2 = GetTickCount();
	WCHAR msg[255];
	StringCchPrintf(msg, 255, L"Draw time: %d msec", tic2 - tic1);
	MessageBoxX(m_hWnd, msg, L"litebrowser", MB_ICONINFORMATION | MB_OK);
}

void htmlview_wnd::render(BOOL calc_time, BOOL do_redraw, int calc_repeat) {
	if (!m_hWnd) {
		return;
	}

	web_page* page = get_page();

	if (page) {
		RECTX rcClient;
		GetClientRectX(m_hWnd, &rcClient);

		int width = rcClient.right - rcClient.left;
		int height = rcClient.bottom - rcClient.top;

		if (calc_time) {
			if (calc_repeat <= 0) calc_repeat = 1;
			DWORD tic1 = GetTickCount();
			for (int i = 0; i < calc_repeat; i++) {
				page->m_doc->render(width);
			}
			DWORD tic2 = GetTickCount();
			WCHAR msg[255];
			StringCchPrintf(msg, 255, L"Render time: %d msec", tic2 - tic1);
			MessageBoxX(m_hWnd, msg, L"litebrowser", MB_ICONINFORMATION | MB_OK);
		}
		else {
			page->m_doc->render(width);
		}

		m_max_top = page->m_doc->height() - height;
		if (m_max_top < 0) m_max_top = 0;

		m_max_left = page->m_doc->width() - width;
		if (m_max_left < 0) m_max_left = 0;

		if (do_redraw) {
			update_scroll();
			redraw(NULL, FALSE);
		}

		page->release();
	}
}

void htmlview_wnd::redraw(LPRECTX rcDraw, BOOL update) {
	if (m_hWnd) {
		InvalidateRectX(m_hWnd, rcDraw, TRUE);
		if (update) {
			UpdateWindowX(m_hWnd);
		}
	}
}

void htmlview_wnd::update_scroll() {
	if (!is_valid_page()) {
		ShowScrollBarX(m_hWnd, SB_BOTH, FALSE);
		return;
	}

	if (m_max_top > 0) {
		ShowScrollBarX(m_hWnd, SB_VERT, TRUE);

		RECTX rcClient;
		GetClientRectX(m_hWnd, &rcClient);

		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = m_max_top + (rcClient.bottom - rcClient.top);
		si.nPos = m_top;
		si.nPage = rcClient.bottom - rcClient.top;
		SetScrollInfoX(m_hWnd, SB_VERT, &si, TRUE);
	}
	else {
		ShowScrollBarX(m_hWnd, SB_VERT, FALSE);
	}

	if (m_max_left > 0) {
		ShowScrollBarX(m_hWnd, SB_HORZ, TRUE);

		RECTX rcClient;
		GetClientRectX(m_hWnd, &rcClient);

		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nMin = 0;
		si.nMax = m_max_left + (rcClient.right - rcClient.left);
		si.nPos = m_left;
		si.nPage = rcClient.right - rcClient.left;
		SetScrollInfoX(m_hWnd, SB_HORZ, &si, TRUE);
	}
	else {
		ShowScrollBarX(m_hWnd, SB_HORZ, FALSE);
	}
}

void htmlview_wnd::OnVScroll(int pos, int flags) {
	RECTX rcClient;
	GetClientRectX(m_hWnd, &rcClient);

	int lineHeight = 16;
	int pageHeight = rcClient.bottom - rcClient.top - lineHeight;

	int newTop = m_top;

	switch (flags) {
	case SB_LINEDOWN:
		newTop = m_top + lineHeight;
		if (newTop > m_max_top) {
			newTop = m_max_top;
		}
		break;
	case SB_PAGEDOWN:
		newTop = m_top + pageHeight;
		if (newTop > m_max_top) {
			newTop = m_max_top;
		}
		break;
	case SB_LINEUP:
		newTop = m_top - lineHeight;
		if (newTop < 0) {
			newTop = 0;
		}
		break;
	case SB_PAGEUP:
		newTop = m_top - pageHeight;
		if (newTop < 0) {
			newTop = 0;
		}
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		newTop = pos;
		if (newTop < 0) {
			newTop = 0;
		}
		if (newTop > m_max_top) {
			newTop = m_max_top;
		}
		break;
	}

	scroll_to(m_left, newTop);
}

void htmlview_wnd::OnHScroll(int pos, int flags) {
	RECTX rcClient;
	GetClientRectX(m_hWnd, &rcClient);

	int lineWidth = 16;
	int pageWidth = rcClient.right - rcClient.left - lineWidth;

	int newLeft = m_left;

	switch (flags) {
	case SB_LINERIGHT:
		newLeft = m_left + lineWidth;
		if (newLeft > m_max_left) {
			newLeft = m_max_left;
		}
		break;
	case SB_PAGERIGHT:
		newLeft = m_left + pageWidth;
		if (newLeft > m_max_left) {
			newLeft = m_max_left;
		}
		break;
	case SB_LINELEFT:
		newLeft = m_left - lineWidth;
		if (newLeft < 0) {
			newLeft = 0;
		}
		break;
	case SB_PAGELEFT:
		newLeft = m_left - pageWidth;
		if (newLeft < 0) {
			newLeft = 0;
		}
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		newLeft = pos;
		if (newLeft < 0) {
			newLeft = 0;
		}
		if (newLeft > m_max_left) {
			newLeft = m_max_left;
		}
		break;
	}

	scroll_to(newLeft, m_top);
}

void htmlview_wnd::OnMouseWheel(int delta) {
	int lineHeight = 16;

	int newTop = m_top - delta / WHEEL_DELTA * lineHeight * 3;

	if (newTop < 0) {
		newTop = 0;
	}
	if (newTop > m_max_top) {
		newTop = m_max_top;
	}

	if (newTop != m_top) {
		scroll_to(m_left, newTop);
	}
}

void htmlview_wnd::OnKeyDown(UINT vKey) {
	switch (vKey) {
	case VK_F5: refresh(); break;
	case VK_NEXT: OnVScroll(0, SB_PAGEDOWN); break;
	case VK_PRIOR: OnVScroll(0, SB_PAGEUP); break;
	case VK_DOWN: OnVScroll(0, SB_LINEDOWN); break;
	case VK_UP: OnVScroll(0, SB_LINEUP); break;
	case VK_HOME: scroll_to(m_left, 0); break;
	case VK_END: scroll_to(m_left, m_max_top); break;
	case VK_LEFT: OnHScroll(0, SB_LINELEFT); break;
	case VK_RIGHT: OnHScroll(0, SB_LINERIGHT); break;
	}
}

void htmlview_wnd::refresh() {
	web_page* page = get_page();

	if (page) {
		open(page->m_url.c_str(), true);

		page->release();
	}
}

void htmlview_wnd::set_caption() {
	web_page* page = get_page();

	if (!page) {
		SetWindowTextX(GetParentX(m_hWnd), L"litebrowser");
	}
	else {
		SetWindowTextX(GetParentX(m_hWnd), page->m_caption.c_str());
		page->release();
	}
}

void htmlview_wnd::OnMouseMove(int x, int y) {
	web_page* page = get_page();
	if (page) {
		litehtml::position::vector redraw_boxes;
		if (page->m_doc->on_mouse_over(x + m_left, y + m_top, x, y, redraw_boxes)) {
			for (litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++) {
				box->x -= m_left;
				box->y -= m_top;
				RECTX rcRedraw;
				rcRedraw.left = box->left();
				rcRedraw.right = box->right();
				rcRedraw.top = box->top();
				rcRedraw.bottom = box->bottom();
				redraw(&rcRedraw, FALSE);
			}
			UpdateWindowX(m_hWnd);
			update_cursor();
		}
		page->release();
	}
}

void htmlview_wnd::OnMouseLeave() {
	web_page* page = get_page();

	if (page) {
		litehtml::position::vector redraw_boxes;
		if (page->m_doc->on_mouse_leave(redraw_boxes)) {
			for (litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++) {
				box->x -= m_left;
				box->y -= m_top;
				RECTX rcRedraw;
				rcRedraw.left = box->left();
				rcRedraw.right = box->right();
				rcRedraw.top = box->top();
				rcRedraw.bottom = box->bottom();
				redraw(&rcRedraw, FALSE);
			}
			UpdateWindowX(m_hWnd);
		}

		page->release();
	}
}

void htmlview_wnd::OnLButtonDown(int x, int y) {
	web_page* page = get_page();

	if (page) {
		litehtml::position::vector redraw_boxes;
		if (page->m_doc->on_lbutton_down(x + m_left, y + m_top, x, y, redraw_boxes)) {
			for (litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++) {
				box->x -= m_left;
				box->y -= m_top;
				RECTX rcRedraw;
				rcRedraw.left = box->left();
				rcRedraw.right = box->right();
				rcRedraw.top = box->top();
				rcRedraw.bottom = box->bottom();
				redraw(&rcRedraw, FALSE);
			}
			UpdateWindowX(m_hWnd);
		}

		page->release();
	}
}

void htmlview_wnd::OnLButtonUp(int x, int y) {
	web_page* page = get_page();

	if (page) {
		litehtml::position::vector redraw_boxes;
		if (page->m_doc->on_lbutton_up(x + m_left, y + m_top, x, y, redraw_boxes)) {
			for (litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++) {
				box->x -= m_left;
				box->y -= m_top;
				RECTX rcRedraw;
				rcRedraw.left = box->left();
				rcRedraw.right = box->right();
				rcRedraw.top = box->top();
				rcRedraw.bottom = box->bottom();
				redraw(&rcRedraw, FALSE);
			}
			UpdateWindowX(m_hWnd);
		}

		page->release();
	}
}

void htmlview_wnd::back() {
	std::wstring url;
	if (m_history.back(url)) {
		open(url.c_str(), false);
	}
}

void htmlview_wnd::forward() {
	std::wstring url;
	if (m_history.forward(url)) {
		open(url.c_str(), false);
	}
}

void htmlview_wnd::update_cursor() {
	LPCWSTR defArrow = m_page_next ? IDC_APPSTARTING : IDC_ARROW;

	web_page* page = get_page();

	if (!page) {
		SetCursor(LoadCursor(NULL, defArrow));
	}
	else
	{
		if (page->m_cursor == L"pointer") {
			SetCursor(LoadCursor(NULL, IDC_HAND));
		}
		else {
			SetCursor(LoadCursor(NULL, defArrow));
		}
		page->release();
	}
}

void htmlview_wnd::get_client_rect(litehtml::position& client) const {
	RECTX rcClient;
	GetClientRectX(m_hWnd, &rcClient);
	client.x = rcClient.left;
	client.y = rcClient.top;
	client.width = rcClient.right - rcClient.left;
	client.height = rcClient.bottom - rcClient.top;
}

bool htmlview_wnd::is_valid_page(bool with_lock) {
	bool ret_val = true;

	if (with_lock) {
		lock();
	}

	if (!m_page || m_page && !m_page->m_doc) {
		ret_val = false;
	}

	if (with_lock) {
		unlock();
	}

	return ret_val;
}

web_page* htmlview_wnd::get_page(bool with_lock) {
	web_page* ret_val = NULL;
	if (with_lock) {
		lock();
	}
	if (is_valid_page(false)) {
		ret_val = m_page;
		ret_val->add_ref();
	}
	if (with_lock) {
		unlock();
	}

	return ret_val;
}

void htmlview_wnd::OnPageReady() {
	std::wstring url;
	lock();
	web_page* page = m_page_next;
	unlock();

	std::wstring hash;

	bool is_ok = false;

	lock();

	if (m_page_next) {
		if (m_page) {
			m_page->release();
		}
		m_page = m_page_next;
		m_page_next = NULL;
		is_ok = true;
		hash = m_page->m_hash;
		url = m_page->m_url;
	}

	unlock();

	if (is_ok) {
		render(FALSE, FALSE);
		m_top = 0;
		m_left = 0;
		show_hash(hash);
		update_scroll();
		redraw(NULL, FALSE);
		set_caption();
		update_history();
		m_parent->on_page_loaded(url.c_str());
	}
}

void htmlview_wnd::show_hash(std::wstring& hash) {
	web_page* page = get_page();
	if (page) {
		if (!hash.empty()) {
			char selector[255];
			LPSTR hashA = cairo_font::wchar_to_utf8(hash.c_str());
			StringCchPrintfA(selector, 255, "#%s", hashA);
			element::ptr el = page->m_doc->root()->select_one(selector);
			if (!el) {
				StringCchPrintfA(selector, 255, "[name=%s]", hashA);
				el = page->m_doc->root()->select_one(selector);
			}
			if (el) {
				litehtml::position pos = el->get_placement();
				m_top = pos.y;
			}
			delete hashA;
		}
		else {
			m_top = 0;
		}
		if (page->m_hash != hash) {
			page->m_hash = hash;
		}
		page->release();
	}
}

void htmlview_wnd::update_history() {
	web_page* page = get_page();

	if (page) {
		std::wstring url;
		page->get_url(url);

		m_history.url_opened(url);

		page->release();
	}
}

void htmlview_wnd::create_dib(int width, int height) {
	if (m_dib.width() < width || m_dib.height() < height) {
		m_dib.destroy();
		m_dib.create(width, height, true);
	}
}

void htmlview_wnd::scroll_to(int new_left, int new_top) {
	litehtml::position client;
	get_client_rect(client);

	bool need_redraw = false;
	if (new_top != m_top) {
		if (std::abs(new_top - m_top) < client.height - client.height / 4) {
			RECTX rcRedraw;
			if (new_top > m_top) {
				int lines_count = new_top - m_top;
				int rgba_to_scroll = m_dib.width() * lines_count;
				int rgba_total = m_dib.width() * client.height;

				memmove(m_dib.bits(), m_dib.bits() + rgba_to_scroll, (rgba_total - rgba_to_scroll) * sizeof(RGBQUAD));
				rcRedraw.left = client.left();
				rcRedraw.right = client.right();
				rcRedraw.top = client.height - lines_count;
				rcRedraw.bottom = client.height;
			}
			else {
				int lines_count = m_top - new_top;
				int rgba_to_scroll = m_dib.width() * lines_count;
				int rgba_total = m_dib.width() * client.height;

				memmove(m_dib.bits() + rgba_to_scroll, m_dib.bits(), (rgba_total - rgba_to_scroll) * sizeof(RGBQUAD));
				rcRedraw.left = client.left();
				rcRedraw.right = client.right();
				rcRedraw.top = client.top();
				rcRedraw.bottom = lines_count;
			}

			int old_top = m_top;
			m_top = new_top;
			OnPaint(&m_dib, &rcRedraw);

			litehtml::position::vector fixed_boxes;

			lock();
			web_page* page = get_page(false);
			if (page) {
				page->m_doc->get_fixed_boxes(fixed_boxes);
				page->release();
			}
			unlock();

			if (!fixed_boxes.empty()) {
				RECTX rcFixed;
				RECTX rcClient;
				rcClient.left = client.left();
				rcClient.right = client.right();
				rcClient.top = client.top();
				rcClient.bottom = client.bottom();

				for (litehtml::position::vector::iterator iter = fixed_boxes.begin(); iter != fixed_boxes.end(); iter++) {
					rcRedraw.left = iter->left();
					rcRedraw.right = iter->right();
					rcRedraw.top = iter->top();
					rcRedraw.bottom = iter->bottom();
					if (IntersectRectX(&rcFixed, &rcRedraw, &rcClient)) {
						OnPaint(&m_dib, &rcFixed);
					}

					rcRedraw.left = iter->left();
					rcRedraw.right = iter->right();
					rcRedraw.top = iter->top() + (old_top - m_top);
					rcRedraw.bottom = iter->bottom() + (old_top - m_top);
					if (IntersectRectX(&rcFixed, &rcRedraw, &rcClient)) {
						OnPaint(&m_dib, &rcFixed);
					}
				}
			}

			//HDC hdc = GetDC(m_hWnd);

			//BitBlt(hdc, client.left(), client.top(),
			//	client.width,
			//	client.height, m_dib, 0, 0, SRCCOPY);

			//ReleaseDC(m_hWnd, hdc);

		}
		else {
			need_redraw = true;
		}

		m_top = new_top;
		SetScrollPosX(m_hWnd, SB_VERT, m_top, TRUE);
	}


	if (new_left != m_left) {
		m_left = new_left;
		SetScrollPosX(m_hWnd, SB_HORZ, m_left, TRUE);
		need_redraw = true;
	}

	if (need_redraw) {
		redraw(NULL, TRUE);
	}
}