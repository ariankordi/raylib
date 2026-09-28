// Source: https://github.com/rndtrash/glfw/blob/7dea60cb4fe97090094d60a453a3d1c9e9a7902e/src/win32_platform.h#L135
// Local modifications (not upstream), 2026-09: GetMonitorInfoA gets its own polyfill instead of aliasing
// the W one, and EnumDisplayDevicesW is added. See the notes at the top of win32_polyfill.c.

#include <windows.h>

#if WINVER < 0x0A00 || _WIN32_WINNT < 0x0A00

// Polyfills for Windows versions below 10 - specifically, version 1607
#define GetDpiForWindow GLFW_GetDpiForWindow
UINT GetDpiForWindow(HWND hwnd); // always 96

#define AdjustWindowRectExForDpi GLFW_AdjustWindowRectExForDpi
BOOL AdjustWindowRectExForDpi(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);

#if WINVER < 0x0501 || _WIN32_WINNT < 0x0501

// Polyfills for Windows versions below XP
#define SetThreadExecutionState GLFW_SetThreadExecutionState
EXECUTION_STATE GLFW_SetThreadExecutionState(EXECUTION_STATE esFlags);

#define GetModuleHandleExW GLFW_GetModuleHandleExW
BOOL GLFW_GetModuleHandleExW(DWORD dwFlags, LPCWSTR lpModuleName, HMODULE *phModule);

#define GetLayeredWindowAttributes GLFW_GetLayeredWindowAttributes
BOOL GLFW_GetLayeredWindowAttributes(HWND hwnd, COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags);

#define SetLayeredWindowAttributes GLFW_SetLayeredWindowAttributes
BOOL GLFW_SetLayeredWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

#define GetRawInputData GLFW_GetRawInputData
UINT GLFW_GetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader);

#define GetRawInputDeviceInfoA GLFW_GetRawInputDeviceInfoA
UINT GLFW_GetRawInputDeviceInfoA(HANDLE hDevice, UINT uiCommand, LPVOID pData, PUINT pcbSize);

#define GetRawInputDeviceList GLFW_GetRawInputDeviceList
UINT GLFW_GetRawInputDeviceList(PRAWINPUTDEVICELIST pRawInputDeviceList, PUINT puiNumDevices, UINT cbSize);

#define RegisterDeviceNotificationW GLFW_RegisterDeviceNotificationW
HDEVNOTIFY GLFW_RegisterDeviceNotificationW(HANDLE hRecipient, LPVOID NotificationFilter, DWORD Flags);

#define RegisterRawInputDevices GLFW_RegisterRawInputDevices
BOOL GLFW_RegisterRawInputDevices(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize);

#define UnregisterDeviceNotification GLFW_UnregisterDeviceNotification
BOOL GLFW_UnregisterDeviceNotification(HDEVNOTIFY Handle);

// Wrapper for ChoosePixelFormat that "sanitizes" the input.
#define ChoosePixelFormat GLFW_ChoosePixelFormat
int GLFW_ChoosePixelFormat(HDC hdc, PIXELFORMATDESCRIPTOR* pfd);

#if WINVER < 0x0500 || _WIN32_WINNT < 0x0500

// Polyfills for Windows versions below 2000

#define VerSetConditionMask GLFW_VerSetConditionMask
ULONGLONG GLFW_VerSetConditionMask(ULONGLONG dwlConditionMask, DWORD dwTypeBitMask, BYTE dwConditionMask);

// Display polyfills (local addition): emulate one primary display. See win32_polyfill.c.

#define GetMonitorInfoW GLFW_GetMonitorInfoW
BOOL GLFW_GetMonitorInfoW(HMONITOR hMonitor, LPMONITORINFO lpmi);

#define GetMonitorInfoA GLFW_GetMonitorInfoA
BOOL GLFW_GetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO lpmi);

#define EnumDisplayMonitors GLFW_EnumDisplayMonitors
BOOL GLFW_EnumDisplayMonitors(HDC hdc, LPCRECT lprcClip, MONITORENUMPROC lpfnEnum, LPARAM dwData);

// NT 4.0's user32 exports an undocumented 3-parameter EnumDisplayDevicesW; calling it through the SDK's
// 4-parameter prototype unbalances the stack, so this must never reach it.
#define EnumDisplayDevicesW GLFW_EnumDisplayDevicesW
BOOL GLFW_EnumDisplayDevicesW(LPCWSTR lpDevice, DWORD iDevNum, PDISPLAY_DEVICEW lpDisplayDevice, DWORD dwFlags);

#define EnumDisplaySettingsW GLFW_EnumDisplaySettingsW
BOOL GLFW_EnumDisplaySettingsW(LPCWSTR  lpszDeviceName, DWORD iModeNum, DEVMODEW *lpDevMode);

#define EnumDisplaySettingsExW GLFW_EnumDisplaySettingsExW
BOOL GLFW_EnumDisplaySettingsExW(LPCWSTR lpszDeviceName, DWORD iModeNum, DEVMODEW *lpDevMode, DWORD dwFlags);

#define MonitorFromWindow GLFW_MonitorFromWindow
HMONITOR GLFW_MonitorFromWindow(HWND hwnd, DWORD dwFlags);

#define MonitorFromPoint GLFW_MonitorFromPoint
HMONITOR GLFW_MonitorFromPoint(POINT pt, DWORD dwFlags);

#endif // WINVER < 0x0500 || _WIN32_WINNT < 0x0500

#endif // WINVER < 0x0501 || _WIN32_WINNT < 0x0501

#endif // WINVER < 0x0A00 || _WIN32_WINNT < 0x0A00
