#ifndef MAP_H
#define MAP_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <list>
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

class MapNode {
public:
    const uint32_t x_coord;
    const uint32_t y_coord;
    const bool blocking;

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
    {
        gen.seed(2);
    }

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

            map.nodes.emplace_back(x, y, rng(Map::gen) > 3);
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
    struct ExploredNode {
        uint32_t idx;
        uint32_t dist_from_start;
        uint32_t heur_dist_to_end;

        std::optional<std::reference_wrapper<const ExploredNode>> parent;

        ExploredNode(
            const uint32_t idx,
            const uint32_t dist_from_start,
            const uint32_t heur_dist_to_end,
            std::optional<std::reference_wrapper<const ExploredNode>> parent
        ):
            idx(idx),
            dist_from_start(dist_from_start),
            heur_dist_to_end(heur_dist_to_end),
            parent(parent)
        {}

        friend bool operator>(
            const ExploredNode &lhs, const ExploredNode &rhs
        ){
            return (
                lhs.dist_from_start + lhs.heur_dist_to_end
            ) > (
                rhs.dist_from_start + rhs.heur_dist_to_end
            );
        }
    };

    const map_t &map;
    const uint32_t x_start;
    const uint32_t y_start;
    const uint32_t x_end;
    const uint32_t y_end;
    const std::function<bool(const node_t&)> pred_accessible;

    std::unordered_set<uint32_t> explored_nodes;
    std::vector<ExploredNode> all_explored_nodes;

    std::vector<std::reference_wrapper<const ExploredNode>> to_explore;

    void push_node(
        const uint32_t idx,
        const uint32_t dist_from_start,
        const uint32_t heur_dist_to_end,
        const std::optional<std::reference_wrapper<const ExploredNode>> parent
    ) {
        const auto [iter, inserted] = explored_nodes.insert(idx);

        if (inserted) {
            to_explore.emplace_back(
                all_explored_nodes.emplace_back(
                    idx, dist_from_start, heur_dist_to_end, parent
                )
            );

            std::push_heap(
                to_explore.begin(), to_explore.end(),
                std::greater<ExploredNode>{}
            );
        }
    }

    void pop_node() {
        assert(to_explore.size() > 0);

        std::pop_heap(
            to_explore.begin(), to_explore.end(),
            std::greater<ExploredNode>{}
        );

        to_explore.pop_back();
    }

    const ExploredNode &get_best_node() const {
        assert(to_explore.size() > 0);

        return to_explore.at(0);
    }

    uint32_t heuristic_to_end(const uint32_t x, const uint32_t y) const {
        return (
            (
                (x > x_end)
                    ? x - x_end
                    : x_end - x
            ) +
            (
                (y > y_end)
                    ? y - y_end
                    : y_end - y
            )
        );
    }

