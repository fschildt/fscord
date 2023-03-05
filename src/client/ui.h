// @Speed,@Space: We don't need all these indirections

typedef enum
{
    UI_TEXTBOX,
    UI_TEXT,

    UI_USERLIST,
    UI_HISTORY,

    UI_LOGIN,
    UI_SESSION,
} Ui_Entity_Type;

struct Ui_Entity_Header;
typedef struct Ui_Entity_Header Ui_Entity_Header;
struct Ui_Entity_Header
{
    Ui_Entity_Type type;

    V2 pos;
    V2 dim;

    Ui_Entity_Header *parent;
    Ui_Entity_Header *next;
    Ui_Entity_Header *first_child;
    Ui_Entity_Header *last_child;
};

typedef struct {
    Ui_Entity_Header header;
    s32 cursor;
    s32 len;
    s32 size;
    char *buffer;
} Ui_Textbox;

typedef struct {
    Ui_Entity_Header header;
    s32 len;
    s32 size;
    char *buffer;
} Ui_Text;


typedef struct
{
    Ui_Entity_Header header;
    Ui_Text *username_hint;
    Ui_Text *servername_hint;
    Ui_Textbox *username;
    Ui_Textbox *servername;
    Ui_Text *warning;
} Ui_Login;


typedef struct
{
    Ui_Entity_Header header;
    Message_History *history;
} Ui_History;

typedef struct
{
    Ui_Entity_Header header;
    Userlist *userlist;
} Ui_Userlist;

typedef struct
{
    Ui_Userlist *userlist;
    Ui_Textbox *prompt;
    Ui_History *history;
} Ui_Session;


typedef struct
{
    bool is_login;
    Memory_Arena arena;

    Font *font;
    f32 textbox_border_size;
    f32 textbox_text_xoff;
    f32 textbox_text_yoff;

    Ui_Login *login;
    Ui_Session *session;

    Ui_Entity_Header *curr_parent;
    Ui_Entity_Header *active;
} Ui;
