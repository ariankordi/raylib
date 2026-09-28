// Source: https://github.com/rndtrash/glfw/blob/master/src/win32_polyfill.c
//
// Local modifications (not upstream), 2026-09: the always-FALSE GetMonitorInfoW, EnumDisplaySettingsW
// and EnumDisplaySettingsExW stubs were replaced, and EnumDisplayDevicesW and GetMonitorInfoA added, in
// the "Display polyfills" section below. RGFW ignores those return values, so the stubs left its
// MONITORINFOEX/DEVMODE locals uninitialized.
// Note: this file must be included BEFORE win32_polyfill.h, since the polyfills call the real
// EnumDisplaySettingsW, which that header redefines.

#include <windows.h>

// Reference: https://github.com/metaxor/KernelEx/blob/31cdfc3560fc116637ee8ed7be31b12f3aacf5d1/common/common.h#L143

#define STACK_AtoW(strA,strW) \
    { \
        strW = (LPWSTR)strA; \
        if (HIWORD(strA)) \
        { \
            int c = lstrlenA((LPCSTR)strA); \
            if (c) \
            { \
                strW = (LPWSTR)alloca(c * sizeof(WCHAR)); \
                MultiByteToWideChar(CP_ACP, 0, (LPCSTR)strA, -1, (LPWSTR)strW, c); \
            } \
        } \
    }

// SetThreadExecutionState polyfill
// https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadexecutionstate
//

EXECUTION_STATE GLFW_SetThreadExecutionState(EXECUTION_STATE esFlags)
{
    // TODO: figure out how to prevent the screen from turning off on Windows 2000
    return (EXECUTION_STATE)NULL;
}

// GetModuleHandleExW polyfill
// https://learn.microsoft.com/ru-ru/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandleexw
// Reference: https://github.com/metaxor/KernelEx/blob/31cdfc3560fc116637ee8ed7be31b12f3aacf5d1/apilibs/kexbases/Kernel32/module.c#L108
//

