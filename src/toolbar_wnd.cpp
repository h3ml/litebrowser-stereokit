#include "globals.h"
#include "defaults.h"
#include "browser_wnd.h"
#include "toolbar_wnd.h"

toolbar_wnd::toolbar_wnd(int hInst, browser_wnd* parent) {
	m_inCapture = FALSE;
	m_omnibox = nullptr;
	m_parent = parent;
	m_hInst = hInst;
	m_hWnd = NULL;
}

toolbar_wnd::~toolbar_wnd(void) {
}

void toolbar_wnd::update() {
	bounds_t bounds;
	EvaluteWndX(m_hWnd, &bounds);
	material_set_texture(m_material, "diffuse", m_tex);
	render_add_mesh(defaults::meshQuad, m_material, matrix_trs(bounds.center + vec3{ 0.0f, 0.0f, -0.015f }, quat_identity, bounds.dimensions));
}

LRESULT CALLBACK toolbar_wnd::WndProc(XWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam) {
	toolbar_wnd* pThis = NULL;
	if (IsWindowX(hWnd)) {
		pThis = (toolbar_wnd*)GetPropX(hWnd, TEXT("toolbar_this"));
		if (pThis && pThis->m_hWnd != hWnd) {
			pThis = NULL;
		}
	}

	if (pThis || uMessage == WM_CREATE) {
		switch (uMessage) {
		case WM_EDIT_CAPTURE:
			if (wParam) {
				SetCaptureX(hWnd);
				pThis->m_inCapture = TRUE;
			}
			else {
				ReleaseCaptureX();
				pThis->m_inCapture = FALSE;
			}
			break;
		case WM_EDIT_ACTIONKEY:
			switch (wParam) {
			case VK_RETURN: {
				std::wstring url = pThis->m_omnibox->get_url();
				pThis->m_omnibox->select_all();
				pThis->m_parent->open(url.c_str());
				break;
			}
			}
			return 0;
		case WM_OMNIBOX_CLICKED:
			pThis->OnOmniboxClicked();
			break;
		case WM_UPDATE_CONTROL: {
			LPRECTX rcDraw = (LPRECTX)lParam;
			InvalidateRectX(hWnd, rcDraw, FALSE);
			break;
		}
		case WM_SETCURSOR:
			pThis->update_cursor();
			break;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_CREATE: {
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
			pThis = (toolbar_wnd*)(lpcs->lpCreateParams);
			SetPropX(hWnd, TEXT("toolbar_this"), (HANDLE)pThis);
			pThis->m_hWnd = hWnd;
			pThis->OnCreate();
			break;
		}
		case WM_PAINT: {
			PAINTSTRUCTX ps;
			HDC hdc = BeginPaintX(hWnd, &ps);
			
			simpledib::dib dib;
			dib.beginPaint(hdc, (LPRECT)&ps.rcPaint);
			pThis->OnPaint(&dib, &ps.rcPaint);
			tex_set_colors(pThis->m_tex = pThis->m_texs[pThis->m_texi], dib.width(), dib.height(), dib.bits());
			pThis->m_texi = (pThis->m_texi + 1) % 2;
			dib.endPaint();

			EndPaintX(hWnd, &ps);
			return 0;
		}
		case WM_KILLFOCUS:
			if (pThis->m_omnibox && pThis->m_omnibox->have_focus()) {
				pThis->m_omnibox->KillFocus();
			}
			break;
		case WM_SIZE:
			pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
			return 0;
		case WM_DESTROY:
			RemovePropX(hWnd, TEXT("toolbar_this"));
			pThis->OnDestroy();
			delete pThis;
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
		case WM_KEYDOWN:
			if (pThis->m_omnibox && pThis->m_omnibox->have_focus()) {
				if (pThis->m_omnibox->OnKeyDown(wParam, lParam)) {
					return 0;
				}
			}
			break;
		case WM_KEYUP:
			if (pThis->m_omnibox && pThis->m_omnibox->have_focus()) {
				if (pThis->m_omnibox->OnKeyUp(wParam, lParam)) {
					return 0;
				}
			}
			break;
		case WM_CHAR:
			if (pThis->m_omnibox && pThis->m_omnibox->have_focus()) {
				if (pThis->m_omnibox->OnChar(wParam, lParam)) {
					return 0;
				}
			}
			break;
		}
	}

	return DefWindowProcX(hWnd, uMessage, wParam, lParam);
}

