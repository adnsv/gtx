#pragma once

#include <gtx/dx/factory.hpp>
#include <comdef.h>
#include <cstring>
#include <gtx/factory.hpp>
#include <stdexcept>
#include <wrl/client.h>

namespace gtx::dx {

struct shader_error_info {
    HRESULT hr;
    char const* error_msg;
    char const* entry_point;
    char const* target;
};

using shader_error_handler = std::function<void(const shader_error_info& info)>;

class error : public std::domain_error {
public:
    explicit error(const std::string& m)
        : std::domain_error(m.c_str())
    {
    }

    explicit error(const char* m)
        : std::domain_error(m)
    {
    }
};

inline void check_hr(HRESULT hr)
{
    if (FAILED(hr))
        throw _com_error(hr);
}

struct shader_source {
    char const* name;
    std::string_view data;
};

struct shader_code : std::vector<uint8_t> {
    shader_code(const shader_source& source, const D3D_SHADER_MACRO* macros,
        const char* entry_point, const char* target);

    shader_code(const shader_source& source, const D3D_SHADER_MACRO* macros,
        const char* entry_point, const char* target,
        shader_error_handler on_error);

    template <size_t L, size_t N>
    shader_code(const shader_source& source, const macros<L, N>& macros,
        const char* entry_point, const char* target)
        : shader_code{source, detail::data(macros), entry_point, target}
    {
    }

    template <size_t L, size_t N>
    shader_code(const shader_source& source, const macros<L, N>& macros,
        const char* entry_point, const char* target,
        shader_error_handler on_error)
        : shader_code{
              source, detail::data(macros), entry_point, target, on_error}
    {
    }
};

struct vertex_shader : public Microsoft::WRL::ComPtr<ID3D11VertexShader> {
    vertex_shader(const shader_code& buf);
    void bind();
};

struct geometry_shader : public Microsoft::WRL::ComPtr<ID3D11GeometryShader> {
    geometry_shader(const shader_code& buf);
    void bind();
    static void unbind();
};

struct pixel_shader : public Microsoft::WRL::ComPtr<ID3D11PixelShader> {
    pixel_shader(const shader_code& buf);
    void bind();
};

struct input_layout : public Microsoft::WRL::ComPtr<ID3D11InputLayout> {
    input_layout(const shader_code& buf,
        const std::vector<D3D11_INPUT_ELEMENT_DESC>& descriptors);
    void bind();
};

template <typename T>
struct constant_buffer : public Microsoft::WRL::ComPtr<ID3D11Buffer> {
    using value_type = T;
    static constexpr auto capacity_bytes = sizeof(T);

    constant_buffer(const T* init = nullptr)
    {
        auto device = get_device().device;
        if (!device)
            throw error("missing device");
        auto buf = D3D11_BUFFER_DESC{};
        buf.ByteWidth = sizeof(T);
        buf.Usage = D3D11_USAGE_DEFAULT;
        buf.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        buf.CPUAccessFlags = 0;

        if (!init) {
            check_hr(device->CreateBuffer(&buf, nullptr, GetAddressOf()));
        }
        else {
            auto data = D3D11_SUBRESOURCE_DATA{init, 0, 0};
            check_hr(device->CreateBuffer(&buf, &data, GetAddressOf()));
        }
    }

    constant_buffer(const T& init)
        : constant_buffer(&init)
    {
    }

    void bind_vs(unsigned slot)
    {
        auto* ptr = Get();
        auto context = get_device().context;
        if (context)
            context->VSSetConstantBuffers(slot, 1, &ptr);
    }

    void bind_gs(unsigned slot)
    {
        auto* ptr = Get();
        auto context = get_device().context;
        if (context)
            context->GSSetConstantBuffers(slot, 1, &ptr);
    }

    void bind_ps(unsigned slot)
    {
        auto* ptr = Get();
        auto context = get_device().context;
        if (context)
            context->PSSetConstantBuffers(slot, 1, &ptr);
    }

    void update(const T& value)
    {
        auto context = get_device().context;
        if (context)
            context->UpdateSubresource(Get(), 0, nullptr, &value, 0, 0);
    }

    auto operator=(const T& value) const -> constant_buffer&
    {
        update(value);
        return *this;
    }
};

template <typename T>
struct vertex_buffer : public Microsoft::WRL::ComPtr<ID3D11Buffer> {
    using vertex_type = T;

    vertex_buffer(size_t capacity = 0)
    {
        if (capacity)
            setup(nullptr, capacity);
    }

    void setup(const vertex_type* first, size_t count)
    {
        Reset();
        if (!count)
            return;

        auto device = get_device().device;
        if (!device)
            throw error("missing device");

        auto desc = D3D11_BUFFER_DESC{};
        desc.ByteWidth = static_cast<UINT>(sizeof(vertex_type) * count);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.StructureByteStride = sizeof(vertex_type);

        if (first) {
            auto data = D3D11_SUBRESOURCE_DATA{first, 0, 0};
            check_hr(device->CreateBuffer(&desc, &data, GetAddressOf()));
        }
        else {
            check_hr(device->CreateBuffer(&desc, nullptr, GetAddressOf()));
        }
    }

    auto capacity() const -> size_t
    {
        auto ptr = Get();
        if (!ptr)
            return 0;
        auto desc = D3D11_BUFFER_DESC{};
        ptr->GetDesc(&desc);
        return desc.ByteWidth / sizeof(vertex_type);
    }

    void bind(unsigned slot, size_t stride = sizeof(T), size_t offset = 0)
    {
        auto context = get_device().context;
        if (!context)
            throw error("missing context");
        auto ptr = Get();
        auto s = UINT(stride);
        auto o = UINT(offset);
        context->IASetVertexBuffers(slot, 1, &ptr, &s, &o);
    }