    void calc_neighbors() {
        const ExploredNode &cur_node {get_best_node()};
        pop_node();

        const auto [x_node, y_node] = get_node_xy(cur_node.idx, map.width);

        for (int32_t d_y {-1}; d_y <= 1; ++d_y) {
            const int32_t y_neighbor {static_cast<int32_t>(y_node) + d_y};

            // Skip any out-of-bounds Y-coordinates.
            if (
                y_neighbor < 0 ||
                static_cast<uint32_t>(y_neighbor) >= map.height
            ) {
                continue;
            }

            for (int32_t d_x {-1}; d_x <= 1; ++d_x) {
                // Skip the start node.
                if (d_y == 0 && d_x == 0) {
                    continue;
                }

                const int32_t x_neighbor {static_cast<int32_t>(x_node) + d_x};

                // Skip any out-of-bounds X-coordinates.
                if (
                    x_neighbor < 0 ||
                    static_cast<uint32_t>(x_neighbor) >= map.width
                ) {
                    continue;
                }

                const uint32_t idx_neighbor_candidate {
                    get_node_index(x_neighbor, y_neighbor, map.width)
                };

                // First make sure the node is itself non-blocking.
                if (
                    !map.get_nodes()[idx_neighbor_candidate].blocking
                    // pred_accessible(map.get_nodes()[idx_neighbor_candidate])
                ) {
                    // Then make sure that, if this would be a diagonal move, we
                    // disallow it if it would be intersecting with an adjacent
                    // blocking node. That is, if the dots are open and the X's
                    // are blocking nodes, then the center node can only validly
                    // move straight down; the other corners are "blocked" by
                    // the adjacent obstacles:
                    //
                    // . X .
                    // X . X
                    // . . .
                    //

                    if (
                        (d_x == -1 && d_y == -1) ||
                        (d_x == -1 && d_y ==  1) ||
                        (d_x ==  1 && d_y ==  1) ||
                        (d_x ==  1 && d_y == -1)
                    ) {
                        // NOTE: Since we know that the middle node is valid
                        // (since we're expanding it now), and since we know
                        // that this d_x/d_y combination is valid (since we got
                        // past the checks above), then we know that these
                        // adjacent nodes are also guaranteed-accessible.
                        {
                            const uint32_t x_neigh_adj {x_node + d_x};
                            const uint32_t y_neigh_adj {y_node};

                            const uint32_t idx_neigh_adj {
                                get_node_index(
                                    x_neigh_adj, y_neigh_adj, map.width
                                )
                            };

                            if (map.get_nodes()[idx_neigh_adj].blocking) {
                                continue;
                            }
                        }
                        {
                            const uint32_t x_neigh_adj {x_node};
                            const uint32_t y_neigh_adj {y_node + d_y};

                            const uint32_t idx_neigh_adj {
                                get_node_index(
                                    x_neigh_adj, y_neigh_adj, map.width
                                )
                            };

                            if (map.get_nodes()[idx_neigh_adj].blocking) {
                                continue;
                            }
                        }
                    }

                    push_node(
                        idx_neighbor_candidate,
                        cur_node.dist_from_start + 1,
                        heuristic_to_end(x_neighbor, y_neighbor),
                        cur_node
                    );
                }
            }
        }

        return;
    }

public:
    Pathfind(
        const map_t &map,
        const uint32_t x_start,
        const uint32_t y_start,
        const uint32_t x_end,
        const uint32_t y_end,
        const std::function<bool(const node_t&)> pred_accessible
    ):
        map(map),
        x_start(x_start),
        y_start(y_start),
        x_end(x_end),
        y_end(y_end),
        pred_accessible(pred_accessible)
    {
        to_explore.reserve(map.width * map.height);
        explored_nodes.reserve(map.width * map.height);
        all_explored_nodes.reserve(map.width * map.height);
    }

    std::vector<std::pair<uint32_t, uint32_t>> get_path() {
        if (x_start == x_end && y_start == y_end) {
            return {};
        }

        const std::vector<node_t> &nodes {map.get_nodes()};
        const uint32_t map_width {map.width};

        const uint32_t idx_node_start {
            get_node_index(x_start, y_start, map_width)
        };
        const uint32_t idx_node_end {
            get_node_index(x_end, y_end, map_width)
        };

        if (
            nodes[idx_node_start].blocking ||
            nodes[idx_node_end].blocking
        ) {
            return {};
        }

        push_node(
            idx_node_start, 0, heuristic_to_end(x_start, x_end), std::nullopt
        );

        while (to_explore.size() > 0) {
            const ExploredNode &best_node {get_best_node()};

            const auto &[x_best, y_best] = get_node_xy(
                best_node.idx, map.width
            );

            if (x_best == x_end && y_best == y_end) {
                break;
            }

            calc_neighbors();
        }

        if (to_explore.size() == 0) {
            return {};
        }

        std::optional<std::reference_wrapper<const ExploredNode>> path_node {
            get_best_node()
        };

        std::vector<std::pair<uint32_t, uint32_t>> path;

        path.push_back(get_node_xy(path_node->get().idx, map.width));

        while (path_node->get().parent) {
            path.push_back(
                get_node_xy(path_node->get().parent->get().idx, map.width)
            );

            path_node = path_node->get().parent;
        }

        return path;
    }
};

#endif
