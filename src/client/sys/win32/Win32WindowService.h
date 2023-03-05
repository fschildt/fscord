struct Win32WindowSettings
{
    const char *name;
    int width;
    int height;
};

class Win32WindowService
{
public:
    bool Init();

    Win32Window *CreateWin32Window(const char *name, int width, int height);
    void DestroyWin32Window(Win32Window *window);
    s32 GetWindowEvents(Win32Window *window, Sys_Event *events, s32 max_event_count);
    void ShowOffscreenBuffer(Win32Window *window);

    enum
    {
        SERVICE_WINDOW_CREATED     = (WM_USER + 0),
        SERVICE_WINDOW_NOT_CREATED = (WM_USER + 1),
        CREATE_WINDOW              = (WM_USER + 2),
        DESTROY_WINDOW             = (WM_USER + 3),
        WINDOW_CREATED             = (WM_USER + 4),
        WINDOW_DESTROYED           = (WM_USER + 5)
    };

private:
    void RecreateOffscreenBuffer(Sys_Offscreen_Buffer *buff, s32 width, s32 height);

public:
    HWND m_ServiceWindow;
};