BOOL GLFW_GetModuleHandleExW(DWORD dwFlags, LPCWSTR lpModuleName, HMODULE *phModule)
{
    WCHAR buf[MAX_PATH];
    if (!phModule)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *phModule = NULL;
    if (dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)
    {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery(lpModuleName, &mbi, sizeof(mbi)))
            return FALSE;

        *phModule = (HMODULE) mbi.AllocationBase;
    }
    else
        *phModule = GetModuleHandleW(lpModuleName);

    if (*phModule == NULL || !GetModuleFileNameW(*phModule, buf, MAX_PATH))
    {
        *phModule = NULL;
        return FALSE;
    }

    if (!(dwFlags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
        LoadLibraryW(buf);

    return TRUE;
}

// VerSetConditionMask polyfill
// https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-versetconditionmask
// Reference: https://github.com/metaxor/KernelEx/blob/31cdfc3560fc116637ee8ed7be31b12f3aacf5d1/apilibs/kexbases/Kernel32/version.c#L383
//

inline ULONGLONG GLFW_VerSetConditionMask(ULONGLONG dwlConditionMask, DWORD dwTypeBitMask, BYTE dwConditionMask)
{
    if (dwTypeBitMask == 0)
        return dwlConditionMask;

    dwConditionMask &= 0x07;
    if (dwConditionMask == 0)
        return dwlConditionMask;

    if (dwTypeBitMask & VER_PRODUCT_TYPE)
        dwlConditionMask |= dwConditionMask << 7*3;
    else if (dwTypeBitMask & VER_SUITENAME)
        dwlConditionMask |= dwConditionMask << 6*3;
    else if (dwTypeBitMask & VER_SERVICEPACKMAJOR)
        dwlConditionMask |= dwConditionMask << 5*3;
    else if (dwTypeBitMask & VER_SERVICEPACKMINOR)
        dwlConditionMask |= dwConditionMask << 4*3;
    else if (dwTypeBitMask & VER_PLATFORMID)
        dwlConditionMask |= dwConditionMask << 3*3;
    else if (dwTypeBitMask & VER_BUILDNUMBER)
        dwlConditionMask |= dwConditionMask << 2*3;
    else if (dwTypeBitMask & VER_MAJORVERSION)
        dwlConditionMask |= dwConditionMask << 1*3;
    else if (dwTypeBitMask & VER_MINORVERSION)
        dwlConditionMask |= dwConditionMask << 0*3;
    return dwlConditionMask;
}

// ---------------------------------------------------------------------------------------------------
// Display polyfills (local addition, not from rndtrash/glfw): emulate a single display.
//
// NT 4.0's user32 exports an undocumented 3-parameter EnumDisplayDevicesW (it ends in `ret 0xc`, and
// rejects any non-NULL lpDevice). Calling it through the SDK's 4-parameter prototype leaks 4 bytes of
// stack per call, which corrupted RGFW_pollMonitors()'s frame, so it must never be called.
// ---------------------------------------------------------------------------------------------------

/// Adapter name reported by EnumDisplayDevicesW() and GetMonitorInfo()'s szDevice. RGFW matches the
/// two with wcscmp() and passes szDevice to CreateDCW(), which accepts "DISPLAY" on every NT version.
#define POLYFILL_DISPLAY_NAME "DISPLAY"

// EnumDisplayDevicesW polyfill
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enumdisplaydevicesw
//
// Reports one adapter with one monitor under it. The monitor is required: with no children, RGFW
// falls back to RGFW_win32_createMonitor(&adapter, NULL), which dereferences the NULL.
//

BOOL GLFW_EnumDisplayDevicesW(LPCWSTR lpDevice, DWORD iDevNum, PDISPLAY_DEVICEW lpDisplayDevice, DWORD dwFlags)
{
    if (iDevNum != 0 || lpDisplayDevice == NULL || lpDisplayDevice->cb < sizeof(DISPLAY_DEVICEW))
        return FALSE;

    ZeroMemory(lpDisplayDevice, sizeof(DISPLAY_DEVICEW));
    lpDisplayDevice->cb = sizeof(DISPLAY_DEVICEW);

    // Bit 0 is ATTACHED_TO_DESKTOP for adapters and ACTIVE for monitors (both 0x1).
    if (lpDevice == NULL)
    {
        lstrcpyW(lpDisplayDevice->DeviceName, L"" POLYFILL_DISPLAY_NAME);
        lstrcpyW(lpDisplayDevice->DeviceString, L"Primary Display Adapter");
        lpDisplayDevice->StateFlags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_PRIMARY_DEVICE;
    }
    else
    {
        lstrcpyW(lpDisplayDevice->DeviceName, L"" POLYFILL_DISPLAY_NAME "\\Monitor0");
        lstrcpyW(lpDisplayDevice->DeviceString, L"Default Monitor");
        lpDisplayDevice->StateFlags = DISPLAY_DEVICE_ACTIVE;
    }

    return TRUE;
}

/// Fills the part of MONITORINFO shared by the A and W variants with the whole screen. Any hMonitor is
/// accepted, since the EnumDisplayMonitors/MonitorFrom* stubs hand out NULL for the one display.
static BOOL Polyfill_FillMonitorInfo(LPMONITORINFO lpmi)
{
    if (lpmi == NULL || lpmi->cbSize < sizeof(MONITORINFO))
        return FALSE;

    SetRect(&lpmi->rcMonitor, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    if (!SystemParametersInfoA(SPI_GETWORKAREA, 0, &lpmi->rcWork, 0))
        lpmi->rcWork = lpmi->rcMonitor;
    lpmi->dwFlags = MONITORINFOF_PRIMARY;
    return TRUE;
}

// EnumDisplayMonitors polyfill
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enumdisplaymonitors
//

BOOL GLFW_EnumDisplayMonitors(HDC hdc, LPCRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData)
{
    // TODO: call the callback at least once for one monitor
    return FALSE;
}

// GetMonitorInfoW/A polyfills
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmonitorinfow
//

BOOL GLFW_GetMonitorInfoW(HMONITOR hMonitor, LPMONITORINFO lpmi)
{
    if (!Polyfill_FillMonitorInfo(lpmi))
        return FALSE;

    if (lpmi->cbSize >= sizeof(MONITORINFOEXW))
        lstrcpyW(((MONITORINFOEXW *)lpmi)->szDevice, L"" POLYFILL_DISPLAY_NAME);
    return TRUE;
}

BOOL GLFW_GetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO lpmi)
{
    if (!Polyfill_FillMonitorInfo(lpmi))
        return FALSE;

    if (lpmi->cbSize >= sizeof(MONITORINFOEXA))
        lstrcpyA(((MONITORINFOEXA *)lpmi)->szDevice, POLYFILL_DISPLAY_NAME);
    return TRUE;
}

// EnumDisplaySettingsW/ExW polyfills
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enumdisplaysettingsw
//
// The real EnumDisplaySettingsW exists on NT 4.0 (still unmacroed here, see the note at the top); only
// the emulated device name needs replacing with NULL, meaning the current display. The Ex flags only
// widen the mode list, so ignoring them is a safe subset.
//

BOOL GLFW_EnumDisplaySettingsW(LPCWSTR lpszDeviceName, DWORD iModeNum, DEVMODEW *lpDevMode)
{
    return EnumDisplaySettingsW(NULL, iModeNum, lpDevMode);
}

BOOL GLFW_EnumDisplaySettingsExW(LPCWSTR lpszDeviceName, DWORD iModeNum, DEVMODEW *lpDevMode, DWORD dwFlags)
{
    return EnumDisplaySettingsW(NULL, iModeNum, lpDevMode);
}


// GetLayeredWindowAttributes polyfill
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getlayeredwindowattributes
//

BOOL GLFW_GetLayeredWindowAttributes(HWND hwnd, COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags)
{
    // TODO:
    return FALSE;
}

// SetLayeredWindowAttributes polyfill
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setlayeredwindowattributes
//

BOOL GLFW_SetLayeredWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags)
{
    // TODO:
    return FALSE;
}

