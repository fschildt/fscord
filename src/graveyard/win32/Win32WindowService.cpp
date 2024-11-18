#include <stdio.h>

static DWORD g_main_thread_id;
static PIXELFORMATDESCRIPTOR g_pfd;

internal LRESULT
win32__window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    LRESULT result = 0;
    switch (message)
    {
        case WM_CLOSE:
        case WM_QUIT:
        case WM_CHAR:
        case WM_SIZE:
        {
            PostThreadMessage(g_main_thread_id, message, w_param, l_param);
        }
        break;

        case WM_KEYDOWN:
        {
            if (w_param >= VK_LEFT && w_param <= VK_DOWN)
            {
                PostThreadMessage(g_main_thread_id, message, w_param, l_param);
            }
            else
            {
                // let windows turn this into a WM_CHAR
                result = DefWindowProcA(window, message, w_param, l_param);
            }
        }
        break;

        default:
        {
            result = DefWindowProcA(window, message, w_param, l_param);
        }
        break;
    }

    return result;
}

internal HWND
win32__create_window(const char *name, int width, int height)
{
    const char *classname = "win32_window_class";

    WNDCLASSEX window_class = {};
    window_class.style = CS_OWNDC;
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = &win32__window_proc;
    window_class.hInstance = GetModuleHandle(NULL);
    window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    window_class.lpszClassName = classname;
    if (RegisterClassExA(&window_class) == 0)
    {
        printf("RegisterClassEx() failed\n");
        return 0;
    }

    HWND window = CreateWindowExA(0,
                                  classname,
                                  name,
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  width,
                                  height,
                                  0,
                                  0,
                                  window_class.hInstance,
                                  0);
    if (!window)
    {
        printf("CreateWindowEx() failed\n");
    }

    return window;
}

internal void
win32__destroy_window(HWND window)
{
    DestroyWindow(window);
}

internal LRESULT
win32__service_window_proc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	LRESULT result = 0;
	switch (message)
	{
        case Win32WindowService::CREATE_WINDOW:
        {
            Win32WindowSettings *settings = (Win32WindowSettings*)w_param;

            HWND handle = win32__create_window(settings->name, settings->width, settings->height);
            result = (LRESULT)handle;
        }
        break;

        case Win32WindowService::DESTROY_WINDOW:
        {
            win32__destroy_window(window);
        }
        break;

        default:
        {
            result = DefWindowProc(window, message, w_param, l_param);
        }
        break;
	}
	return result;
}

