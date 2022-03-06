#ifndef ACTOR_H
#define ACTOR_H

#include <unordered_map>

#include <assert.h>

#include <SDL.h>
#include <SDL_gpu.h>

class SpriteSheet;

class Sprite {
private:
    friend class SpriteSheet;

    const uint32_t idx_start;
    const uint32_t idx_end;

    const double fps_base;

    const SpriteSheet &sprite_sheet;

    uint32_t idx_sprite_frame;

    Sprite(
        const uint32_t idx_start,
        const uint32_t idx_end,
        const double fps_base,
        const SpriteSheet &sprite_sheet
    ):
        idx_start(idx_start),
        idx_end(idx_end),
        fps_base(fps_base),
        sprite_sheet(sprite_sheet),
        idx_sprite_frame(idx_start)
    {}
};

class SpriteSheet {
private:
    const uint32_t sheet_width;
    const uint32_t sheet_height;

    const uint32_t sprite_width;
    const uint32_t sprite_height;

    GPU_Image* sheet_texture;

public:
    SpriteSheet(
        const uint32_t sheet_width,
        const uint32_t sheet_height,
        const uint32_t sprite_width,
        const uint32_t sprite_height,
        const std::string &filename
    ):
        sheet_width(sheet_width),
        sheet_height(sheet_height),
        sprite_width(sprite_width),
        sprite_height(sprite_height),
        sheet_texture(GPU_LoadImage(filename.c_str()))
    {
        assert(sprite_width <= sheet_width);
        assert(sheet_width % sprite_width == 0);

        assert(sprite_height <= sheet_height);
        assert(sheet_height % sprite_height == 0);
    }

    ~SpriteSheet() {
        GPU_FreeImage(sheet_texture);
    }

    Sprite get_sprite(
        const uint32_t idx_start,
        const uint32_t idx_end,
        const double fps_base
    ) const {
        return Sprite(idx_start, idx_end, fps_base, *this);
    }
};

enum class E_SpriteSheet : uint32_t {
    CharSheet
};

enum class E_Sprite : uint32_t {
    Walker
};

class TextureAtlas {
private:
    std::unordered_map<E_SpriteSheet, SpriteSheet> id_to_spritesheet;
    std::unordered_map<E_Sprite, Sprite> id_to_sprite;

public:
    TextureAtlas() {}

    // Add a spritesheet to the map. Asserts that this is a new insertion.
    //
    // TODO: Could we do this either during program startup
    // (static initialization) or possibly even during compilation?
    const decltype(id_to_spritesheet)::mapped_type & add_spritesheet(
        const E_SpriteSheet id_sheet,
        const uint32_t sheet_width,
        const uint32_t sheet_height,
        const uint32_t sprite_width,
        const uint32_t sprite_height,
        const std::string &filename
    ) {
        const auto &[iter, inserted] = id_to_spritesheet.try_emplace(
            id_sheet,
            sheet_width,
            sheet_height,
            sprite_width,
            sprite_height,
            filename
        );

        assert(inserted);

        return iter->second;
    }
};

class Actor {

};

#endif
