#pragma once

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <numeric>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace gtx::texture {

template <typename PageBase, typename Payload> struct atlas {
    using page_base_t = PageBase;
    using payload_t = Payload;

    using coord_t = uint16_t; // sizing and positioning
    using offset_t = int16_t;

    using pageref_t = uint16_t;
    using rowref_t = uint16_t;
    using cellref_t = uint16_t;
    using tileref_t = uint32_t;

    struct tile;
    struct cell;
    struct row;
    struct page;

    using tilevector = typename std::vector<tile>;
    using cellvector = typename std::vector<cell>;
    using rowvector = typename std::vector<row>;
    using pagevector = typename std::vector<page>;

    using tileiter = typename tilevector::iterator;
    using celliter = typename cellvector::iterator;
    using rowiter = typename rowvector::iterator;
    using pageiter = typename pagevector::iterator;

    struct tile {
        coord_t x;
        coord_t y;
        coord_t w;
        coord_t h;
        payload_t payload;
        pageref_t pageref;
    };

    struct cell {
        coord_t x;
        coord_t w;
        coord_t h;
    };

    struct row {
        coord_t y;
        coord_t h;
        cellvector cells;
        bool sealed = false;
    };

    struct page {
        page_base_t base;
        rowvector rows;
        page(page_base_t&& base)
            : base{std::forward<page_base_t>(base)}
        {
        }
    };

    coord_t page_w;
    coord_t page_h;
    tilevector tiles;
    pagevector pages;

    atlas(coord_t page_w, coord_t page_h)
        : page_w{page_w}
        , page_h{page_h}
    {
        assert(page_w >= 8 && page_h >= 8);
    }

    void clear()
    {
        pages.clear();
        tiles.clear();
    }

    auto insert_tile(coord_t tile_w, coord_t tile_h, payload_t&& payload)
        -> tileref_t
    {
        tile_w = std::clamp(tile_w, coord_t{1}, page_w);
        tile_h = std::clamp(tile_h, coord_t{1}, page_h);

        auto best_page = pages.end();
        auto best_row = rowiter{};
        auto best_cell = celliter{};
        auto best_y = coord_t{0};
        auto best_h = coord_t{0};
        auto best_w = coord_t{0};

        auto pit = pageiter{};
        auto rit = rowiter{};
        auto cit = celliter{};

        if (best_page == pages.end()) {
            // find best location within one of the existing rows
            for (pit = pages.begin(); pit != pages.end(); ++pit)
                for (rit = pit->rows.begin(); rit != pit->rows.end(); ++rit) {
                    if (tile_h > rit->h)
                        continue;
                    for (cit = rit->cells.begin(); cit != rit->cells.end();
                         ++cit) {
                        if (tile_w > cit->w)
                            continue;
                        auto cell_h = rit->h - cit->h;
                        if (tile_h > cell_h)
                            continue;

                        if (best_page == pages.end() || cit->w < best_cell->w) {
                            best_cell = cit;
                            best_row = rit;
                            best_page = pit;
                            best_y = cit->h;
                        }
                    }
                }
        }

        if (best_page == pages.end()) {
            // find best matching end-of-row insertion point
            for (pit = pages.begin(); pit != pages.end(); ++pit)
                for (rit = pit->rows.begin(); rit != pit->rows.end(); ++rit) {
                    auto row_h = rit->sealed ? rit->h : page_h - rit->y;
                    if (tile_h > row_h)
                        continue;
                    auto remaining_w =
                        page_w - (rit->cells.back().x + rit->cells.back().w);
                    if (tile_w > remaining_w)
                        continue;
                    if (best_page == pages.end() || row_h < best_h ||
                        (row_h == best_h && remaining_w < best_w)) {
                        best_row = rit;
                        best_page = pit;
                        best_y = 0;
                        best_h = row_h;
                        best_w = remaining_w;
                    }
                }

            if (best_page != pages.end()) {
                auto x = best_row->cells.back().x + best_row->cells.back().w;
                best_cell = best_row->cells.emplace(best_row->cells.end());
                best_cell->x = x;
                best_cell->w = tile_w;
                best_y = 0;
                if (best_row->sealed)
                    assert(best_row->h >= best_h);
                else {
                    best_row->h = std::max(best_row->h, tile_h);
                    if (x + tile_w == page_w)
                        best_row->sealed = true;
                }
            }
        }

        if (best_page == pages.end()) {
            // new row
            for (pit = pages.begin(); pit != pages.end(); ++pit) {
                auto& last_row = pit->rows.back();
                auto y = last_row.y + last_row.h;
                if (tile_h > page_h - y)
                    continue;
                last_row.sealed = true;
                best_page = pit;
                break;
            }

            if (best_page != pages.end()) {
                auto y = best_page->rows.back().y + best_page->rows.back().h;
                best_row = best_page->rows.emplace(best_page->rows.end());
                best_row->y = y;
                best_row->h = tile_h;
                best_cell = best_row->cells.emplace(best_row->cells.end());
                best_cell->x = 0;
                best_cell->w = tile_w;
                best_y = 0;
            }
        }

        if (best_page == pages.end()) {
            if (!pages.empty())
                pages.back().rows.back().sealed = true;
            best_page = new_page();
            best_row = best_page->rows.emplace(best_page->rows.end());
            best_row->y = 0;
            best_row->h = tile_h;
            best_cell = best_row->cells.emplace(best_row->cells.end());
            best_cell->x = 0;
            best_cell->w = tile_w;
            best_y = 0;
        }

        auto& tile = tiles.emplace_back();
        tile.x = best_cell->x;
        tile.y = best_row->y + best_y;
        tile.w = tile_w;
        tile.h = tile_h;
        tile.payload = std::forward<payload_t>(payload);
        tile.pageref = static_cast<pageref_t>(best_page - pages.begin());

        auto y = best_y + tile_h;
        if (tile_w < best_cell->w) {
            auto new_x = best_cell->x;
            best_cell->x += tile_w;
            best_cell->w -= tile_w;
            best_cell = best_row->cells.emplace(best_cell);
            best_cell->x = new_x;
            best_cell->w = tile_w;
            best_cell->h = y;
        }
        else if (y > best_cell->h)
            best_cell->h = y;

        if (best_cell != best_row->cells.begin()) {
            auto prev_cell = best_cell - 1;
            if (prev_cell->h == best_cell->h) {
                prev_cell->w += best_cell->w;
                best_cell = best_row->cells.erase(best_cell) - 1;
            }
        }

        if (auto next_cell = best_cell + 1;
            next_cell != best_row->cells.end() &&
            next_cell->h == best_cell->h) {
            best_cell->w += next_cell->w;
            best_cell = best_row->cells.erase(next_cell);
        }

        return static_cast<tileref_t>(tiles.size());
    }

private:
    auto new_page() -> pageiter
    {
        return pages.emplace(pages.end(), page_base_t{page_w, page_h});
    }
};

} // namespace texture