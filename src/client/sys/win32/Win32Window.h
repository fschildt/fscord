struct Win32OffscreenBuffer
{
    Sys_Offscreen_Buffer sys_offscreen_buffer;
    BITMAPINFO bitmap_info;
};

class Win32Window
{
public:
    Win32Window(HWND handle, HDC dc);
    bool RecreateOffscreenBuffer(s32 width, s32 height);
    Sys_Offscreen_Buffer* GetOffscreenBuffer();
    void ShowOffscreenBuffer();

private:
    Sys_Window m_SysWindow;
    HWND m_Wnd;
    HDC m_Dc;
    Win32OffscreenBuffer m_OffscreenBuffer;
};

