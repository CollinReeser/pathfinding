#include "Actor.h"

std::pair<GPU_Image*, GPU_Rect>
Sprite::get_sprite_texture() const {
    return {
        sprite_sheet.get_sheet_texture(),
        GPU_Rect(
            0, 0, sprite_sheet.sprite_width, sprite_sheet.sprite_height
        )
    };
}