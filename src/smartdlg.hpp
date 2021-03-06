/**
  * Win32 smart dialog wrapper
  *
  * Pixel-less, dynamic layout abstraction over Ye Olde Win32 window API.
  */

#pragma once

#include <vector>

namespace SmartDlg {
	// It makes little sense for things like area or padding to accept
	// negative values.
	struct unsigned_point_t {
		unsigned int x = 0;
		unsigned int y = 0;
	};

	struct unsigned_rect_t {
		unsigned int left = 0;
		unsigned int top = 0;
		unsigned int right = 0;
		unsigned int bottom = 0;
	};

	enum align_t {
		LEFT,
		CENTER,
		RIGHT
	};

	// Set width or height to this value to have
	// the parent widget determine them instead.
#define MAX_AREA (unsigned int)(-1)

	// Good luck trying to do the same with templates.
#define CACHED(type, name, get_method, update_method) \
	private: \
		type name; \
	public: \
		bool name##_stale = true; \
		\
		const type& get_method() { \
			if(name##_stale) { \
				update_method(name); \
				name##_stale = false; \
			} \
			return name; \
		}

	/// Fonts
	/// -----
	// Base class
	class Font {
	protected:
		void create(const LOGFONTW *lf);
	public:
		HDC hDC = GetDC(nullptr);
		LONG height = 0;
		LONG pad = 0;
		HFONT hFont = nullptr;

		void getPadding(unsigned_rect_t &padding);

		~Font();
	};

	// Default Windows dialog font
	class FontDefault : public Font {
	public:
		FontDefault::FontDefault();
	};
	/// -----

	/// Base classes
	/// ------------
	// Base class for all objects in a window's hierarchy
	class Base {
	protected:
		virtual void updateArea(unsigned_point_t &area) = 0;
		virtual void updatePos(POINT &pos_abs) {
			assert(parent);
			parent->updatePosForChild(pos_abs, this);
		}
		virtual void updatePosForChild(POINT &pos_abs, Base *w) {
			pos_abs.x = 0;
			pos_abs.y = 0;
		}
		virtual void updatePadding(unsigned_rect_t &padding) {
			getFont().getPadding(padding);
		}

	public:
		Base *parent = nullptr;

		CACHED(unsigned_point_t, area, getArea, updateArea);
		CACHED(unsigned_rect_t, padding, getPadding, updatePadding);
		CACHED(POINT, pos, getPos, updatePos);

		// Resolves MAX_AREA to the width or height of the
		// parent widget.
		virtual unsigned_point_t getRealArea();

		virtual unsigned_point_t pad(unsigned_point_t area);
		virtual POINT getPosPadded();

		virtual Font& getFont() {
			assert(parent != nullptr);
			return parent->getFont();
		}

		virtual void applyFontRecursive() = 0;
		virtual void applyDimensionsRecursive() = 0;
		virtual void createRecursive(HWND hWndParent) = 0;

		virtual void addChild(Base *child) = 0;

		virtual void applyAreaChangeUpwards(Base *child) = 0;
	};

	// Base class for all displayed widgets
	class BaseWidget : public Base {
	private:
		virtual LPCSTR CLASS_NAME() { return NULL; }

	protected:
		const char *text = nullptr;

	public:
		Base *child = nullptr;

		HWND hWnd = nullptr;
		DWORD style = 0;
		DWORD style_ex = 0;

		virtual unsigned_point_t decorate(unsigned_point_t area) {
			return area;
		}

		virtual void applyFontRecursive();
		virtual void applyDimensionsRecursive();
		virtual void createRecursive(HWND hWndParent);

		virtual void addChild(Base *w);

		virtual void applyAreaChangeUpwards(Base *child);

		virtual void setText(const char *text_new);

		BaseWidget(Base *parent) {
			if(parent) {
				parent->addChild(this);
			}
		}
	};

	// Base class for layout groups
	class BaseGroup : public Base {
	protected:
		std::vector<Base *> children;

	public:
		virtual void applyFontRecursive();
		virtual void applyDimensionsRecursive();
		virtual void createRecursive(HWND hWndParent);

		virtual void addChild(Base *w);

		virtual void applyAreaChangeUpwards(Base *child);

		BaseGroup(Base *parent) {
			assert(parent);
			parent->addChild(this);
		}
	};
	/// ------------

	/// Layout groups
	/// -------------
	class VerticalGroup : public BaseGroup {
	private:
		virtual void updateArea(unsigned_point_t &area);
		virtual void updatePosForChild(POINT &pos_abs, Base *w);

	public:
		align_t halign = LEFT;

		VerticalGroup(Base *parent, align_t halign = LEFT)
			: BaseGroup(parent), halign(halign) {
		}
	};
	/// -------------

	/// Label
	/// -----
	class Label : public BaseWidget {
	private:
		virtual void updateArea(unsigned_point_t &area);

		virtual LPCSTR CLASS_NAME() { return "Static"; }

	public:
		void setText(const char *text_new);

		Label(Base *parent, const char *str) : BaseWidget(parent) {
			text = str;
		}
	};
	/// -----

	/// Progress bar
	/// ------------
	class ProgressBar : public BaseWidget {
	private:
		virtual void updateArea(unsigned_point_t &area);

		virtual LPCSTR CLASS_NAME() { return PROGRESS_CLASSA; }

	public:
		ProgressBar(Base *parent) : BaseWidget(parent) {
			style |= PBS_SMOOTH;
		}
	};
	/// ------------

	/// Top-level dialog window
	/// -----------------------
	class Top : public BaseWidget {
	private:
		virtual void updateArea(unsigned_point_t &area);
		virtual void updatePos(POINT &pos_abs);
		virtual void updatePadding(unsigned_rect_t &padding);

		virtual LPCSTR CLASS_NAME() { return MAKEINTRESOURCEA(WC_DIALOG); }

	protected:
		FontDefault font;

	public:
		HANDLE event_created = CreateEvent(nullptr, true, false, nullptr);

		virtual Font& getFont() { return font; }

		virtual unsigned_point_t decorate(unsigned_point_t area);

		// The Win32 API demands that the message loop must be run in the
		// same thread that called CreateWindow(), so we might as well
		// combine both into a single function.
		virtual WPARAM create_and_run(const char *title = nullptr);

		virtual void close();

		Top() : BaseWidget(nullptr) {
			style |= WS_OVERLAPPED;
		}

		~Top() {
			CloseHandle(event_created);
		}
	};
}
