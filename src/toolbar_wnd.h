#pragma once

#include "../containers/cairo/cairo_container.h"
#include "dib.h"
#include "litehtml/el_omnibox.h"
class browser_wnd;

class toolbar_wnd : public cairo_container {
	XWND					m_hWnd;
	XINSTANCE				m_hInst;
	litehtml::document::ptr	m_doc;
	browser_wnd*			m_parent;
	std::shared_ptr<el_omnibox>	m_omnibox;
	litehtml::string		m_cursor;
	BOOL					m_inCapture;
	material_t				m_material;
	tex_t					m_tex;
	int						m_texi;
	tex_t					m_texs[2];
public:
	toolbar_wnd(XINSTANCE hInst, browser_wnd* parent);
	virtual ~toolbar_wnd(void);
	void update();

	void create(int x, int y, int z, int width, XWND parent);
	XWND wnd() { return m_hWnd; }
	int height() {
		return m_doc ? m_doc->height() : 0;
	}
	int set_width(int width);
	void on_page_loaded(LPCWSTR url);

	// cairo_container members
	virtual void	make_url(LPCWSTR url, LPCWSTR basepath, std::wstring& out);
	virtual cairo_container::image_ptr get_image(LPCWSTR url, bool redraw_on_ready);

	// litehtml::document_container members
	virtual	void	set_caption(const char* caption);
	virtual	void	set_base_url(const char* base_url);
	virtual	void	link(std::shared_ptr<litehtml::document>& doc, litehtml::element::ptr el);
	virtual void	import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl);
	virtual	void	on_anchor_click(const char* url, const litehtml::element::ptr& el);
	virtual	void	set_cursor(const char* cursor);
	virtual std::shared_ptr<litehtml::element> create_element(const char* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc);

protected:
	virtual void	OnCreate();
	virtual void	OnPaint(simpledib::dib* dib, LPRECTX rcDraw);
	virtual void	OnSize(int width, int height);
	virtual void	OnDestroy();
	virtual void	OnMouseMove(int x, int y);
	virtual void	OnLButtonDown(int x, int y);
	virtual void	OnLButtonUp(int x, int y);
	virtual void	OnMouseLeave();
	virtual void	OnOmniboxClicked();

	virtual void	get_client_rect(litehtml::position& client) const;

private:
	static LRESULT CALLBACK WndProc(XWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	void render_toolbar(int width);
	void update_cursor();
};
