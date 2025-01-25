#include <os/os.h>
#include <client/font.h>
#include <basic/math.h>
#include <basic/string32.h>

void draw_rectf32(OSOffscreenBuffer *screen, RectF32 rect, V3F32 color);
void draw_mono_bitmap(OSOffscreenBuffer *screen, V2F32 pos, Bitmap *bitmap, V3F32 color);
void draw_string32(OSOffscreenBuffer *screen, V2F32 pos, String32 *text, Font *font);
void draw_border(OSOffscreenBuffer *screen, RectF32 rect, f32 size, V3F32 color);