void toolbar_wnd::render_toolbar(int width) {
	if (m_doc) {
		m_doc->render(width);
		m_omnibox->update_position();
	}
}

void toolbar_wnd::update_cursor() {
	LPCWSTR defArrow = IDC_ARROW;

	if (m_cursor == "pointer") {
		SetCursor(LoadCursor(NULL, IDC_HAND));
	}
	else if (m_cursor == "text") {
		SetCursor(LoadCursor(NULL, IDC_IBEAM));
	}
	else {
		SetCursor(LoadCursor(NULL, defArrow));
	}
}

void toolbar_wnd::OnCreate() {
	m_material = material_copy(defaults::materialUnlit);
	m_texi = 0;
	m_texs[0] = tex_create(tex_type_image_nomips, tex_format_bgra32);
	tex_set_address(m_texs[0], tex_address_clamp);
	m_texs[1] = tex_create(tex_type_image_nomips, tex_format_bgra32);
	tex_set_address(m_texs[1], tex_address_clamp);
}

void toolbar_wnd::OnPaint(simpledib::dib* dib, LPRECTX rcDraw) {
	if (m_doc) {
		cairo_surface_t* surface = cairo_image_surface_create_for_data((unsigned char*)dib->bits(), CAIRO_FORMAT_ARGB32, dib->width(), dib->height(), dib->width() * 4);
		cairo_t* cr = cairo_create(surface);

		POINT pt;
		GetWindowOrgEx(dib->hdc(), &pt);
		if (pt.x != 0 || pt.y != 0) {
			cairo_translate(cr, -pt.x, -pt.y);
		}
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_paint(cr);

		litehtml::position clip(rcDraw->left, rcDraw->top, rcDraw->right - rcDraw->left, rcDraw->bottom - rcDraw->top);
		m_doc->draw((litehtml::uint_ptr)cr, 0, 0, &clip);

		cairo_destroy(cr);
		cairo_surface_destroy(surface);
	}
}

void toolbar_wnd::OnSize(int width, int height) {
}

void toolbar_wnd::OnDestroy() {
}

extern std::map<std::wstring, std::tuple<const unsigned char*, const unsigned long>> ResourceMap;
void toolbar_wnd::create(int x, int y, int z, int width, XWND parent) {
	LPSTR html = NULL;

	const auto hResource = ResourceMap[L"toolbar.html"];
	if (std::get<0>(hResource)) {
		DWORD imageSize = std::get<1>(hResource);
		if (imageSize) {
			LPCSTR pResourceData = (LPCSTR)std::get<0>(hResource);
			if (pResourceData) {
				html = new CHAR[imageSize + 1];
				lstrcpynA(html, pResourceData, imageSize);
				html[imageSize] = 0;
			}
		}
	}
	m_hWnd = CreateWindowX(L"toolbar", x, y, z, width, 1, 1, parent, NULL, m_hInst, (LPVOID)this, (WNDPROC)toolbar_wnd::WndProc);

	m_doc = litehtml::document::createFromString(html, this, "html,div,body { display: block; } head,style { display: none; }");
	delete html;
	render_toolbar(width);
	MoveWindowX(m_hWnd, x, y, z, width, m_doc->height(), 1, TRUE);
}

void toolbar_wnd::make_url(LPCWSTR url, LPCWSTR basepath, std::wstring& out) {
	out = url;
}

cairo_container::image_ptr toolbar_wnd::get_image(LPCWSTR url, bool redraw_on_ready) {
	cairo_container::image_ptr img = cairo_container::image_ptr(new CTxDIB);
	const auto hResource = ResourceMap[url];
	if (!std::get<0>(hResource) || !img->load((LPBYTE)std::get<0>(hResource), std::get<1>(hResource))) {
		img = nullptr;
	}

	return img;
}

