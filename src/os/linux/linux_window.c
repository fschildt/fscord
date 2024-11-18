#include <basic/basic.h>
#include <os/os.h>

#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>


struct OSWindow {
    Window xid;
    Atom wm_delete_window;
    u32 event_mask;

    i32 width;
    i32 height;

    GLXContext glx_context;
    u32 gl_texture_id;
    OSOffscreenBuffer offscreen_buffer;

    OSEvent current_event;
};


internal_var Display *g_display;

internal_fn b32
os_connect_to_x()
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


void
os_window_destroy_offscreen_buffer(OSOffscreenBuffer *offscreen_buffer)
{
    free(offscreen_buffer->pixels);
}

void
os_offscreen_buffer_resize(OSOffscreenBuffer *offscreen, i32 width, i32 height)
{
    u32 *new_pixels = realloc(offscreen->pixels, width*height*4);
    if (new_pixels) {
        offscreen->width = width;
        offscreen->height = height;
        offscreen->pixels = new_pixels;
        glViewport(0, 0, width, height);
    }
}

void
os_offscreen_buffer_destroy(OSOffscreenBuffer *offscreen_buffer)
{
    free(offscreen_buffer->pixels);
    free(offscreen_buffer);
}

OSOffscreenBuffer *
os_offscreen_buffer_create(i32 width, i32 height)
{
    OSOffscreenBuffer *offscreen_buffer = malloc(sizeof(OSOffscreenBuffer));
    if (offscreen_buffer) {
        offscreen_buffer->green_shift = 8;
        offscreen_buffer->blue_shift = 16;
        offscreen_buffer->red_shift = 0;
        offscreen_buffer->alpha_shift = 24;
        offscreen_buffer->width = 0;
        offscreen_buffer->height = 0;
        offscreen_buffer->pixels = 0;
    }
    os_offscreen_buffer_resize(offscreen_buffer, width, height);
    return offscreen_buffer;
}

void
os_window_swap_buffers(OSWindow *window, OSOffscreenBuffer *offscreen_buffer)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, window->gl_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, offscreen_buffer->width, offscreen_buffer->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, offscreen_buffer->pixels);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);

    glXSwapBuffers(g_display, window->xid);
}


#define ADD_KEY_PRESS(c) \
    event->type  = OS_EVENT_KEY_PRESS; \
    event->key_press.code = c; \
    return true;

b32
os_window_get_event(OSWindow *window, OSEvent *event)
{
    while (XPending(g_display)) {
        XEvent xevent;
        XNextEvent(g_display, &xevent);

        // Todo: Rework this whole shift/caps-lock logic.
        persist_var b32 is_caps;
        persist_var b32 is_shift_l;
        persist_var b32 is_shift_r;
        b32 is_uppercase = ( is_caps && !(is_shift_l || is_shift_r)) ||
                           (!is_caps &&  (is_shift_l || is_shift_r));

        switch (xevent.type) {
            case ClientMessage: {
                if ((Atom)xevent.xclient.data.l[0] == window->wm_delete_window) {
                    event->type = OS_EVENT_WINDOW_DESTROYED;
                    return true;
                }
                continue;
            } break;

            case DestroyNotify: {
                event->type = OS_EVENT_WINDOW_DESTROYED;
                return true;
            } break;

            case ConfigureNotify: {
                i32 width = xevent.xconfigure.width;
                i32 height = xevent.xconfigure.height;
                if (width != window->width || height != window->height) {
                    event->type = OS_EVENT_WINDOW_RESIZE;
                    event->width = width;
                    event->height = height;
                    window->width = width;
                    window->height = height;
                    return true;
                }
                continue;
            } break;

            case KeyPress: {
                int index = is_uppercase ? 1 : 0;
                KeySym keysym = XLookupKeysym(&xevent.xkey, index);
                if (keysym >= 32 && keysym <= 126) {ADD_KEY_PRESS(keysym);}
                else if (keysym == XK_Tab)         {ADD_KEY_PRESS('\t');  }
                else if (keysym == XK_Return)      {ADD_KEY_PRESS('\r');  }
                else if (keysym == XK_BackSpace)   {ADD_KEY_PRESS(8);     }
                else if (keysym == XK_Delete)      {ADD_KEY_PRESS(127);   } 
                else if (keysym == XK_Left)        {ADD_KEY_PRESS(OS_KEYCODE_LEFT); }
                else if (keysym == XK_Right)       {ADD_KEY_PRESS(OS_KEYCODE_RIGHT);}
                else if (keysym == XK_Up)          {ADD_KEY_PRESS(OS_KEYCODE_UP);   }
                else if (keysym == XK_Down)        {ADD_KEY_PRESS(OS_KEYCODE_DOWN); }
                else if (keysym == XK_Shift_L)     is_shift_l = true;
                else if (keysym == XK_Shift_R)     is_shift_r = true;
                else if (keysym == XK_Caps_Lock)   is_caps = true;
            } break;

            case KeyRelease: {
                int index = 0;
                KeySym keysym = XLookupKeysym(&xevent.xkey, index);
                if (keysym == XK_Shift_L)        is_shift_l = false;
                else if (keysym == XK_Shift_R)   is_shift_r = false;
                else if (keysym == XK_Caps_Lock) is_caps = false;
                continue; // ignore this for now i guess
            } break;

            case GraphicsExpose: {
                printf("graphics exposure happened\n");
                continue;
            } break;

            default:;
        }
    }
    return false;
}
#undef ADD_KEY_PRESS

