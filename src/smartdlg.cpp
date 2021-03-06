/**
  * Win32 smart dialog wrapper
  *
  * Pixel-less, dynamic layout abstraction over Ye Olde Win32 window API.
  */

#include <win32_utf8.h>
#include "smartdlg.hpp"

namespace SmartDlg {
	HMODULE hMod = GetModuleHandle(nullptr);

	LRESULT CALLBACK DlgProc(
		HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam
	)
	{
		switch(uMsg) {
		case WM_CLOSE: // Yes, these are not handled by DefDlgProc().
			DestroyWindow(hWnd);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		}
		return DefDlgProcW(hWnd, uMsg, wParam, lParam);
	}

	/// Fonts
	/// -----
	void Font::create(const LOGFONTW *lf)
	{
		assert(lf);

		hFont = CreateFontIndirectW(lf);
		if(hFont) {
			SelectObject(hDC, hFont);
		}
		height = (lf->lfHeight < 0 ? -lf->lfHeight : lf->lfHeight);
		pad = height / 2;
	}

	void Font::getPadding(unsigned_rect_t &padding)
	{
		padding.top = pad;
		padding.left = pad;
		padding.right = pad;
		padding.bottom = pad;
	}

	Font::~Font()
	{
		if(hFont) {
			DeleteObject(hFont);
		}
	}

	FontDefault::FontDefault()
	{
		NONCLIENTMETRICSW nc_metrics = {sizeof(nc_metrics)};
		if(SystemParametersInfoW(
			SPI_GETNONCLIENTMETRICS, sizeof(nc_metrics), &nc_metrics, 0
		)) {
			create(&nc_metrics.lfMessageFont);
		}
	}
	/// -----

	/// Base classes
	/// ------------
	unsigned_point_t Base::getRealArea()
	{
		auto ret = getArea();
		bool x_is_max = (area.x == MAX_AREA);
		bool y_is_max = (area.y == MAX_AREA);
		if(!parent) {
			assert(!x_is_max || !"Make sure you have some explicitly sized parent widget!");
			assert(!y_is_max || !"Make sure you have some explicitly sized parent widget!");
			return ret;
		}
		if(x_is_max || y_is_max) {
			const auto& parent_area = parent->getRealArea();
			const auto &pad = getPadding();
			if(x_is_max) {
				ret.x = parent_area.x - pad.left - pad.right;
			}
			if(y_is_max) {
				ret.y = parent_area.y - pad.top - pad.bottom;
			}
		}
		return ret;
	}

	unsigned_point_t Base::pad(unsigned_point_t area)
	{
		const auto& pad = getPadding();
		area.x += (area.x == MAX_AREA) ? 0 : pad.left + pad.right;
		area.y += (area.y == MAX_AREA) ? 0 : pad.top + pad.bottom;
		return area;
	}

	POINT Base::getPosPadded()
	{
		POINT ret = getPos();
		const auto& pad = getPadding();
		ret.x += pad.left;
		ret.y += pad.right;
		return ret;
	}

	void Base::applyAreaChangeUpwards(Base *child)
	{
		if(parent) {
			parent->applyAreaChangeUpwards(child);
		} else {
			applyDimensionsRecursive();
		}
	}

	void BaseWidget::applyFontRecursive()
	{
		const auto &font = getFont();
		SendMessageW(hWnd, WM_SETFONT, (WPARAM)font.hFont, 0);
		if(child) {
			child->applyFontRecursive();
		}
	}

	void BaseWidget::applyDimensionsRecursive()
	{
		const auto &pos = getPosPadded();
		const auto &area = decorate(getRealArea());
		SetWindowPos(hWnd, nullptr, pos.x, pos.y, area.x, area.y, 0);
		UpdateWindow(hWnd);
		if(child) {
			child->applyDimensionsRecursive();
		}
	}

	void BaseWidget::createRecursive(HWND hWndParent)
	{
		const auto &pos = getPosPadded();
		const auto &area = decorate(getRealArea());

		if(hWndParent) {
			style |= WS_CHILD | WS_VISIBLE;
			style_ex |= WS_EX_NOPARENTNOTIFY;
		}
		hWnd = CreateWindowExU(
			style_ex, CLASS_NAME(), text, style,
			pos.x, pos.y, area.x, area.y,
			hWndParent, nullptr, hMod, nullptr
		);
		if(child) {
			child->createRecursive(hWnd);
		}
	}

	void BaseWidget::addChild(Base *w)
	{
		assert(child == nullptr);
		child = w;
		w->parent = this;
	}

	void BaseWidget::applyAreaChangeUpwards(Base *child)
	{
		assert(child);
		const auto &area_child = child->pad(child->getRealArea());
		const auto &area_this = getArea();
		if(area_child.x > area_this.x || area_child.y > area_this.y) {
			area_stale = true;
			Base::applyAreaChangeUpwards(this);
		} else {
			child->applyDimensionsRecursive();
		}
	}

	void BaseWidget::setText(const char *text_new)
	{
		text = text_new;
		// No nullptr check for hWnd necessary.
		SetWindowTextU(hWnd, text_new);
	}

	void BaseGroup::applyFontRecursive()
	{
		for(auto &it : children) {
			it->applyFontRecursive();
		}
	}

	void BaseGroup::applyDimensionsRecursive()
	{
		for(auto &it : children) {
			it->applyDimensionsRecursive();
		}
	}

