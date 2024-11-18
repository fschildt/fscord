#include <platform/platform.h>
#include <windows.h>

struct Win32OffscreenBuffer {
    PlatformOffscreenBuffer platform_offscreen_buffer;
    BITMAPINFO bitmap_info;
};

struct PlatformWindow {
    HWND wnd;
    HDC dc;
    Win32OffscreenBuffer offscreen_buffer;
};

Win32Window::Win32Window(HWND handle, HDC dc)
{
    m_Wnd = handle;
    m_Dc = dc;
    memset((void*)&m_OffscreenBuffer, 0, sizeof(m_OffscreenBuffer));
}

bool
Win32Window::RecreateOffscreenBuffer(i32 width, i32 height)
{
    Sys_Offscreen_Buffer *sys_screen = &m_OffscreenBuffer.sys_offscreen_buffer;

    sys_screen->red_shift   = 16;
    sys_screen->green_shift = 8;
    sys_screen->blue_shift  = 0;
    sys_screen->alpha_shift = 24;

    u32 *new_pixels = (u32*)malloc(sizeof(sys_screen->pixels[0]) * width * height);
    if (!new_pixels)
    {
        return false;
    }
    free(sys_screen->pixels);

    sys_screen->width = width;
    sys_screen->height = height;
    sys_screen->pixels = new_pixels;


    BITMAPINFO *bmi = &m_OffscreenBuffer.bitmap_info;
    memset(bmi, 0, sizeof(*bmi));
    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biWidth = width;
    bmi->bmiHeader.biHeight = height;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biBitCount = 32;
    bmi->bmiHeader.biCompression = BI_RGB;

    return true;
}

Sys_Offscreen_Buffer *
Win32Window::GetOffscreenBuffer()
{
    Sys_Offscreen_Buffer *screen = &m_OffscreenBuffer.sys_offscreen_buffer;
    return screen;
}

void
Win32Window::ShowOffscreenBuffer()
{
    i32 width = m_OffscreenBuffer.sys_offscreen_buffer.width;
    i32 height = m_OffscreenBuffer.sys_offscreen_buffer.height;
    u32 *pixels = m_OffscreenBuffer.sys_offscreen_buffer.pixels;
    BITMAPINFO *bmi = &m_OffscreenBuffer.bitmap_info;

    StretchDIBits(m_Dc,
                  0, 0, width, height,
                  0, 0, width, height,
                  pixels, bmi, DIB_RGB_COLORS, SRCCOPY);

}

