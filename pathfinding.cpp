#include <iostream>
#include <random>
#include <vector>

#include <entt/entt.hpp>

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

class Map;

class MapNode {
private:
    const uint32_t x_coord;
    const uint32_t y_coord;

    bool blocking;

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

    // The x-coordinate range.
    const uint32_t width {64};
    // The y-coordinate range.
    const uint32_t height {32};

    std::vector<MapNode> nodes;

public:
    Map(const uint32_t width = 64, const uint32_t height = 32):
        width(width),
        height(height)
    {}

    Map(Map&& o) noexcept:
        width(o.width),
        height(o.height),
        nodes(std::move(o.nodes))
    {}

    uint32_t get_node_index(const uint32_t x, uint32_t y) const {
        return y * width + x;
    }

    std::pair<uint32_t, uint32_t> get_node_xy(const uint32_t i) const {
        const uint32_t y = i / width;
        const uint32_t x = i - (y * width);

        return {x, y};
    }

    static Map gen_rand_map() {
        std::uniform_int_distribution<> rng(0, 5);

        Map map {};

        map.nodes.reserve(map.width * map.height);

        for (uint32_t i = 0; i < map.nodes.capacity(); ++i) {
            const auto [x, y] = map.get_node_xy(i);

            map.nodes.emplace_back(x, y, rng(Map::gen) > 4);
        }

        return map;
    }

    void debug_print_map() {
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                if (nodes[get_node_index(x, y)].blocking) {
                    std::cout << "X";
                }
                else {
                    std::cout << ".";
                }
            }

            std::cout << std::endl;
        }
    }

    std::pair<uint32_t, uint32_t> get_rand_open_xy() {
        std::uniform_int_distribution<> rng_w(0, width);
        std::uniform_int_distribution<> rng_h(0, height);

        uint32_t x;
        uint32_t y;
        uint32_t i;

        do {
            x = rng_w(Map::gen);
            y = rng_h(Map::gen);

            i = get_node_index(x, y);
        } while (!nodes[i].blocking);

        return {x, y};
    }
};

struct Pos {
    float x;
    float y;
};

struct Mover {};

int main(int argc, char** argv) {
    entt::registry registry;

    std::cout << "Hello, world!" << std::endl;

    Pathfind<Map, MapNode> pathfind {};

    Map map {Map::gen_rand_map()};

    for (uint32_t i = 0; i < 100; ++i) {
        const auto [x, y] = map.get_rand_open_xy();

        const auto entity = registry.create();
        registry.emplace<Pos>(entity, (float)x + 0.5f, (float)y + 0.5f);
    }

    map.debug_print_map();

    return 0;
}