void toolbar_wnd::set_caption(const char* caption) {
}

void toolbar_wnd::set_base_url(const char* base_url) {
}

void toolbar_wnd::link(std::shared_ptr<litehtml::document>& doc, litehtml::element::ptr el) {
}

int toolbar_wnd::set_width(int width) {
	if (m_doc) {
		render_toolbar(width);

		return m_doc->height();
	}
	return 0;
}

void toolbar_wnd::on_page_loaded(LPCWSTR url) {
	if (m_omnibox) {
		m_omnibox->set_url(url);
	}
}

void toolbar_wnd::OnMouseMove(int x, int y) {
	if (m_doc) {
		BOOL process = TRUE;
		if (m_omnibox) {
			m_omnibox->OnMouseMove(x, y);
		}
		if (!m_inCapture) {
			litehtml::position::vector redraw_boxes;
			if (m_doc->on_mouse_over(x, y, x, y, redraw_boxes)) {
				for (litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++) {
					RECTX rcRedraw;
					rcRedraw.left = box->left();
					rcRedraw.right = box->right();
					rcRedraw.top = box->top();
					rcRedraw.bottom = box->bottom();
					InvalidateRectX(m_hWnd, &rcRedraw, TRUE);
				}
				UpdateWindowX(m_hWnd);
			}
		}
	}
	update_cursor();
}

void toolbar_wnd::OnMouseLeave() {
	if (m_doc) {
		litehtml::position::vector redraw_boxes;
		if (m_doc->on_mouse_leave(redraw_boxes)) {
			for (litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++) {
				RECTX rcRedraw;
				rcRedraw.left = box->left();
				rcRedraw.right = box->right();
				rcRedraw.top = box->top();
				rcRedraw.bottom = box->bottom();
				InvalidateRectX(m_hWnd, &rcRedraw, TRUE);
			}
			UpdateWindowX(m_hWnd);
		}
	}
}

void toolbar_wnd::OnOmniboxClicked() {
	SetFocusX(m_hWnd);
	m_omnibox->SetFocus();
}

void toolbar_wnd::OnLButtonDown(int x, int y) {
	if (m_doc) {
		BOOL process = TRUE;
		if (m_omnibox && m_omnibox->OnLButtonDown(x, y)) {
			process = FALSE;
		}
		if (process && !m_inCapture) {
			litehtml::position::vector redraw_boxes;
			if (m_doc->on_lbutton_down(x, y, x, y, redraw_boxes)) {
				for (litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++) {
					RECTX rcRedraw;
					rcRedraw.left = box->left();
					rcRedraw.right = box->right();
					rcRedraw.top = box->top();
					rcRedraw.bottom = box->bottom();
					InvalidateRectX(m_hWnd, &rcRedraw, TRUE);
				}
				UpdateWindowX(m_hWnd);
			}
		}
	}
}

void toolbar_wnd::OnLButtonUp(int x, int y) {
	if (m_doc) {
		BOOL process = TRUE;
		if (m_omnibox && m_omnibox->OnLButtonUp(x, y)) {
			process = FALSE;
		}
		if (process && !m_inCapture) {
			litehtml::position::vector redraw_boxes;
			if (m_doc->on_lbutton_up(x, y, x, y, redraw_boxes)) {
				for (litehtml::position::vector::iterator box = redraw_boxes.begin(); box != redraw_boxes.end(); box++) {
					RECTX rcRedraw;
					rcRedraw.left = box->left();
					rcRedraw.right = box->right();
					rcRedraw.top = box->top();
					rcRedraw.bottom = box->bottom();
					InvalidateRectX(m_hWnd, &rcRedraw, TRUE);
				}
				UpdateWindowX(m_hWnd);
			}
		}
	}
}