    void write(vertex_type const* first, size_t count)
    {
        if (count > capacity()) {
            setup(first, count);
            return;
        }
        if (!count || !first)
            return;
        auto context = get_device().context;
        if (!context)
            throw error("missing context");
        auto m = D3D11_MAPPED_SUBRESOURCE{};
        check_hr(context->Map(Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m));
        std::memcpy(m.pData, first, count * sizeof(vertex_type));
        context->Unmap(Get(), 0);
    }
};

enum class state {
    vertex_shader,
    geometry_shader,
    pixel_shader,
    vs_constant_buffer_0,
    gs_constant_buffer_0,
    ps_constant_buffer_0,
    vertex_buffer_0,
    input_layout,
    primitive_topology,
};

namespace detail {

template <state S> struct save;

template <> struct save<state::vertex_shader> {
    save()
    {
        auto context = get_device().context;
        if (!context)
            throw error("missing context");
        context->VSGetShader(&save_vs, vs_insts, &vs_count);
    }

    ~save()
    {
        auto context = get_device().context;
        if (context)
            context->VSSetShader(save_vs, vs_insts, vs_count);
        if (save_vs)
            save_vs->Release();
        for (UINT i = 0; i < vs_count; i++)
            if (vs_insts[i])
                vs_insts[i]->Release();
    }

    ID3D11VertexShader* save_vs;
    ID3D11ClassInstance* vs_insts[256];
    UINT vs_count;
};

template <> struct save<state::geometry_shader> {
    save()
    {
        auto context = get_device().context;
        if (!context)
            throw error("missing context");
        context->GSGetShader(&save_gs, gs_insts, &gs_count);
    }

    ~save()
    {
        auto context = get_device().context;
        if (context)
            context->GSSetShader(save_gs, gs_insts, gs_count);
        if (save_gs)
            save_gs->Release();
        for (UINT i = 0; i < gs_count; i++)
            if (gs_insts[i])
                gs_insts[i]->Release();
    }

    ID3D11GeometryShader* save_gs;
    ID3D11ClassInstance* gs_insts[256];
    UINT gs_count;
};

template <> struct save<state::pixel_shader> {
    save()
    {
        auto context = get_device().context;
        if (!context)
            throw error("missing context");
        context->PSGetShader(&save_ps, ps_insts, &ps_count);
    }

    ~save()
    {
        auto context = get_device().context;
        if (context)
            context->PSSetShader(save_ps, ps_insts, ps_count);
        if (save_ps)
            save_ps->Release();
        for (UINT i = 0; i < ps_count; i++)
            if (ps_insts[i])
                ps_insts[i]->Release();
    }

    ID3D11PixelShader* save_ps;
    ID3D11ClassInstance* ps_insts[256];
    UINT ps_count;
};

template <> struct save<state::vs_constant_buffer_0> {
    save()
    {
        auto context = get_device().context;
        if (!context)
            throw error("missing context");
        context->VSGetConstantBuffers(0, 1, &save_buffer);
    }

    ~save()
    {
        auto context = get_device().context;
        if (context)
            context->VSSetConstantBuffers(0, 1, &save_buffer);
        if (save_buffer)
            save_buffer->Release();
    }

    ID3D11Buffer* save_buffer;
};

template <> struct save<state::gs_constant_buffer_0> {
    save()
    {
        auto context = get_device().context;
        if (!context)
            throw error("missing context");
        context->GSGetConstantBuffers(0, 1, &save_buffer);
    }

    ~save()
    {
        auto context = get_device().context;
        if (context)
            context->GSSetConstantBuffers(0, 1, &save_buffer);
        if (save_buffer)
            save_buffer->Release();
    }

    ID3D11Buffer* save_buffer;
};

template <> struct save<state::ps_constant_buffer_0> {
    save()
    {
        auto context = get_device().context;
        if (!context)
            throw error("missing context");
        context->PSGetConstantBuffers(0, 1, &save_buffer);
    }

    ~save()
    {
        auto context = get_device().context;
        if (context)
            context->PSSetConstantBuffers(0, 1, &save_buffer);
        if (save_buffer)
            save_buffer->Release();
    }

    ID3D11Buffer* save_buffer;
};

template <> struct save<state::vertex_buffer_0> {
    save()
    {
        auto context = get_device().context;
        if (!context)
            throw error("missing context");
        context->IAGetVertexBuffers(0, 1, &save_vb, &save_stride, &save_offset);
    }

    ~save()
    {
        auto context = get_device().context;
        if (context)
            context->IASetVertexBuffers(
                0, 1, &save_vb, &save_stride, &save_offset);
        if (save_vb)
            save_vb->Release();
    }

    ID3D11Buffer* save_vb;
    UINT save_stride;
    UINT save_offset;
};

template <> struct save<state::input_layout> {
    save()
    {
        auto context = get_device().context;
        if (!context)
            throw error("missing context");
        context->IAGetInputLayout(&save_layout);
    }

    ~save()
    {
        auto context = get_device().context;
        if (context)
            context->IASetInputLayout(save_layout);
        if (save_layout)
            save_layout->Release();
    }

    ID3D11InputLayout* save_layout;
};

template <> struct save<state::primitive_topology> {
    save()
    {
        auto context = get_device().context;
        if (!context)
            throw error("missing context");
        context->IAGetPrimitiveTopology(&save_primitive_topology);
    }

    ~save()
    {
        auto context = get_device().context;
        if (context)
            context->IASetPrimitiveTopology(save_primitive_topology);
    }

    D3D11_PRIMITIVE_TOPOLOGY save_primitive_topology;
};

} // namespace detail

template <state... Args> using save = std::tuple<detail::save<Args>...>;

} // namespace gtx::dx