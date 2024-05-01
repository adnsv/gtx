#pragma once

#include "tx-page.hpp"

namespace gtx::texture {

struct grid;

struct cell {
    cell() noexcept {}
    cell(cell const&) = delete;
    ~cell() { release(); }
    void release();

    auto empty() const -> bool { return !_grid || !slot_id; }

    // locate searches for a cell within the grid and returns
    // its location. If the cell does not yet have a location,
    // it is created and the update function is called.
    auto locate(std::function<void(sprite const &)> update)
        -> std::optional<sprite>;

private:
    grid* _grid = nullptr;
    uint64_t slot_id = 0;

    cell(grid* grid)
        : _grid{grid}
    {
    }

    friend struct grid;
};

struct grid {
    grid(texel_size const& cell_size, uint32_t ncols, uint32_t nrows)
        : _cellsz{cell_size}
        , _ncols{ncols}
        , _nrows{nrows}
        , _pagesz{cell_size.w * ncols, cell_size.h * nrows}
    {
    }

    auto new_cell() { return std::unique_ptr<cell>{new cell{this}}; }

    auto cell_size() const { return _cellsz; }

private:
    texel_size _cellsz;
    texel_size _pagesz;
    uint32_t _ncols;
    uint32_t _nrows;
    std::vector<uint64_t> slots;
    std::vector<page> pages;
    friend struct cell;

    auto clear_slot(uint64_t const& slot_id)
    {
        for (auto& id : slots)
            if (id == slot_id) {
                id = 0;
                return;
            }
    }
};

inline void cell::release()
{
    if (_grid && slot_id >= 0) {
        _grid->clear_slot(slot_id);
        slot_id = 0;
    }
}

inline auto cell::locate(std::function<void(sprite const&)> update)
    -> std::optional<sprite>
{
    if (!_grid)
        return {};

    auto& slots = _grid->slots;
    auto it =
        slot_id ? std::find(slots.begin(), slots.end(), slot_id) : slots.end();

    auto just_created = false;

    if (it == slots.end()) {
        slot_id = 0;
        if (!update)
            return {};
        static uint64_t last_slot_id = 0;
        slot_id = ++last_slot_id;
        it = std::find(slots.begin(), slots.end(), 0);
        if (it != slots.end())
            *it = slot_id;
        else
            it = slots.insert(slots.end(), slot_id);

        just_created = true;
    }

    auto i = uint32_t(it - slots.begin());
    auto col = i % _grid->_ncols;
    i /= _grid->_ncols;
    auto row = i % _grid->_nrows;
    i /= _grid->_nrows;
    if (i >= _grid->pages.size()) {
        // a new page is required
        auto& pg = _grid->pages.emplace_back();
        pg.setup(_grid->_pagesz);
    }

    auto ret = sprite{_grid->pages[i], texel_box{
                                           col * _grid->_cellsz.w,
                                           row * _grid->_cellsz.h,
                                           _grid->_cellsz.w,
                                           _grid->_cellsz.h,
                                       }};
    if (just_created && update)
        update(ret);

    return ret;
}

} // namespace texture