HWND
win32__create_service_window()
{
    PIXELFORMATDESCRIPTOR *pfd = &g_pfd;
    memset(pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    pfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd->nVersion = 1;
    pfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    pfd->iPixelType = PFD_TYPE_RGBA;
    pfd->cColorBits = 32;
    pfd->cDepthBits = 24;
    pfd->cStencilBits = 8;
    pfd->iLayerType = PFD_MAIN_PLANE;

    WNDCLASSEX swc = {};
    swc.style         = CS_OWNDC;
    swc.cbSize        = sizeof(swc);
    swc.lpfnWndProc   = &win32__service_window_proc;
    swc.hInstance     = GetModuleHandle(NULL);
    swc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    swc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    swc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    swc.lpszClassName = "win32_service_window_class";
    if (RegisterClassEx(&swc) == 0)
    {
        printf("RegisterClassEx() failed\n");
        return 0;
    }

    const char *name = "win32_service_window";
    HWND handle = CreateWindowEx(0,
                                 swc.lpszClassName,
                                 name,
                                 0,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 0,
                                 0,
                                 swc.hInstance,
                                 0);
    if (!handle)
    {
        printf("CreateWindowEx() for service window failed\n");
        return 0;
    }

    HDC dc = GetDC(handle);
    if (!dc)
    {
        printf("GetDC() for service window failed\n");
        DestroyWindow(handle);
        return 0;
    }

    int pixel_format = ChoosePixelFormat(dc, &g_pfd);
    if (pixel_format == 0)
    {
        printf("ChoosePixelFormat() for service window failed\n");
        DestroyWindow(handle);
        return 0;
    }

    BOOL pixel_format_set = SetPixelFormat(dc, pixel_format, &g_pfd);
    if (pixel_format_set == FALSE)
    {
        printf("SetPixelFormat() for service window failed\n");
        DestroyWindow(handle);
        return 0;
    }

    return handle;
}

DWORD WINAPI
win32__window_service_thread(LPVOID data)
{
    Win32WindowService *window_service = (Win32WindowService*)data;

    // create service window and report back to main thread
    HWND service_window = win32__create_service_window();
    window_service->m_ServiceWindow = service_window;
    if (!service_window)
    {
        PostThreadMessage(g_main_thread_id, Win32WindowService::SERVICE_WINDOW_NOT_CREATED, 0, 0);
        return 0;
    }
    PostThreadMessage(g_main_thread_id, Win32WindowService::SERVICE_WINDOW_CREATED, 0, 0);

    // get events for all windows, redirect to service window proc, let it report back
    for (;;)
    {
        MSG message;
        BOOL recvd = GetMessageA(&message, 0, 0, 0);
        if (recvd == -1)
        {
            printf("GetMessage failed\n");
            return 0;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}

bool
Win32WindowService::Init()
{
    DWORD main_thread_id = GetCurrentThreadId();
    g_main_thread_id = main_thread_id;

    DWORD tid;
    HANDLE thread = CreateThread(0, 0, win32__window_service_thread, this, 0, &tid);
    if (!thread)
    {
        printf("CreatThread() when starting window manager failed\n");
        return false;
    }

    // wait until service window is ready
    for (;;)
    {
        MSG message;
        GetMessageA(&message, 0, 0, 0);

        if (message.message == Win32WindowService::SERVICE_WINDOW_CREATED)
            return true;
        else if (message.message == Win32WindowService::SERVICE_WINDOW_NOT_CREATED)
            return false;
        else assert(0);
    }
}

Win32Window *
Win32WindowService::CreateWin32Window(const char *name, int width, int height)
{
    static int counter = 0;
    assert(counter++ == 0);

    // let the service (other thread) create the window
    Win32WindowSettings settings = {};
    settings.name = name;
    settings.width = width;
    settings.height = height;

    HWND handle = (HWND)SendMessage(m_ServiceWindow, Win32WindowService::CREATE_WINDOW, (WPARAM)&settings, 0);
    if (!handle) return 0;


    // do remaining initialization
    HDC dc = GetDC(handle);
    if (!dc)
    {
        printf("GetDC failed\n");
        return 0;
    }

    PIXELFORMATDESCRIPTOR *pfd = &g_pfd;
    int pixel_format = ChoosePixelFormat(dc, pfd);
    if (pixel_format == 0)
    {
        printf("ChoosePixelFormat failed\n");
        return false;
    }

    BOOL pixel_format_set = SetPixelFormat(dc, pixel_format, &g_pfd);
    if (pixel_format_set == FALSE)
    {
        printf("SetPixelFormat() failed\n");
        return false;
    }

    Win32Window *window = new Win32Window(handle, dc);
    if (!window->RecreateOffscreenBuffer(width, height))
    {
        delete window;
        window = 0;
    }
    return window;
}

void
Win32WindowService::DestroyWin32Window(Win32Window *window)
{
    SendMessage(m_ServiceWindow, Win32WindowService::DESTROY_WINDOW, (WPARAM)window, 0);
    delete window;
}

i32
Win32WindowService::GetWindowEvents(Win32Window *window, Sys_Event *events, i32 max_event_count)
{
    i32 event_count = 0;

    // This receives messages for all windows, so we can only have 1 window atm.
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) && event_count < max_event_count)
    {
        Sys_Event *event = events + event_count;
        switch (msg.message)
        {
#define ADD_KEY_EVENT(keycode) \
    event->type = SYS_EVENT_KEY; \
    event->key.c = (char)keycode; \
    event_count += 1;
            case WM_CHAR:
            {
                char keycode = (char)msg.wParam;
                bool is_down = (msg.lParam & (1<<31)) == 0;
                if (is_down)
                {
                    ADD_KEY_EVENT(keycode);
                }
            }
            break;

            case WM_KEYDOWN:
            {
                if      (msg.wParam == VK_LEFT)  {ADD_KEY_EVENT(SYS_KEY_LEFT)}
                else if (msg.wParam == VK_UP)    {ADD_KEY_EVENT(SYS_KEY_UP)}
                else if (msg.wParam == VK_RIGHT) {ADD_KEY_EVENT(SYS_KEY_RIGHT)}
                else if (msg.wParam == VK_DOWN)  {ADD_KEY_EVENT(SYS_KEY_DOWN)}
                else if (msg.wParam == VK_TAB)   {ADD_KEY_EVENT('\t')}
                else if (msg.wParam == VK_RETURN){ADD_KEY_EVENT('\r')}
                else {}
            }
            break;
#undef ADD_KEY_EVENT

            case WM_SIZE:
            {
                i32 width = (i32)LOWORD(msg.lParam);
                i32 height = (i32)HIWORD(msg.lParam);
                window->RecreateOffscreenBuffer(width, height);
            }
            break;

            case WM_CLOSE:
            case WM_QUIT:
            {
                return -1;
            }
            break;


            default:
            {
                printf("unhandled window event %d\n", msg.message);
            }
        }
    }

    return event_count;
}

