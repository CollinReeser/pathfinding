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

template <typename T, typename U, template <typename, typename> class Derived>
class MapExplorer {
protected:
    void gen_neighbors() {
        auto deriv_ptr {static_cast<Derived<T, U>*>(this)};

        const typename Derived<T, U>::ExploredNode &cur_node {
            deriv_ptr->get_next_node()
        };

        deriv_ptr->pop_node();

        const auto [x_node, y_node] = get_node_xy(
            cur_node.idx, deriv_ptr->get_map_width()
        );

        for (int32_t d_y {-1}; d_y <= 1; ++d_y) {
            const int32_t y_neighbor {static_cast<int32_t>(y_node) + d_y};

            // Skip any out-of-bounds Y-coordinates.
            if (
                y_neighbor < 0 ||
                static_cast<uint32_t>(
                    y_neighbor
                ) >= deriv_ptr->get_map_height()
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
                    ) >= deriv_ptr->get_map_width()
                ) {
                    continue;
                }

                const uint32_t idx_neighbor_candidate {
                    get_node_index(
                        x_neighbor, y_neighbor,
                        deriv_ptr->get_map_width()
                    )
                };

                // First make sure the node is itself non-blocking.
                if (
                    deriv_ptr->is_accessible(
                        deriv_ptr->get_map_nodes()[idx_neighbor_candidate]
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
                                    deriv_ptr->get_map_width()
                                )
                            };

                            if (
                                !deriv_ptr->is_accessible(
                                    deriv_ptr->get_map_nodes()[idx_neigh_adj]
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
                                    deriv_ptr->get_map_width()
                                )
                            };

                            if (
                                !deriv_ptr->is_accessible(
                                    deriv_ptr->get_map_nodes()[idx_neigh_adj]
                                )
                            ) {
                                continue;
                            }
                        }
                    }

                    deriv_ptr->push_node(idx_neighbor_candidate, cur_node);
                }
            }
        }

        return;
    }
};

// Given a start node, "color" (a la graph coloring) the region of nodes
// accessible from the start node. This will modify the relevant properties of
// the accessible nodes in the given map.
//
// Note that no effort is mmade to detect "mistakes" in a node's region
// assignment. If it is believed that the map has changed in any way, the region
// assignment of _all_ nodes should be cleared prior to invoking this class on
// any start node, as a previously-contiguous region may now be split, and/or a
// previously-split region may not be contiguous.
template <typename map_t, typename Predicate>
class RegionColorer : public MapExplorer<map_t, Predicate, RegionColorer> {
public:
    struct ExploredNode {
        const uint32_t idx;

        ExploredNode(
            const uint32_t idx
        ):
            idx(idx)
        {}
    };

private:
    static inline std::mutex mu;
    static inline uint64_t region_color {0};

    // Returns a guaranteed-unique value every time it is called.
    static uint64_t get_next_region_color() {
        std::scoped_lock<std::mutex> lock(mu);

        return ++region_color;
    }

    map_t &map;
    const uint32_t x_start;
    const uint32_t y_start;

    uint32_t idx_unexplored {0};

    std::vector<ExploredNode> seen_nodes;
    std::unordered_set<uint32_t> seen_nodes_idx;

public:
    static uint64_t get_cur_region_color() {
        std::scoped_lock<std::mutex> lock(mu);

        return region_color;
    }

    const Predicate &is_accessible;


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

