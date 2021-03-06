// This file is part of PinballY
// Copyright 2018 Michael J Roberts | GPL v3 or later | NO WARRANTY
//
// Based upon "AX" (an ActiveX container window) by Michael Courdakis, from
// https://www.codeproject.com/Articles/18417/Use-an-ActiveX-control-in-your-Win-Project-witho
//
// The Code Project article reference above is about hosting arbitrary
// ActiveX controls in bare-metal Win32 C++ programs (i.e., without any
// dependencies on Microsoft frameworks, such as MFC or ATL).  The code
// here used the code from the article as a starting point; the example
// in the article provides a good skeleton with the minimum set of OLE
// interfaces required to host an ActiveX control.  This version takes
// that basic skeleton and specializes it into a container specifically
// for the Shockwave Flash control, running in "windowless" mode.
// Flash requires a sort of super-sub-set of the basic set of 
// interfaces required for an ActiveX host; that is, it doesn't
// require all of the standard container interfaces to be implemented,
// but it adds some special requirements of its own.  We further
// specialize this by adding the "windowless ActiveX" capability.
//
// A windowless ActiveX container is a specialized subclass of the
// regular OLE in-place site that uses explicit COM object proxies to
// handle drawing and user interaction, instead of letting the control
// do its own drawing directly into an HWND.  Windowless mode's main
// utility is that it lets the container combine graphics from one or
// more ActiveX controls with graphics generated by other means.  In
// contrast, a normal ActiveX basically expects to have an HWND all to
// itself, which is fine for a dialog-like application but not great
// for a more graphically rich or 3D application such as this one.
// We use the windowless capability to integrate Flash into our D3D
// rendering model.  We have Flash render into a memory DC backed by
// a DIB, which we then we use as a D3D texture that we render on a 3D
// mesh representing our sprite.  In this way, we can use a Flash
// object as part of an arbitrary 3D scene by wrapping it onto any
// 3D mesh in the scene.
//

#include "../../Utilities/StringUtil.h"
#include "../../Utilities/Pointers.h"

class ErrorHandler;