	void BaseGroup::createRecursive(HWND hWndParent)
	{
		for(auto &it : children) {
			it->createRecursive(hWndParent);
		}
	}

	void BaseGroup::addChild(Base *w)
	{
		children.push_back(w);
		w->parent = this;
	}

	void BaseGroup::applyAreaChangeUpwards(Base *child)
	{
		assert(child);
		const auto &area_child = child->pad(child->getArea());
		const auto &area_group = getRealArea();

		bool larger_x = area_child.x > area_group.x;
		bool larger_y = area_child.y > area_group.y;

		if(larger_x || larger_y) {
			area_stale = true;
			for(auto &it : children) {
				if(it != child) {
					const auto &area_it = it->getArea();
					it->area_stale  = (larger_x && area_it.x == MAX_AREA);
					it->area_stale |= (larger_y && area_it.y == MAX_AREA);
				}
			}
			Base::applyAreaChangeUpwards(this);
		} else {
			child->applyDimensionsRecursive();
		}
	}
	/// -----------------

	/// Layout groups
	/// -------------
	void VerticalGroup::updateArea(unsigned_point_t &area)
	{
		assert(area_stale); // Call getArea() instead!

		area.x = 0;
		area.y = 0;
		for(auto &it : children) {
			const auto &child_area = it->pad(it->getArea());
			if(child_area.x != MAX_AREA) {
				area.x = max(area.x, child_area.x);
			}
			area.y += child_area.y;
		}
	}

	void VerticalGroup::updatePosForChild(POINT &pos_abs, Base *w)
	{
		const auto &self_area = getRealArea();
		const auto &self_pos = getPosPadded();

		pos_abs.x = self_pos.x;
		pos_abs.y = self_pos.y;
		for(auto &it : children) {
			const auto &child_area = it->pad(it->getRealArea());
			if(it == w) {
				switch(halign) {
				case LEFT:
					break;
				case CENTER:
					pos_abs.x += (self_area.x / 2) - (child_area.x / 2);
					break;
				case RIGHT:
					pos_abs.x += self_area.x - child_area.x;
					break;
				}
				return;
			}
			pos_abs.y += child_area.y;
		}
		assert(!"not a child of this group");
	}
	/// -------------

	/// Label
	/// -----
	void Label::updateArea(unsigned_point_t &area)
	{
		assert(area_stale); // Call getArea() instead!

		RECT rect = {0};
		const auto& font = getFont();
		DrawText(font.hDC, text, -1, &rect, DT_CALCRECT);
		area.x = rect.right;
		area.y = rect.bottom;
	}

	void Label::setText(const char *text_new)
	{
		text = text_new;
		area_stale = true;
		Base::applyAreaChangeUpwards(this);
		// Doing this after the size change
		// seems to avoid rendering glitches.
		SetWindowTextU(hWnd, text_new);
	}
	/// -----

	/// Progress bar
	/// ------------
	void ProgressBar::updateArea(unsigned_point_t &area)
	{
		assert(area_stale); // Call getArea() instead!

		area.x = MAX_AREA;
		area.y = getFont().height * 2;
	}
	/// ------------

	/// Top-level dialog window
	/// -----------------------
	void Top::updateArea(unsigned_point_t &area)
	{
		assert(child);
		assert(area_stale); // Call getArea() instead!

		area = child->pad(child->getRealArea());
	}

	void Top::updatePos(POINT &pos_abs)
	{
		assert(pos_stale); // Call getPos() instead!

		const auto &area = getArea();
		RECT screen;
		if(!SystemParametersInfoW(SPI_GETWORKAREA, sizeof(RECT), &screen, 0)) {
			screen.right = GetSystemMetrics(SM_CXSCREEN);
			screen.bottom = GetSystemMetrics(SM_CYSCREEN);
		}
		pos_abs.x = (screen.right / 2) - (area.x / 2);
		pos_abs.y = (screen.bottom / 2) - (area.y / 2);
	}

	void Top::updatePadding(unsigned_rect_t &padding)
	{
		ZeroMemory(&padding, sizeof(padding));
	}

	unsigned_point_t Top::decorate(unsigned_point_t area)
	{
		RECT self = { 0, 0, area.x, area.y };
		AdjustWindowRectEx(&self, style, false, style_ex);
		area.x = self.right - self.left;
		area.y = self.bottom - self.top;
		return area;
	}

	WPARAM Top::create_and_run(const char *title)
	{
		// CreateWindowEx() sets this automatically for windows with
		// WS_OVERLAPPED, but AdjustWindowRectEx() doesn't, causing
		// our calculations to return a window size that's too small.
		// See https://github.com/wine-mirror/wine/blob/797a746fc2a1b17d67b7423293e081e3e7171033/dlls/user32/win.c#L1499
		if(style == WS_OVERLAPPED) {
			style |= WS_CAPTION;
		}

		if(title) {
			text = title;
		}
		createRecursive(nullptr);
		applyFontRecursive();
		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);

		SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LPARAM)DlgProc);

		SetEvent(event_created);

		MSG msg;
		BOOL msg_ret;

		while((msg_ret = GetMessage(&msg, nullptr, 0, 0)) != 0) {
			if(msg_ret != -1) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		return msg.wParam;
	}

	void Top::close()
	{
		if(hWnd) {
			SendMessageW(hWnd, WM_CLOSE, 0, 0);
		}
	}
	/// ----------------------
}
