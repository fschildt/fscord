#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H


#include <basic/basic.h>
#include <client/font.h>
#include <client/sound.h>

Font *asset_manager_load_font(void);
Sound *asset_manager_load_sound(i32 id);


#endif // ASSET_MANAGER_H
