#ifndef MAP_H
#define MAP_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <list>
#include <mutex>
#include <optional>
#include <random>
#include <unordered_set>
#include <vector>

#include <assert.h>

#include "Util.h"

struct Pos {
    float x;
    float y;
};

struct Pather {};

class Map;

struct MapNode {
public:
    static inline const float DEFAULT_WEIGHT {1.3};

    const uint32_t x_coord;
    const uint32_t y_coord;

private:
    bool blocking;
    std::optional<uint64_t> region;
    float weight;

public:
    MapNode(
        const uint32_t x_coord,
        const uint32_t y_coord,
        const bool blocking,
        const std::optional<uint64_t> region = std::nullopt,
        const float weight = DEFAULT_WEIGHT
    ):
        x_coord(x_coord),
        y_coord(y_coord),
        blocking(blocking),
        region(region),
        weight(weight)
    {}

    bool get_blocking() const {
        return blocking;
    }

    void set_blocking(const bool blocking_new) {
        blocking = blocking_new;
    }

    float get_weight() const {
        return weight;
    }

    void set_weight(const float weight_new) {
        weight = weight_new;
    }

    const std::optional<uint64_t> &get_region() const {
        return region;
    }

    void set_region(std::optional<uint64_t> &&region_new) {
        region = std::move(region_new);
    }

    friend Map;
};

class Map {
public:
    typedef MapNode node_t;

private:
    static inline std::random_device rd;
    static inline std::mt19937 gen {rd()};

    std::vector<node_t> nodes;

public:
    // The x-coordinate range.
    const uint32_t width {64};
    // The y-coordinate range.
    const uint32_t height {32};

    Map(const uint32_t width = 64, const uint32_t height = 32):
        width(width),
        height(height)
    {
        gen.seed(2);
    }

    Map(Map &&other) noexcept:
        nodes(std::move(other.nodes)),
        width(other.width),
        height(other.height)
    {}

    static Map gen_rand_map(
        const uint32_t width = 64, const uint32_t height = 32
    ) {
        std::uniform_int_distribution<> rng(0, 100);
        std::uniform_int_distribution<> rng_width(0, width);
        std::uniform_int_distribution<> rng_height(0, height);

        Map map(width, height);

        map.nodes.reserve(map.width * map.height);

        for (uint32_t i = 0; i < map.nodes.capacity(); ++i) {
            const auto [x, y] = get_node_xy(i, map.width);

            map.nodes.emplace_back(x, y, rng(Map::gen) > 65);
        }

        for (uint32_t i = 0; i < 10; ++i) {
            uint32_t x_road = rng_width(Map::gen);
            uint32_t x_extend = rng_width(Map::gen);

            if (x_road > x_extend) {
                std::swap(x_road, x_extend);
            }

            uint32_t y_road = rng_height(Map::gen);
            uint32_t y_extend = rng_height(Map::gen);

            if (y_road > y_extend) {
                std::swap(y_road, y_extend);
            }

            for (uint32_t i_x = x_road; i_x <= x_extend; ++i_x) {
                const auto idx = get_node_index(i_x, y_road, map.width);

                map.nodes[idx].set_weight(0.7);
                map.nodes[idx].set_blocking(false);
            }
            for (uint32_t i_y = y_road; i_y <= y_extend; ++i_y) {
                const auto idx = get_node_index(x_road, i_y, map.width);

                map.nodes[idx].set_weight(0.7);
                map.nodes[idx].set_blocking(false);
            }
            for (uint32_t i_x = x_road; i_x <= x_extend; ++i_x) {
                const auto idx = get_node_index(i_x, y_extend, map.width);

                map.nodes[idx].set_weight(0.7);
                map.nodes[idx].set_blocking(false);
            }
            for (uint32_t i_y = y_road; i_y <= y_extend; ++i_y) {
                const auto idx = get_node_index(x_extend, i_y, map.width);

                map.nodes[idx].set_weight(0.7);
                map.nodes[idx].set_blocking(false);
            }
        }

        return map;
    }

    void draw_map_to_frame(std::vector<char> &tiles) const {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const uint32_t i {get_node_index(x, y, width)};
                if (nodes[i].get_blocking()) {
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
        } while (!nodes[i].get_blocking());

        return {x, y};
    }

    bool is_blocking(const uint32_t x, const uint32_t y) const {
        return nodes[get_node_index(x, y, width)].get_blocking();
    }

    const std::vector<node_t> &get_nodes() const {
        return nodes;
    }

    std::vector<node_t> &get_nodes_mut() {
        return nodes;
    }

    void clear_regions() {
        for (auto &node : nodes) {
            node.set_region(std::nullopt);
        }
    }
};

#endif
