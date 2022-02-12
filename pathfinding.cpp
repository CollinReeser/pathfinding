#include <iostream>
#include <random>
#include <vector>

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
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> rng(0, 5);

        Map map {};

        map.nodes.reserve(map.width * map.height);

        for (uint32_t i = 0; i < map.nodes.capacity(); ++i) {
            const auto [x, y] = map.get_node_xy(i);

            map.nodes.emplace_back(x, y, rng(gen) > 4);
        }

        return map;
    }

    void print_map() {
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
};

int main(int argc, char** argv) {
    std::cout << "Hello, world!" << std::endl;

    Pathfind<Map, MapNode> pathfind {};

    Map map {Map::gen_rand_map()};

    map.print_map();

    return 0;
}