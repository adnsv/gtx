#pragma once

namespace gtx {

struct mat4x4 {
    float elts[4][4] = {};

    static constexpr auto identity() -> mat4x4
    {
        return mat4x4{{
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1},
        }};
    }

    static constexpr auto ortho_projection(float l, float t, float r, float b)
        -> mat4x4
    {
        return mat4x4{{
            {2.0f / (r - l), 0, 0, 0},
            {0, 2.0f / (t - b), 0, 0},
            {0, 0, -1, 0},
            {(r + l) / (l - r), (t + b) / (b - t), 0, 1},
        }};
    }
};

} // namespace gtx