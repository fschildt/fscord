static Display *g_display;

internal void
lin_show_offscreen_buffer(Lin_Window *window)
{
    Lin_Offscreen_Buffer *lob = &window->offscreen_buffer;
    Sys_Offscreen_Buffer *sob = (Sys_Offscreen_Buffer*)lob;
    s32 width   = sob->width;
    s32 height  = sob->height;
    u32 *pixels = sob->pixels;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lob->gl_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);

    glXSwapBuffers(g_display, window->xid);
}

internal b32
lin_recreate_offscreen_buffer(Lin_Offscreen_Buffer *offscreen_buffer, s32 width, s32 height)
{
    u32 bytes_per_pixel = 4;
    u32 pixels_size = width * height * bytes_per_pixel;

    Sys_Offscreen_Buffer *sob = (Sys_Offscreen_Buffer*)offscreen_buffer;

    if (!sob->pixels)
    {
        glEnable(GL_TEXTURE_2D);

        u32 texture_id;
        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        sob->green_shift = 8;
        sob->blue_shift = 16;
        sob->red_shift = 0;
        sob->alpha_shift = 24;

        offscreen_buffer->gl_texture_id = texture_id;
    }

    u32 *pixels = realloc(sob->pixels, pixels_size);
    if (!pixels)
    {
        printf("offscreen buffer pixel realloc failed\n");
        return false;
    }

    glViewport(0, 0, width, height);

    sob->width  = width;
    sob->height = height;
    sob->pixels = pixels;
    return true;
}


#define ADD_KEY_EVENT(_c) \
    event->type  = SYS_EVENT_KEY; \
    event->key.c = _c; \
    event_count += 1;

internal s32
lin_get_window_events(Lin_Window *window, Sys_Event *events, u32 max_event_count)
{
    s32 event_count = 0;

    XEvent xevent;
    while (XPending(g_display) &&
           event_count < max_event_count)
    {
        XNextEvent(g_display, &xevent);
        Sys_Event *event = events + event_count;

        static b32 is_caps;
        static b32 is_shift_l;
        static b32 is_shift_r;
        b32 is_uppercase = ( is_caps && !(is_shift_l || is_shift_r)) ||
                           (!is_caps &&  (is_shift_l || is_shift_r));

        switch (xevent.type)
        {
        case ClientMessage:
        {
            printf("closed by window manager\n");
            if ((Atom)xevent.xclient.data.l[0] == window->wm_delete_window) {
                return -1;
            }
        }
        break;

        case ConfigureNotify:
        {
            Lin_Offscreen_Buffer *ob = &window->offscreen_buffer;
            Sys_Offscreen_Buffer *sob = (Sys_Offscreen_Buffer*)ob;

            s32 width  = xevent.xconfigure.width;
            s32 height = xevent.xconfigure.height;
            if (width  != sob->width ||
                height != sob->height)
            {
                lin_recreate_offscreen_buffer(ob, width, height);
            }
        }
        break;

        case DestroyNotify:
        {
            printf("DestroyNotify processed\n");
            return -1;
        }
        break;

        // Note: keysyms defined in X11/keysymdef.h.
        case KeyPress:
        {
            int index = is_uppercase ? 1 : 0;
            KeySym keysym = XLookupKeysym(&xevent.xkey, index);
            if (keysym >= 32 && keysym <= 126) { ADD_KEY_EVENT(keysym); }
            else if (keysym == XK_Tab)         { ADD_KEY_EVENT('\t');   }
            else if (keysym == XK_Return)      { ADD_KEY_EVENT('\r');   }
            else if (keysym == XK_BackSpace)   { ADD_KEY_EVENT(8);      }
            else if (keysym == XK_Delete)      { ADD_KEY_EVENT(127);    } 
            else if (keysym == XK_Left)        { ADD_KEY_EVENT(SYS_KEY_LEFT);  }
            else if (keysym == XK_Right)       { ADD_KEY_EVENT(SYS_KEY_RIGHT); }
            else if (keysym == XK_Up)          { ADD_KEY_EVENT(SYS_KEY_UP);    }
            else if (keysym == XK_Down)        { ADD_KEY_EVENT(SYS_KEY_DOWN);  }
            else if (keysym == XK_Shift_L)     is_shift_l = true;
            else if (keysym == XK_Shift_R)     is_shift_r = true;
            else if (keysym == XK_Caps_Lock)   is_caps = true;
        }
        break;

        case KeyRelease:
        {
            int index = 0;
            KeySym keysym = XLookupKeysym(&xevent.xkey, index);

            if (keysym == XK_Shift_L)        is_shift_l = false;
            else if (keysym == XK_Shift_R)   is_shift_r = false;
            else if (keysym == XK_Caps_Lock) is_caps = false;
        }
        break;

        case GraphicsExpose:
        {
            printf("graphics exposure happened\n");
        }
        break;

        default:;
        }
    }
    return event_count;
}
#undef ADD_KEY_EVENT

