#include <gtx/dx/dx.hpp>

#include <d3dcompiler.h>

namespace gtx::dx {

shader_code::shader_code(shader_source const& source,
    D3D_SHADER_MACRO const* macros, char const* entry_point, char const* target)
{
    Microsoft::WRL::ComPtr<ID3DBlob> data;
    check_hr(D3DCompile(source.data.data(), source.data.size(), source.name,
        macros, nullptr, entry_point, target, 0, 0, &data, nullptr));
    auto ptr = reinterpret_cast<uint8_t*>(data->GetBufferPointer());
    assign(ptr, ptr + data->GetBufferSize());
}

shader_code::shader_code(shader_source const& source,
    D3D_SHADER_MACRO const* macros, char const* entry_point, char const* target,
    shader_error_handler on_error)
{
    Microsoft::WRL::ComPtr<ID3DBlob> data;
    Microsoft::WRL::ComPtr<ID3DBlob> error;
    auto hr = D3DCompile(source.data.data(), source.data.size(), source.name,
        macros, nullptr, entry_point, target, 0, 0, &data, &error);

    if (FAILED(hr)) {
        data.Reset();
        auto msg =
            error ? reinterpret_cast<const char*>(error->GetBufferPointer())
                  : "unknown";
        if (on_error)
            on_error({hr, msg, entry_point, target});
    }
    else {
        auto ptr = reinterpret_cast<uint8_t*>(data->GetBufferPointer());
        assign(ptr, ptr + data->GetBufferSize());
        data.Reset();
        error.Reset();
    }
}

vertex_shader::vertex_shader(shader_code const& buf)
{
    auto device = gtx::get_device().device;
    if (!device)
        throw error("missing device");
    check_hr(device->CreateVertexShader(
        buf.data(), buf.size(), nullptr, GetAddressOf()));
};

void vertex_shader::bind()
{
    auto context = gtx::get_device().context;
    if (context)
        context->VSSetShader(Get(), nullptr, 0);
}

geometry_shader::geometry_shader(shader_code const& buf)
{
    auto device = gtx::get_device().device;
    if (!device)
        throw error("missing device");
    check_hr(device->CreateGeometryShader(
        buf.data(), buf.size(), nullptr, GetAddressOf()));
};

void geometry_shader::bind()
{
    auto context = gtx::get_device().context;
    if (context)
        context->GSSetShader(Get(), nullptr, 0);
}

void geometry_shader::unbind()
{
    auto context = gtx::get_device().context;
    if (context)
        context->GSSetShader(nullptr, nullptr, 0);
}

pixel_shader::pixel_shader(const shader_code& buf)
{
    auto device = gtx::get_device().device;
    if (!device)
        throw error("missing device");
    check_hr(device->CreatePixelShader(
        buf.data(), buf.size(), nullptr, GetAddressOf()));
};

void pixel_shader::bind()
{
    auto context = gtx::get_device().context;
    if (context)
        context->PSSetShader(Get(), nullptr, 0);
}

input_layout::input_layout(shader_code const& buf,
    const std::vector<D3D11_INPUT_ELEMENT_DESC>& descriptors)
{
    auto device = gtx::get_device().device;
    if (!device)
        throw error("missing device");
    check_hr(device->CreateInputLayout(descriptors.data(),
        static_cast<UINT>(descriptors.size()), buf.data(), buf.size(),
        GetAddressOf()));
}

void input_layout::bind()
{
    auto context = gtx::get_device().context;
    if (context)
        context->IASetInputLayout(Get());
}

} // namespace gtx::dx