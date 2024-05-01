#pragma once

#include <functional>
#include <span>
#include <vector>

namespace gtx {

struct index_range {
    size_t first;
    size_t last;
};

template <typename Vertex> struct vlist {
    struct write;

    using vertex_type = Vertex;
    using callback_type = std::function<void(write const&)>;
    using segment_id = size_t;

    struct write {
        void operator()(vertex_type const& v) const
        {
            _target._vertices.push_back(v);
        }
        void operator()(vertex_type&& v) const
        {
            _target._vertices.push_back(std::move(v));
        }
       /* void operator()(std::span<vertex_type const> vv) const
        {
            _target._vertices.insert(
                _target._vertices.end(), vv.begin(), vv.end());
        }*/

    private:
        vlist& _target;
        write(vlist* target)
            : _target{*target}
        {
        }
        friend struct vlist;
    };

    auto insert(std::function<void(write const&)> callback) -> segment_id
    {
        if (!callback)
            return segment_id(-1);
        auto const first = _vertices.size();
        auto ctx = write{this};
        callback(ctx);
        auto const last = _vertices.size();
        if (last <= first)
            return segment_id(-1);
        _dirty_flag = true;
        _segments.push_back(index_range{first, last});
        return segment_id(_segments.size()-1);
    };

    auto vertices() const { return std::span<vertex_type const>{_vertices}; }
    auto segments() const { return std::span<index_range const>{_segments}; }
    auto clear()
    {
        _vertices.clear();
        _segments.clear();
        _dirty_flag = true;
    }
    auto dirty_flag() const { return _dirty_flag; }
    void reset_dirty_flag() { _dirty_flag = false; }

protected:
    std::vector<vertex_type> _vertices;
    std::vector<index_range> _segments;
    bool _dirty_flag = false;
};

} // namespace gtx