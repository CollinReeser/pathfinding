#ifndef MAP_H
#define MAP_H

#include <iostream>
#include <random>
#include <vector>

#include "Util.h"

struct Pos {
    float x;
    float y;
};

struct Pather {};

class Map;

class MapNode {
public:
    const uint32_t x_coord;
    const uint32_t y_coord;
    const bool blocking;

private:

public:
    MapNode(
        const uint32_t x_coord,
        const uint32_t y_coord,
        const bool blocking
    ):
        x_coord(x_coord),
        y_coord(y_coord),
        blocking(blocking)
    {}

    friend Map;
};

class Map {
private:
    static inline std::random_device rd;
    static inline std::mt19937 gen {rd()};

    std::vector<MapNode> nodes;

public:
    // The x-coordinate range.
    const uint32_t width {64};
    // The y-coordinate range.
    const uint32_t height {32};

    Map(const uint32_t width = 64, const uint32_t height = 32):
        width(width),
        height(height)
    {}

    Map(Map&& o) noexcept:
        nodes(std::move(o.nodes)),
        width(o.width),
        height(o.height)
    {}

    static Map gen_rand_map(
        const uint32_t width = 64, const uint32_t height = 32
    ) {
        std::uniform_int_distribution<> rng(0, 5);

        Map map(width, height);

        map.nodes.reserve(map.width * map.height);

        for (uint32_t i = 0; i < map.nodes.capacity(); ++i) {
            const auto [x, y] = get_node_xy(i, map.width);

            map.nodes.emplace_back(x, y, rng(Map::gen) > 4);
        }

        return map;
    }

    void draw_map_to_frame(std::vector<char> &tiles) const {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const uint32_t i {get_node_index(x, y, width)};
                if (nodes[i].blocking) {
                    tiles[i] = 'X';
                }
                else {
                    tiles[i] = '.';
                }
            }
        }
    }

    std::pair<uint32_t, uint32_t> get_rand_open_xy() const {
        std::uniform_int_distribution<> rng_w(0, width - 1);
        std::uniform_int_distribution<> rng_h(0, height - 1);

        uint32_t x;
        uint32_t y;
        uint32_t i;

        do {
            x = rng_w(Map::gen);
            y = rng_h(Map::gen);

            i = get_node_index(x, y, width);
        } while (!nodes[i].blocking);

        return {x, y};
    }

    bool is_blocking(const uint32_t x, const uint32_t y) const {
        return nodes[get_node_index(x, y, width)].blocking;
    }

    const std::vector<MapNode> &get_nodes() const {
        return nodes;
    }
};

// node_t represents a single node in the pathfinding graph. These are acquired
// from interactions with map_t.
//
// Type map_t must implement member methods with signatures:
//
// std::vector<node_t> next_nodes(const node_t &cur)
template <typename map_t, typename node_t>
class Pathfind {
private:
public:
    Pathfind() {}
};

#endif