struct {
	LPCWSTR	name;
	LPCWSTR	url;
} g_bookmarks[] = {
	{L"DMOZ",					L"http://dmoz-odp.org/"},
	{L"litehtml project",		L"https://github.com/litehtml/litehtml"},
	{L"litehtml website",		L"http://www.litehtml.com/"},
	{L"True Launch Bar",		L"http://www.truelaunchbar.com/"},
	{L"Tordex",					L"http://www.tordex.com/"},
	{L"True Paste",				L"http://www.truepaste.com/"},
	{L"Text Accelerator",		L"http://www.textaccelerator.com/"},
	{L"Wiki: Web Browser",		L"http://en.wikipedia.org/wiki/Web_browser"},
	{L"Wiki: Obama",			L"http://en.wikipedia.org/wiki/Obama"},
	{L"Code Project",			L"http://www.codeproject.com/"},

	{NULL,						NULL},
};

void toolbar_wnd::on_anchor_click(const char* url, const litehtml::element::ptr& el) {
	if (!strcmp(url, "back")) {
		m_parent->back();
	}
	else if (!strcmp(url, "forward")) {
		m_parent->forward();
	}
	else if (!strcmp(url, "reload")) {
		m_parent->reload();
	}
	else if (!strcmp(url, "bookmarks")) {
		//litehtml::position pos = el->get_placement();
		//POINT pt;
		//pt.x = pos.right();
		//pt.y = pos.bottom();
		//MapWindowPoints(m_hWnd, NULL, &pt, 1);

		//HMENU hMenu = CreatePopupMenu();

		//for (int i = 0; g_bookmarks[i].url; i++) {
		//	InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, i + 1, g_bookmarks[i].name);
		//}

		//int ret = TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		//DestroyMenu(hMenu);

		//if (ret) {
		//	m_parent->open(g_bookmarks[ret - 1].url);
		//}
	}
	else if (!strcmp(url, "settings")) {
		//litehtml::position pos = el->get_placement();
		//POINT pt;
		//pt.x = pos.right();
		//pt.y = pos.bottom();
		//MapWindowPoints(m_hWnd, NULL, &pt, 1);

		//HMENU hMenu = CreatePopupMenu();

		//InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 1, L"Calculate Render Time");
		//InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 3, L"Calculate Render Time (10)");
		//InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 4, L"Calculate Render Time (100)");
		//InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, L"");
		//InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 5, L"Calculate Draw Time");
		//InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 6, L"Calculate Draw Time (10)");
		//InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 7, L"Calculate Draw Time (100)");
		//InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, L"");
		//InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, 2, L"Exit");

		//int ret = TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
		//DestroyMenu(hMenu);

		//switch (ret) {
		//case 2: PostQuitMessage(0); break;
		//case 1: m_parent->calc_time(); break;
		//case 3: m_parent->calc_time(10); break;
		//case 4: m_parent->calc_time(100); break;
		//case 5: m_parent->calc_redraw(1); break;
		//case 6: m_parent->calc_redraw(10); break;
		//case 7: m_parent->calc_redraw(100); break;
		//}
	}
}

void toolbar_wnd::set_cursor(const char* cursor) {
	m_cursor = cursor;
}

std::shared_ptr<litehtml::element> toolbar_wnd::create_element(const char* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc) {
	if (!litehtml::t_strcasecmp(tag_name, "input")) {
		auto iter = attributes.find("type");
		if (iter != attributes.end()) {
			if (!litehtml::t_strcasecmp(iter->second.c_str(), "text")) {
				if (m_omnibox) {
					m_omnibox = nullptr;
				}

				m_omnibox = std::make_shared<el_omnibox>(doc, m_hWnd, this);
				return m_omnibox;
			}
		}
	}
	return 0;
}

void toolbar_wnd::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) {
}

void toolbar_wnd::get_client_rect(litehtml::position& client) const {
	RECTX rcClient;
	GetClientRectX(m_hWnd, &rcClient);
	client.x = rcClient.left;
	client.y = rcClient.top;
	//client.z = rcClient.front;
	client.width = rcClient.right - rcClient.left;
	client.height = rcClient.bottom - rcClient.top;
	//client.back = rcClient.back - rcClient.front;
}