void os_window_destroy(OSWindow *window) {
    os_window_destroy_offscreen_buffer(&window->offscreen_buffer);
    XDestroyWindow(g_display, window->xid);
    free(window);
}

OSWindow *os_window_create(const char *name, i32 width, i32 height) {
    if (!g_display) {
        if (!os_connect_to_x())
            return 0;
    }

    int screen_number = XDefaultScreen(g_display);
    Window root_window_id = XRootWindow(g_display, screen_number);
    if (!root_window_id)
    {
        printf("XDefaultRootWindow(display) failed\n");
        XCloseDisplay(g_display);
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
    XVisualInfo* vinfo = glXChooseVisual(g_display, screen_number, va);
    if (!vinfo)
    {
        printf("glXChooseVisual failed\n");
        return 0;
    }

    // set window attribs
    long swa_mask = CWEventMask | CWColormap | CWBackPixmap |CWBorderPixel; 
    long swa_event_mask = PropertyChangeMask | SubstructureNotifyMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask;
    Colormap colormap = XCreateColormap(g_display, root_window_id, vinfo->visual, AllocNone);

    XSetWindowAttributes swa = {};
    swa.background_pixmap = None;
    swa.border_pixel = 0;
    swa.event_mask = swa_event_mask;
    swa.colormap = colormap;

    Window window_id = XCreateWindow(g_display, root_window_id, 0, 0, width, height, 0, depth, InputOutput, vinfo->visual, swa_mask, &swa);
    XStoreName(g_display, window_id, name);
    XMapWindow(g_display, window_id);


    // I support the WM_DELETE_WINDOW protocol @Leak?
    Atom wm_delete_window = XInternAtom(g_display, "WM_DELETE_WINDOW", False); 
    XSetWMProtocols(g_display, window_id, &wm_delete_window, 1);


    // glx context
    Bool direct = True;
    GLXContext glx_context = glXCreateContext(g_display, vinfo, 0, direct);
    if (glx_context == 0)
    {
        printf("glXCreateContext failed\n");
        return 0;
    }
    Bool made_current = glXMakeCurrent(g_display, window_id, glx_context);
    if (made_current == False)
    {
        printf("glXMakeCurrent failed\n");
        return 0;
    }


    OSWindow *window = malloc(sizeof(OSWindow));
    window->xid = window_id;
    window->wm_delete_window = wm_delete_window;
    window->event_mask = swa_event_mask;
    window->glx_context = glx_context;
    XSync(g_display, False);


    glEnable(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &window->gl_texture_id);
    glBindTexture(GL_TEXTURE_2D, window->gl_texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    return window;
}