    // Return the existing region assignment of the start node if it exists, or
    // perform an accessibility crawl, color the start node and all accessible
    // nodes with a new unique color, and then return that color as the new
    // region assignment.
    std::optional<uint64_t> identify_region() {
        auto start_ident = std::chrono::steady_clock::now();

        const uint32_t idx_node_start {
            get_node_index(x_start, y_start, get_map_width())
        };

        // If the start node is inaccessible (and thus does not have a
        // meaningful region), then there's nothing to do.
        if (
            !is_accessible(get_map_nodes()[idx_node_start])
        ) {
            return std::nullopt;
        }
        // If the start node already has an assigned region, then we can just
        // return that.
        else if (get_map_nodes()[idx_node_start].get_region()) {
            return get_map_nodes()[idx_node_start].get_region();
        }

        // Explore all accessible nodes from the starting node. This tells us
        // all nodes in this "regionn".

        push_node(idx_node_start, std::nullopt);

        while (idx_unexplored < seen_nodes.size()) {
            this->gen_neighbors();
        }

        // Get a unique color for this new region.
        const uint64_t region_color = get_next_region_color();

        // Inform all nodes in this region of their new region assignment.

        auto &map_nodes = map.get_nodes_mut();

        for (const auto &node : seen_nodes) {
            map_nodes[node.idx].set_region(region_color);
        }

        auto end_ident = std::chrono::steady_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::microseconds>(
            end_ident - start_ident
        );

        std::cout
            << "Identified region [" << region_color << "] at ["
            << x_start << ", " << y_start << "]: [" << dur.count() << "] us" << std::endl;

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
class Pathfind : public MapExplorer<map_t, Predicate, Pathfind> {
public:
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
        ) {
            return (
                lhs.dist_from_start + lhs.heur_dist_to_end
            ) > (
                rhs.dist_from_start + rhs.heur_dist_to_end
            );
        }
    };

private:
    static inline uint32_t count_push_node {0};
    static inline uint32_t count_novel_nodes {0};
    static inline uint32_t path_length {0};

    map_t &map;
    const uint32_t x_start;
    const uint32_t y_start;
    const uint32_t x_end;
    const uint32_t y_end;

    std::vector<ExploredNode> seen_nodes;
    std::unordered_set<uint32_t> seen_nodes_idx;

    std::vector<std::reference_wrapper<const ExploredNode>> to_explore;

public:
    static inline std::unordered_set<uint32_t> static_seen_nodes_idx;

    const Predicate &is_accessible;

    // Push a candidate neighbor node to the queue of nodes to explore next.
    //
    // NOTE: The `parent` argument is moved from.
    void push_node(
        const uint32_t idx,
        const std::optional<std::reference_wrapper<const ExploredNode>> &&parent
    ) {
        ++count_push_node;

        const auto [iter, inserted] = seen_nodes_idx.insert(idx);

        if (inserted) {
            ++count_novel_nodes;

            const auto [x_new, y_new] {
                get_node_xy(idx, map.width)
            };

            // The Euclidean distance to the end will be equal to or greater
            // than the Chebyshev distance to the end, meaning we are
            // overestimating our heuristic. In practice, this reduces the
            // optimality of the path by only a small amount, but greatly
            // reduces the number of nodes explored, achieving much better
            // performance for only a small optimality penalty over long
            // distances.
            const double heur_dist_to_end = dist_euclidean(
                x_new, y_new, x_end, y_end
            );

            if (parent) [[likely]] {
                const ExploredNode &prev = *parent;

                const auto [x_prev, y_prev] {
                    get_node_xy(prev.idx, map.width)
                };

                const double dist_prev_to_new = dist_chebyshev(
                    x_prev, y_prev, x_new, y_new
                );

                const double weight {get_map_nodes()[idx].get_weight()};

                to_explore.emplace_back(
                    seen_nodes.emplace_back(
                        idx,
                        prev.dist_from_start + (dist_prev_to_new * weight),
                        heur_dist_to_end * weight,
                        std::move(parent)
                    )
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
            }

            std::push_heap(
                to_explore.begin(), to_explore.end(),
                std::greater<ExploredNode>{}
            );
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
        count_novel_nodes = 0;
        count_push_node = 0;

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

            this->gen_neighbors();
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

        path_length = path.size();

        {
            static_seen_nodes_idx = std::unordered_set<uint32_t>(
                seen_nodes_idx.begin(), seen_nodes_idx.end()
            );
        }

        return path;
    }

    static void print_perf() {
        std::cout << "count_push_node  : " << count_push_node << std::endl;
        std::cout << "count_novel_nodes: " << count_novel_nodes << std::endl;
        std::cout << "path_length      : " << path_length << std::endl;

        return;
    }
};

#endif
