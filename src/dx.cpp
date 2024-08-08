#include <gtx/device.hpp>
#include <gtx/tx-page.hpp>

#include <cstring>
#include <d3d11.h>

namespace gtx {

auto new_page(texture::texel_size const& sz,
    bool wrap) -> std::shared_ptr<texture::page_data>;

struct texture::page_data {
    page_data(page_data const&) = delete;

    page_data(ID3D11ShaderResourceView* srv, texel_size const& sz, bool wrap)
        : srv{srv}
        , sz{sz}
        , wrap{wrap}
    {
    }

    ~page_data()
    {
        if (srv) {
            srv->Release();
            srv = nullptr;
        }
    }

private:
    ID3D11ShaderResourceView* srv = nullptr;
    texel_size sz = {0, 0};
    bool wrap = false;

    friend struct page;
    friend struct sprite;
    friend auto new_page(texture::texel_size const& sz,
        bool wrap) -> std::shared_ptr<texture::page_data>;
};

auto device_info::operator=(device_info const& rhs) -> device_info&
{
    if (device != rhs.device) {
        if (device)
            device->Release();
        device = rhs.device;
        if (device)
            device->AddRef();
    }
    if (context != rhs.context) {
        if (context)
            context->Release();
        context = rhs.context;
        if (context)
            context->AddRef();
    }
    return *this;
}

device_info::~device_info()
{
    if (context)
        context->Release();
    if (device)
        device->Release();
}

device_info d;
std::vector<std::shared_ptr<texture::page_data>> pages;

void set_device(device_info const& rhs) { d = rhs; }
void set_frame(frame_info const&) {}
auto get_device() -> device_info const& { return d; }

auto new_page(texture::texel_size const& sz,
    bool wrap) -> std::shared_ptr<texture::page_data>
{
    if (!sz.w || !sz.h || !d.device)
        return {};

    ID3D11ShaderResourceView* srv = nullptr;
    {
        auto desc = D3D11_TEXTURE2D_DESC{0};
        desc.Width = sz.w;
        desc.Height = sz.h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        if (ID3D11Texture2D * pTexture;
            SUCCEEDED(d.device->CreateTexture2D(&desc, nullptr, &pTexture))) {
            auto srvDesc = D3D11_SHADER_RESOURCE_VIEW_DESC{};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            d.device->CreateShaderResourceView(pTexture, &srvDesc, &srv);
            pTexture->Release();
        }
    }

    if (!srv)
        return {};

    auto p = std::make_shared<texture::page_data>(srv, sz, wrap);
    pages.push_back(p);
    return p;
}

auto texture::page::update(
    texel_box const& box, uint32_t const* data, std::size_t data_stride) -> bool
{
    if (!d.context)
        return false;

    if (!data || data_stride < size_t(box.w))
        return false;

    if (auto pp = pd_.lock()) {
        auto& pd = *pp;
        if (box.x + box.w > pd.sz.w || box.y + box.h > pd.sz.h)
            return false;
        if (!pd.srv)
            return false;

        auto d3d_box =
            D3D11_BOX{box.x, box.y, 0, box.x + box.w, box.y + box.h, 1};

        ID3D11Resource* res;
        pd.srv->GetResource(&res);
        if (res) {
            d.context->UpdateSubresource(res, 0, &d3d_box, data,
                UINT(data_stride * sizeof(uint32_t)), 0);
            res->Release();
        }
        return true;
    }
    return false;
}

auto texture::page::native_handle() const -> void*
{
    if (auto pp = pd_.lock())
        return (void*)intptr_t(pp->srv);
    return nullptr;
}

auto texture::page::get_size() const -> texel_size
{
    if (auto pp = pd_.lock())
        return pp->sz;
    return {0, 0};
}

void texture::page::setup(texel_size const& sz, bool wrap)
{
    if (auto pp = pd_.lock()) {
        if (pp->sz == sz && pp->wrap == wrap)
            return;
    }
    pd_ = new_page(sz, wrap);
}

void texture::page::release_all() { pages.clear(); }

auto texture::sprite::native_handle() const -> void*
{
    if (auto pp = pd_.lock())
        return (void*)intptr_t(pp->srv);
    return nullptr;
}

} // namespace gtx