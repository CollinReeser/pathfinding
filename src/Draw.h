#ifndef DRAW_H
#define DRAW_H

#include <vector>

#include "Map.h"
#include "Util.h"

#include <entt/entt.hpp>

struct Frame {
private:
    const uint32_t width;
    const uint32_t height;

    std::vector<char> tiles;

public:
    Frame(
        const Map &map, const entt::registry &registry,
        const uint32_t view_width = 128, const uint32_t view_height = 128
    ):
        width(map.width),
        height(map.height)
    {
        tiles.reserve(width * height);

        map.draw_map_to_frame(tiles);

        const auto view {registry.view<const Pos>()};

        for (const auto &[entity, pos]: view.each()) {
            const uint32_t i = get_node_index(
                (uint32_t)pos.x, (uint32_t)pos.y, width
            );

            tiles[i] = 'O';
        }
    }

    void draw_frame() {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                std::cout << tiles[get_node_index(x, y, width)];
            }

            std::cout << std::endl;
        }
    }
};

#endif