// GetRawInputData polyfill
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getrawinputdata
//

UINT GLFW_GetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader)
{
    // TODO:
    return -1;
}

// GetRawInputDeviceInfoA polyfill
// https://learn.microsoft.com/ru-ru/windows/win32/api/winuser/nf-winuser-getrawinputdeviceinfoa
//

UINT GLFW_GetRawInputDeviceInfoA(HANDLE hDevice, UINT uiCommand, LPVOID pData, PUINT pcbSize)
{
    // TODO:
    return -1;
}

// GetRawInputDeviceList polyfill
// https://learn.microsoft.com/ru-ru/windows/win32/api/winuser/nf-winuser-getrawinputdevicelist
//

UINT GLFW_GetRawInputDeviceList(PRAWINPUTDEVICELIST pRawInputDeviceList, PUINT puiNumDevices, UINT cbSize)
{
    // TODO:
    return -1;
}

// MonitorFromWindow polyfill
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-monitorfromwindow
//

HMONITOR GLFW_MonitorFromWindow(HWND hwnd, DWORD dwFlags)
{
    // TODO:
    return NULL;
}

// MonitorFromPoint polyfill
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-monitorfrompoint
//

HMONITOR GLFW_MonitorFromPoint(POINT pt, DWORD dwFlags)
{
    // TODO:
    return NULL;
}

// RegisterDeviceNotificationW polyfill
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerdevicenotificationw
//

HDEVNOTIFY GLFW_RegisterDeviceNotificationW(HANDLE hRecipient, LPVOID NotificationFilter, DWORD Flags)
{
    // TODO:
    return NULL;
}

// RegisterRawInputDevices polyfill
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerrawinputdevices
//

BOOL GLFW_RegisterRawInputDevices(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize)
{
    return FALSE;
}

// UnregisterDeviceNotification polyfill
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-unregisterdevicenotification
//

BOOL GLFW_UnregisterDeviceNotification(HDEVNOTIFY Handle)
{
    return FALSE;
}


// ChoosePixelFormat function that "sanitizes" the input
// to better ensure compatibility with very old OpenGL ICDs
// that may not support cAlphaBits, to avoid falling back to GDI Generic.

int GLFW_ChoosePixelFormat(HDC hdc, PIXELFORMATDESCRIPTOR* pfd)
{
    // Modify in place.
    pfd->cAlphaBits = 0;
    pfd->cDepthBits = 16; // This may actually affect depth precision
    // and older GPUs should support 24 bit depth fine

    // Make a local copy (untested!)
    /* --- Option 2 (commented): make a local copy instead ---
    PIXELFORMATDESCRIPTOR safePfd = *pfd;
    safePfd.cAlphaBits = 0;
    safePfd.cDepthBits = 16;
    return ChoosePixelFormat(hdc, &safePfd);
    */

    return ChoosePixelFormat(hdc, pfd);
}

// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdpiforwindow
UINT GLFW_GetDpiForWindow(HWND hwnd)
{
    return 96; // 1x
}

// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-adjustwindowrectexfordpi
BOOL GLFW_AdjustWindowRectExForDpi(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi)
{
    return TRUE; // lol we did not adjust it
}