void
lin_destroy_window(Lin_Window *window)
{
    XDestroyWindow(g_display, window->xid);
}

internal b32
lin_connect_to_x()
{
    const char *display_name = 0;
    g_display = XOpenDisplay(display_name);
    if (!g_display)
    {
        printf("XOpenDisplay(%s) failed\n", display_name);
        return false;
    }

    return true;
}

Lin_Window *
lin_create_window(const char *name, s32 width, s32 height)
{
    if (!g_display)
    {
        if (!lin_connect_to_x())
            return 0;
    }
    Display *display = g_display;

    int screen_number = XDefaultScreen(display);
    Window root_window_id = XRootWindow(display, screen_number);
    if (!root_window_id)
    {
        printf("XDefaultRootWindow(display) failed\n");
        XCloseDisplay(display);
        return 0;
    }

    // get visual info
    int depth = 24;
    int va[] = {
        GLX_RGBA, 1,
        GLX_DOUBLEBUFFER, 1,
        GLX_RED_SIZE,   8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE,  8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, depth,
        None
    };
    XVisualInfo* vinfo = glXChooseVisual(display, screen_number, va);
    if (!vinfo)
    {
        printf("glXChooseVisual failed\n");
        return 0;
    }

    // set window attribs
    long swa_mask = CWEventMask | CWColormap | CWBackPixmap |CWBorderPixel; 
    long swa_event_mask = PropertyChangeMask | SubstructureNotifyMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask;
    Colormap colormap = XCreateColormap(display, root_window_id, vinfo->visual, AllocNone);

    XSetWindowAttributes swa = {};
    swa.background_pixmap = None;
    swa.border_pixel = 0;
    swa.event_mask = swa_event_mask;
    swa.colormap = colormap;

    Window window_id = XCreateWindow(display, root_window_id, 0, 0, width, height, 0, depth, InputOutput, vinfo->visual, swa_mask, &swa);
    XStoreName(display, window_id, name);
    XMapWindow(display, window_id);


    // I support the WM_DELETE_WINDOW protocol @Leak?
    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False); 
    XSetWMProtocols(display, window_id, &wm_delete_window, 1);


    // glx context
    Bool direct = True;
    GLXContext glx_context = glXCreateContext(display, vinfo, 0, direct);
    if (glx_context == 0)
    {
        printf("glXCreateContext failed\n");
        return 0;
    }
    Bool made_current = glXMakeCurrent(display, window_id, glx_context);
    if (made_current == False)
    {
        printf("glXMakeCurrent failed\n");
        return 0;
    }

    static int counter = 0;
    assert(counter++ == 0);

    static Lin_Window window;
    window.xid = window_id;
    window.wm_delete_window = wm_delete_window;
    window.event_mask = swa_event_mask;
    window.glx_context = glx_context;
    if (!lin_recreate_offscreen_buffer(&window.offscreen_buffer, width, height))
    {
        return 0;
    }


    XSync(display, False);
    return &window;
}

