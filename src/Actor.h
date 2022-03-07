#ifndef ACTOR_H
#define ACTOR_H

#include "Map.h"
#include "Util.h"

#include <string>
#include <unordered_map>
#include <vector>

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
public:
    std::pair<GPU_Image*, GPU_Rect>
    get_sprite_texture() const;
};

class SpriteSheet {
public:
    const uint32_t sheet_width;
    const uint32_t sheet_height;

    const uint32_t sprite_width;
    const uint32_t sprite_height;

public:
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

    GPU_Image* get_sheet_texture() const {
        return sheet_texture;
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

struct PathDelta {
public:
    const std::pair<uint32_t, uint32_t> base_tile;
    const std::pair<uint32_t, uint32_t> target_tile;
    const double travelled;

    PathDelta(
        const std::pair<uint32_t, uint32_t> &base_tile,
        const std::pair<uint32_t, uint32_t> &target_tile,
        const double travelled
    ):
        base_tile(base_tile),
        target_tile(target_tile),
        travelled(travelled)
    {}
};

#include <iostream>

class ActorPath {
private:
    std::vector<std::pair<uint32_t, uint32_t>> path;

    // Index into path, the current tile we're moving "away" from, "toward"
    // the next tile in the path.
    uint32_t cur_tile_from {0};

    // The delta distance, [0, 1), we are from the current tile to the next tile
    // in the path.
    double cur_tile_delta {0};

    bool done {false};

public:
    // TODO: Pass-by-copy/value is terrible. Can we move the path, or even just
    // pass by reference? Causes compiler errors, but might be entt thing.
    ActorPath(std::vector<std::pair<uint32_t, uint32_t>> path):
        path(std::move(path))
    {}

    // TODO: entt seems to need move constructor, operator=, and does not like
    // construction via move of `path` arg or even reference to other path.
    ActorPath(ActorPath &&other):
        path(std::move(other.path)),
        cur_tile_from(other.cur_tile_from),
        cur_tile_delta(other.cur_tile_delta),
        done(other.done)
    {}

    ActorPath &operator=(const ActorPath &other) {
        if (this == &other) {
            return *this;
        }

        path = other.path;
        cur_tile_from = other.cur_tile_from;
        cur_tile_delta = other.cur_tile_delta;
        done = other.done;

        return *this;
    }

    const std::vector<std::pair<uint32_t, uint32_t>> &get_path() const {
        return path;
    }

    void update_pos(
        // Fractional seconds.
        const double &walk_speed_factor,
        // Fractional seconds.
        const double &time_delta,
        const Map &map
    ) {
        if (done) {
            return;
        }

        const std::vector<MapNode> &nodes = map.get_nodes();

        const auto &[x_from, y_from] = path[cur_tile_from];
        const uint32_t idx_from = get_node_index(x_from, y_from, map.width);
        const double weight_from {nodes[idx_from].get_weight()};

        // Ensure that the total distance travelled is impacted by the tile
        // weight.
        double tile_weight {1.0};

        // Ensure that the total distance travelled is impacted by the
        // angle/distance of the movement.
        double dist {1.0};

        if (cur_tile_from + 1 < path.size()) [[likely]] {
            const auto &[x_to, y_to] = path[cur_tile_from + 1];

            const uint32_t idx_to = get_node_index(x_to, y_to, map.width);

            const double weight_to {nodes[idx_to].get_weight()};

            tile_weight = (weight_from + weight_to) / 2.0;

            dist = dist_euclidean(x_from, y_from, x_to, y_to);
        }
        else [[unlikely]] {
            tile_weight = weight_from;
        }

        const double delta_delta = time_delta * walk_speed_factor * (1.0 / tile_weight) * (1.0 / dist);
        cur_tile_delta += delta_delta;

        std::cout << "Delta delta: [" << delta_delta << "]" << std::endl;

        while (cur_tile_delta >= 1.0) {
            cur_tile_delta -= 1.0;
            cur_tile_from++;
        }

        if (cur_tile_from >= path.size() - 1) {
            done = true;

            cur_tile_from = path.size() - 1;
            cur_tile_delta = 0.0;
        }
    }

    PathDelta get_path_delta() const {
        std::cout << "done: [" << std::boolalpha << done << std::noboolalpha << "]" << std::endl;
        std::cout << "cur_tile_from: [" << cur_tile_from << "]" << std::endl;
        std::cout << "cur_tile_delta: [" << cur_tile_delta << "]" << std::endl;
        std::cout << "path.size(): [" << path.size() << "]" << std::endl;

        if (done) {
            PathDelta(
                path[path.size() - 1],
                path[path.size() - 1],
                0.0
            );
        }

        return PathDelta(
            path[cur_tile_from],
            path[cur_tile_from + 1],
            cur_tile_delta
        );
    }

    bool is_done() const {
        return done;
    }
};

class Actor {
public:
    E_Sprite sprite;
    double walk_speed_factor;

public:
    Actor(const E_Sprite sprite, const double walk_speed_factor):
        sprite(sprite),
        walk_speed_factor(walk_speed_factor)
    {}

    // TODO: entt seems to need move constructor, operator=, and does not like
    // construction via move of `path` arg or even reference to other path.
    Actor(Actor &&other):
        sprite(other.sprite),
        walk_speed_factor(other.walk_speed_factor)
    {}

    Actor &operator=(const Actor &other) {
        if (this == &other) {
            return *this;
        }

        sprite = other.sprite;
        walk_speed_factor = other.walk_speed_factor;

        return *this;
    }
};

#endif
