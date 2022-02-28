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

    const std::vector<node_t> &get_nodes() const {
        return nodes;
    }
};

// node_t represents a single node in the pathfinding graph. These are acquired
// from interactions with map_t.
//
// Type map_t must define typedef node_t.
//
// Type map_t must implement member methods with signatures:
//
// std::vector<typename map_t::node_t> next_nodes(const node_t &cur)
template <typename map_t, typename Predicate>
class Pathfind {
private:
    struct ExploredNode {
        const uint32_t idx;
        const double dist_from_start;
        const double heur_dist_to_end;

        const std::optional<std::reference_wrapper<const ExploredNode>> parent;

        ExploredNode(
            const uint32_t idx,
            const double dist_from_start,
            const double heur_dist_to_end,
            const std::optional<std::reference_wrapper<const ExploredNode>> parent
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
    const Predicate &is_accessible;

    std::vector<ExploredNode> seen_nodes;
    std::unordered_set<uint32_t> seen_nodes_idx;

    std::vector<std::reference_wrapper<const ExploredNode>> to_explore;

    // Push a candidate neighbor node to the queue of nodes to explore next.
    //
    // NOTE: The `parent` argument is moved from.
    void push_node(
        const uint32_t idx,
        const std::optional<std::reference_wrapper<const ExploredNode>> &&parent
    ) {
        const auto [iter, inserted] = seen_nodes_idx.insert(idx);

        if (inserted) {
            const auto [x_new, y_new] {
                get_node_xy(idx, map.width)
            };

            const double heur_dist_to_end = dist_chebyshev(
                x_new, y_new, x_end, y_end
            );

            if (parent) [[likely]] {
                const ExploredNode &prev = *parent;

                const auto [x_prev, y_prev] {
                    get_node_xy(prev.idx, map.width)
                };

                const double dist_between = dist_euclidean(
                    x_prev, y_prev, x_new, y_new
                );

                to_explore.emplace_back(
                    seen_nodes.emplace_back(
                        idx,
                        prev.dist_from_start + dist_between,
                        heur_dist_to_end,
                        std::move(parent)
                    )
                );

                std::push_heap(
                    to_explore.begin(), to_explore.end(),
                    std::greater<ExploredNode>{}
                );
            }
            else [[unlikely]] {
                to_explore.emplace_back(
                    seen_nodes.emplace_back(
                        idx,
                        0,
                        heur_dist_to_end,
                        std::nullopt
                    )
                );

                std::push_heap(
                    to_explore.begin(), to_explore.end(),
                    std::greater<ExploredNode>{}
                );
            }
        }

        return;
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

    void gen_neighbors() {
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
                if (is_accessible(map.get_nodes()[idx_neighbor_candidate])) {
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

                    push_node(idx_neighbor_candidate, cur_node);
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
        const Predicate &is_accessible
    ):
        map(map),
        x_start(x_start),
        y_start(y_start),
        x_end(x_end),
        y_end(y_end),
        is_accessible(is_accessible)
    {
        to_explore.reserve(map.width * map.height);
        seen_nodes_idx.reserve(map.width * map.height);
        seen_nodes.reserve(map.width * map.height);
    }

    std::vector<std::pair<uint32_t, uint32_t>> get_path() {
        if (x_start == x_end && y_start == y_end) {
            return {};
        }

        const std::vector<typename map_t::node_t> &nodes {map.get_nodes()};
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

        push_node(idx_node_start, std::nullopt);

        while (to_explore.size() > 0) {
            const ExploredNode &best_node {get_best_node()};

            const auto &[x_best, y_best] = get_node_xy(
                best_node.idx, map.width
            );

            if (x_best == x_end && y_best == y_end) {
                break;
            }

            gen_neighbors();
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
