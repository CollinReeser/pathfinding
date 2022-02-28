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
    const uint32_t x_coord;
    const uint32_t y_coord;

private:
    bool blocking;
    std::optional<uint64_t> region;

public:
    MapNode(
        const uint32_t x_coord,
        const uint32_t y_coord,
        const bool blocking,
        const std::optional<uint64_t> region = std::nullopt
    ):
        x_coord(x_coord),
        y_coord(y_coord),
        blocking(blocking),
        region(region)
    {}

    bool get_blocking() const {
        return blocking;
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
};

template <typename T, typename U, template <typename, typename> class Derived>
class MapExplorer {
protected:
    void gen_neighbors() {
        const typename Derived<T, U>::ExploredNode &cur_node {
            static_cast<Derived<T, U>*>(this)->get_next_node()
        };

        static_cast<Derived<T, U>*>(this)->pop_node();

        const auto [x_node, y_node] = get_node_xy(
            cur_node.idx, static_cast<Derived<T, U>*>(this)->get_map_width()
        );

        for (int32_t d_y {-1}; d_y <= 1; ++d_y) {
            const int32_t y_neighbor {static_cast<int32_t>(y_node) + d_y};

            // Skip any out-of-bounds Y-coordinates.
            if (
                y_neighbor < 0 ||
                static_cast<uint32_t>(
                    y_neighbor
                ) >= static_cast<Derived<T, U>*>(this)->get_map_height()
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
                    static_cast<uint32_t>(
                        x_neighbor
                    ) >= static_cast<Derived<T, U>*>(this)->get_map_width()
                ) {
                    continue;
                }

                const uint32_t idx_neighbor_candidate {
                    get_node_index(
                        x_neighbor, y_neighbor,
                        static_cast<Derived<T, U>*>(this)->get_map_width()
                    )
                };

                // First make sure the node is itself non-blocking.
                if (
                    static_cast<Derived<T, U>*>(this)->is_accessible(
                        static_cast<Derived<T, U>*>(this)->get_map_nodes()[
                            idx_neighbor_candidate
                        ]
                    )
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
                                    x_neigh_adj, y_neigh_adj,
                                    static_cast<Derived<T, U>*>(this)->get_map_width()
                                )
                            };

                            if (
                                !static_cast<Derived<T, U>*>(this)->is_accessible(
                                    static_cast<Derived<T, U>*>(this)->get_map_nodes(
                                    )[idx_neigh_adj]
                                )
                            ) {
                                continue;
                            }
                        }
                        {
                            const uint32_t x_neigh_adj {x_node};
                            const uint32_t y_neigh_adj {y_node + d_y};

                            const uint32_t idx_neigh_adj {
                                get_node_index(
                                    x_neigh_adj, y_neigh_adj,
                                    static_cast<Derived<T, U>*>(this)->get_map_width()
                                )
                            };

                            if (
                                !static_cast<Derived<T, U>*>(this)->is_accessible(
                                    static_cast<Derived<T, U>*>(this)->get_map_nodes(
                                    )[idx_neigh_adj]
                                )
                            ) {
                                continue;
                            }
                        }
                    }

                    static_cast<Derived<T, U>*>(this)->push_node(
                        idx_neighbor_candidate, cur_node
                    );
                }
            }
        }

        return;
    }
};

template <typename map_t, typename Predicate>
class RegionColorer : public MapExplorer<map_t, Predicate, RegionColorer> {
private:
    typedef typename map_t::node_t map_node_t;

    static inline std::mutex mu;
    static inline uint64_t region_color {0};

    static uint64_t get_next_region_color() {
        std::scoped_lock<std::mutex> lock(mu);

        return region_color++;
    }

public:
    struct ExploredNode {
        const uint32_t idx;

        ExploredNode(
            const uint32_t idx
        ):
            idx(idx)
        {}
    };

    map_t &map;
    const uint32_t x_start;
    const uint32_t y_start;

    const Predicate &is_accessible;

    uint32_t idx_unexplored {0};

    std::vector<ExploredNode> seen_nodes;
    std::unordered_set<uint32_t> seen_nodes_idx;

    void push_node(
        const uint32_t idx,
        const std::optional<std::reference_wrapper<const ExploredNode>> &&parent
    ) {
        const auto [iter, inserted] = seen_nodes_idx.insert(idx);

        if (inserted) {
            seen_nodes.emplace_back(idx);
        }

        return;
    }

    const ExploredNode &get_next_node() const {
        assert(idx_unexplored < seen_nodes.size());

        return seen_nodes.at(idx_unexplored);
    }

    void pop_node() {
        assert(idx_unexplored < seen_nodes.size());

        ++idx_unexplored;

        return;
    }

