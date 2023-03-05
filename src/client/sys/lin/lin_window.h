typedef struct
{
    Sys_Offscreen_Buffer sys_offscreen_buffer;
    u32 gl_texture_id;
} Lin_Offscreen_Buffer;

typedef struct
{
    Sys_Window sys_window;
    Window xid;
    Atom wm_delete_window;
    u32 event_mask;
    u32 gl_texture_id;
    GLXContext glx_context;

    Lin_Offscreen_Buffer offscreen_buffer;
} Lin_Window;
