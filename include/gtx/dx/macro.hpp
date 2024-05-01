#pragma once

#include <array>
#include <functional>

namespace gtx::dx {

template <size_t L, size_t N> struct macros : public std::array<char, L> {
};

// macro - constructs a macro from "name" or "name=definition"
template <size_t L> constexpr auto macro(const char (&s)[L])
{
    macros<L, 1> r{};
    for (int i = 0; i < L; ++i)
        r[i] = s[i];
    return r;
}

template <size_t V> constexpr auto ndigits() -> size_t
{
    if constexpr (V < 10)
        return 1;
    if constexpr (V < 100)
        return 2;
    if constexpr (V < 1000)
        return 3;
    return 4;
}

constexpr auto write_digits(char* p, size_t n) -> char*
{
    if (n < 10) {
        *p++ = '0' + char(n);
    }
    else if (n < 100) {
        *p++ = '0' + char(n / 10);
        *p++ = '0' + char(n % 10);
    }
    else if (n < 1000) {
        *p++ = '0' + char(n / 100);
        n %= 100;
        *p++ = '0' + char(n / 10);
        *p++ = '0' + char(n % 10);
    }
    else {
        n %= 10000;
        *p++ = '0' + char(n / 1000);
        n %= 1000;
        *p++ = '0' + char(n / 100);
        n %= 1000;
        *p++ = '0' + char(n / 10);
        *p++ = '0' + char(n % 10);
    }
    return p;
}

// macro - constructs a macro from "name#" or "name=definition#"
// substututes # with its decimal numeric value
template <size_t N, size_t L> constexpr auto macro_v(const char (&s)[L])
{
    constexpr auto LL = L + ndigits<N>() - 1;
    macros<LL, 1> r{};
    auto p = r.data();
    for (size_t i = 0; i < L; ++i) {
        auto c = s[i];
        if (c == '#')
            p = write_digits(p, N);
        else
            *(p++) = c;
    }
    return r;
}

template <size_t L, size_t L2>
constexpr auto macro_s(const char (&s)[L], const char (&s2)[L2])
{
    constexpr auto LL = L + L2 - 2;
    macros<LL, 1> r{};
    auto p = r.data();
    for (size_t i = 0; i < L; ++i) {
        auto c = s[i];
        if (c == '#') {
            for (size_t j = 0; j + 1 < L2; ++j)
                *(p++) = s2[j];
        }
        else
            *(p++) = c;
    }
    return r;
}

template <size_t L, size_t L2>
constexpr auto macro_s(const char (&s)[L], std::array<char, L2> const& s2)
{
    constexpr auto LL = L + L2 - 1;
    macros<LL, 1> r{};
    auto p = r.data();
    for (size_t i = 0; i < L; ++i) {
        auto c = s[i];
        if (c == '#') {
            for (size_t j = 0; j + 1 < L2; ++j)
                *(p++) = s2[j];
        }
        else
            *(p++) = c;
    }
    return r;
}

template <size_t L1, size_t N1, size_t L2, size_t N2>
constexpr auto operator+(macros<L1, N1> const& m1, macros<L2, N2> const& m2)
{
    macros<L1 + L2, N1 + N2> r{};
    for (size_t i = 0; i < L1; ++i)
        r[i] = m1[i];
    for (size_t i = 0; i < L2; ++i)
        r[L1 + i] = m2[i];
    return r;
}

namespace detail {

template <size_t L, size_t N> struct data {
    constexpr data(macros<L, N> const& m)
    {
        size_t i = 0;
        for (size_t j = 0; j < N; ++j) {
            this->fields[j].Name = &this->buffer[i];
            this->fields[j].Definition = nullptr;
            for (; m[i]; ++i) {
                buffer[i] = m[i];
                if (m[i] == '=' && !this->fields[j].Definition) {
                    buffer[i] = 0;
                    this->fields[j].Definition = &this->buffer[i + 1];
                }
            }
            ++i;
        }
        this->fields[N].Name = nullptr;
        this->fields[N].Definition = nullptr;
    }

    constexpr operator const D3D_SHADER_MACRO*() const { return fields.data(); }

private:
    std::array<char, L> buffer = {};
    std::array<D3D_SHADER_MACRO, N + 1> fields = {};
};

} // namespace detail

} // namespace gtx::dx