    const std::vector<typename map_t::node_t> &get_map_nodes() const {
        return map.get_nodes();
    }

    uint32_t get_map_width() const {
        return map.width;
    }

    uint32_t get_map_height() const {
        return map.height;
    }

public:
    RegionColorer(
        map_t &map,
        const uint32_t x_start,
        const uint32_t y_start,
        const Predicate &is_accessible
    ):
        map(map),
        x_start(x_start),
        y_start(y_start),
        is_accessible(is_accessible)
    {
        seen_nodes.reserve(map.width * map.height);
        seen_nodes_idx.reserve(map.width * map.height);
    }

    std::optional<uint64_t> identify_region() {
        const uint32_t idx_node_start {
            get_node_index(x_start, y_start, get_map_width())
        };

        if (
            !is_accessible(get_map_nodes()[idx_node_start])
        ) {
            return std::nullopt;
        }
        else if (get_map_nodes()[idx_node_start].get_region()) {
            return get_map_nodes()[idx_node_start].get_region();
        }

        push_node(idx_node_start, std::nullopt);

        while (idx_unexplored < seen_nodes.size()) {
            this->gen_neighbors();
        }

        const uint64_t region_color = get_next_region_color();

        auto &map_nodes = map.get_nodes_mut();

        for (const auto &node : seen_nodes) {
            map_nodes[node.idx].set_region(region_color);
        }

        return region_color;
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
            const std::optional<
                std::reference_wrapper<const ExploredNode>
            > parent
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

    static inline uint32_t count_push_node {0};
    static inline uint32_t count_novel_nodes {0};
    static inline uint32_t path_length {0};

    map_t &map;
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

    const ExploredNode &get_next_node() const {
        assert(to_explore.size() > 0);

        return to_explore.at(0);
    }

    const std::vector<typename map_t::node_t> &get_map_nodes() const {
        return map.get_nodes();
    }

    uint32_t get_map_width() const {
        return map.width;
    }

    uint32_t get_map_height() const {
        return map.height;
    }

    void gen_neighbors() {
        const ExploredNode &cur_node {get_next_node()};
        pop_node();

        const auto [x_node, y_node] = get_node_xy(
            cur_node.idx, get_map_width()
        );

        for (int32_t d_y {-1}; d_y <= 1; ++d_y) {
            const int32_t y_neighbor {static_cast<int32_t>(y_node) + d_y};

            // Skip any out-of-bounds Y-coordinates.
            if (
                y_neighbor < 0 ||
                static_cast<uint32_t>(y_neighbor) >= get_map_height()
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
                    static_cast<uint32_t>(x_neighbor) >= get_map_width()
                ) {
                    continue;
                }

                const uint32_t idx_neighbor_candidate {
                    get_node_index(x_neighbor, y_neighbor, get_map_width())
                };

                // First make sure the node is itself non-blocking.
                if (is_accessible(get_map_nodes()[idx_neighbor_candidate])) {
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
                                    x_neigh_adj, y_neigh_adj, get_map_width()
                                )
                            };

                            if (
                                !is_accessible(get_map_nodes()[idx_neigh_adj])
                            ) {
                                continue;
                            }
                        }
                        {
                            const uint32_t x_neigh_adj {x_node};
                            const uint32_t y_neigh_adj {y_node + d_y};

                            const uint32_t idx_neigh_adj {
                                get_node_index(
                                    x_neigh_adj, y_neigh_adj, get_map_width()
                                )
                            };

                            if (
                                !is_accessible(get_map_nodes()[idx_neigh_adj])
                            ) {
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
        map_t &map,
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
            !is_accessible(nodes[idx_node_start]) ||
            !is_accessible(nodes[idx_node_end])
        ) {
            return {};
        }

        // Are the nodes in separate regions and thus inaccessible to each
        // other?

        std::optional<uint64_t> region_start;
        std::optional<uint64_t> region_end;

        if (map.get_nodes()[idx_node_start].get_region()) {
            region_start = map.get_nodes()[idx_node_start].get_region();
        }
        else {
            RegionColorer<map_t, Predicate> region_colorer(
                map, x_start, y_start, is_accessible
            );

            region_start = region_colorer.identify_region();
        }

        if (map.get_nodes()[idx_node_end].get_region()) {
            region_end = map.get_nodes()[idx_node_end].get_region();
        }
        else {
            RegionColorer<map_t, Predicate> region_colorer(
                map, x_end, y_end, is_accessible
            );

            region_end = region_colorer.identify_region();
        }

        if (
            !region_start || !region_end || *region_start != *region_end
        ) {
            return {};
        }

        push_node(idx_node_start, std::nullopt);

        while (to_explore.size() > 0) {
            const ExploredNode &best_node {get_next_node()};

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
            get_next_node()
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