class FlashClientSite :
    public IOleClientSite,
    public IDispatch,
    public IAdviseSink,
	public IOleInPlaceSiteWindowless,
	public IOleInPlaceFrame
{
public:
	// create
	static HRESULT Create(FlashClientSite **ppClientSite,
		const TCHAR *swfFile, int width, int height, ErrorHandler &eh);

	// Deactivate.  This must be explicitly called when the container is
	// closed or destroyed so that the embedded Flash player can detach
	// its resources.
	void Shutdown();

	// Is a redraw needed?
	bool NeedsRedraw() const { return hbmp == NULL || needRedraw; }

	// get the bitmap, redrawing the flash contents if necessary
	HBITMAP GetBitmap(RECT *rcBitmap, LPVOID *bits, BITMAPINFO *bmi);

	// Has the size changed since the last GetBitmap()?  
	bool IsSizeChanged() const { return hbmp == NULL; }

	// Update the bitmap.  This redraws the flash contents into the bitmap
	// if the drawing area has been invalidated. 
	//
	// Returns true on success, false if the drawing area has been resized.
	// A false return means that the object has to re-create the bitmap at
	// the new size, which you can do simply by calling GetBitmap().  That
	// will create the new bitmap and draw the frame into it at the current
	// size.
	bool UpdateBitmap();

	// set the layout size
	void SetLayoutSize(SIZE sz);

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid,void**ppvObject);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IOleClientSite methods
	STDMETHODIMP SaveObject() { return S_OK; }
    STDMETHODIMP GetMoniker(DWORD, DWORD, IMoniker **pm);
    STDMETHODIMP GetContainer(IOleContainer**pc);
	STDMETHODIMP ShowObject() { return S_OK; }
	STDMETHODIMP OnShowWindow(BOOL f) { return S_OK; }
	STDMETHODIMP RequestNewObjectLayout() { return S_OK; }

    // IAdviseSink methods
	STDMETHODIMP_(void) OnDataChange(FORMATETC*, STGMEDIUM*) { }
	STDMETHODIMP_(void) OnViewChange(DWORD, LONG) { }
	STDMETHODIMP_(void) OnRename(IMoniker*) { }
	STDMETHODIMP_(void) OnSave() { }
	STDMETHODIMP_(void) OnClose() { }

    // IOleInPlaceSite methods (inherited by IOleInPlaceSiteWindowless via IOleInPlaceSiteEx)
    STDMETHODIMP GetWindow(HWND *p);
	STDMETHODIMP ContextSensitiveHelp(BOOL) { return E_NOTIMPL; }
	STDMETHODIMP CanInPlaceActivate();
	STDMETHODIMP OnInPlaceActivate() { return S_OK; }
	STDMETHODIMP OnUIActivate() { return S_OK; }
    STDMETHODIMP GetWindowContext(IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc,
		LPRECT r1, LPRECT r2, LPOLEINPLACEFRAMEINFO o);
	STDMETHODIMP Scroll(SIZE) { return E_NOTIMPL; }
	STDMETHODIMP OnUIDeactivate(int) { return S_OK; }
	STDMETHODIMP OnInPlaceDeactivate() { return S_OK; }
	STDMETHODIMP DiscardUndoState() { return S_OK; }
	STDMETHODIMP DeactivateAndUndo() { return S_OK; }
	STDMETHODIMP OnPosRectChange(LPCRECT) { return S_OK; }

	// IOleInPlaceSiteEx methods (inherited by IOleInPlaceSiteWindowless)
	STDMETHODIMP OnInPlaceActivateEx(BOOL*, DWORD) { return S_OK; }
	STDMETHODIMP OnInPlaceDeactivateEx(BOOL) { return S_OK; }
	STDMETHODIMP RequestUIActivate() { return S_OK; }

	// IOleInPlaceSiteWindowless methods
	STDMETHODIMP AdjustRect(LPRECT prc) { return S_OK; }
	STDMETHODIMP CanWindowlessActivate() { return S_OK; }
	STDMETHODIMP GetCapture() { return S_FALSE; }
	STDMETHODIMP GetDC(LPCRECT, DWORD, HDC *) { return E_NOTIMPL; }
	STDMETHODIMP GetFocus() { return S_FALSE; }
	STDMETHODIMP InvalidateRect(LPCRECT pRect, BOOL fErase);
	STDMETHODIMP InvalidateRgn(HRGN hRGN, BOOL fErase);
	STDMETHODIMP OnDefWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
	STDMETHODIMP ReleaseDC(HDC) { return E_NOTIMPL; }
	STDMETHODIMP ScrollRect(INT, INT, LPCRECT, LPCRECT) { return E_NOTIMPL; }
	STDMETHODIMP SetCapture(BOOL fCapture) { return S_FALSE; }
	STDMETHODIMP SetFocus(BOOL fFocus) { return S_FALSE; }

    // IOleInPlaceFrame methods
    STDMETHODIMP GetBorder(LPRECT l);
	STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS) { return E_NOTIMPL; }
	STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS) { return S_OK; }
    STDMETHODIMP SetActiveObject(IOleInPlaceActiveObject *pV, LPCOLESTR s);
	STDMETHODIMP InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS) { return E_NOTIMPL; }
    STDMETHODIMP SetMenu(HMENU, HOLEMENU, HWND hw) { return E_NOTIMPL; }
	STDMETHODIMP RemoveMenus(HMENU h) { return S_OK; }
	STDMETHODIMP SetStatusText(LPCOLESTR) { return E_NOTIMPL; }
	STDMETHODIMP EnableModeless(BOOL) { return E_NOTIMPL; }
    STDMETHODIMP TranslateAccelerator(LPMSG,WORD) { return E_NOTIMPL; }


    // IDispatch Methods
	HRESULT _stdcall GetTypeInfoCount(unsigned int *) { return E_NOTIMPL; }
	HRESULT _stdcall GetTypeInfo(unsigned int, LCID, ITypeInfo FAR* FAR*) { return E_NOTIMPL; }
	HRESULT _stdcall GetIDsOfNames(REFIID, OLECHAR FAR* FAR*, unsigned int, LCID, DISPID FAR*) { return E_NOTIMPL; }
	HRESULT _stdcall Invoke(DISPID, REFIID, LCID, WORD, DISPPARAMS FAR*, VARIANT FAR*, EXCEPINFO FAR*, unsigned int FAR*) { return E_NOTIMPL; }

protected:
	// create via the static Create() method
	FlashClientSite(const TCHAR *swfFile);

	// destroy via Release()
	virtual ~FlashClientSite();

	// set the OLE object
	void SetOleObject(IOleObject *o) { pOleObj = o; }

	// get/set the activation status
	bool IsInPlaceActive() const { return isActivated; }
	void SetInPlaceActive(bool b) { isActivated = b; }

	// redraw the image, using the given memory DC
	void Redraw(HDC memdc);

	// COM reference count
	ULONG refCnt;

	// SWF file 
	TSTRING swfFile;

	// do we need a redraw?
	bool needRedraw;

	// is the object activated?
	bool isActivated;

	// layout rectangle
	RECT rcLayout;

	// the OLE object
	RefPtr<IOleObject> pOleObj;

	// in-place active object reference
	RefPtr<IOleInPlaceActiveObject> pInPlaceObj;

	// Advise token
	DWORD adviseToken;

	// off-screen drawing bitmap (as a DIB)
	BITMAPINFO bmi;
	HBITMAP hbmp;
	LPVOID bmpBits;
};

