#include <gtx/factory.hpp>
#include <gtx/tx-page.hpp>

#include <cstring>
#include <d3d11.h>

namespace gtx {

struct factory;

struct texture::page_data {
    page_data(page_data const&) = delete;

    ~page_data()
    {
        if (srv) {
            srv->Release();
            srv = nullptr;
        }
    }

private:
    page_data(ID3D11ShaderResourceView* srv, texel_size const& sz, bool wrap)
        : srv{srv}
        , sz{sz}
        , wrap{wrap}
    {
    }

    ID3D11ShaderResourceView* srv = nullptr;
    texel_size sz = {0, 0};
    bool wrap = false;
    friend struct factory;
    friend struct page;
    friend struct sprite;
};

struct factory {
    static auto get() -> factory*
    {
        static auto instance = factory{};
        return &instance;
    }

    ~factory()
    {
        if (_info.context)
            _info.context->Release();
        if (_info.device)
            _info.device->Release();
    }

    static auto device() { return get()->_info.device; }
    static auto context() { return get()->_info.context; }

private:
    std::vector<std::shared_ptr<texture::page_data>> pages;
    device_info _info;

    auto new_page(texture::texel_size const& sz,
        bool wrap) -> std::shared_ptr<texture::page_data>;

    void set_device(device_info const& info)
    {
        if (_info.device == info.device)
            return;
        if (_info.device)
            _info.device->Release();
        _info.device = info.device;
        if (_info.device)
            _info.device->AddRef();
        if (_info.context == info.context)
            return;
        if (_info.context)
            _info.context->Release();
        _info.context = info.context;
        if (_info.context)
            _info.context->AddRef();
    }

    friend struct texture::page;
    friend void set_device(device_info const&);
    friend auto get_device() -> device_info const&;
};

void set_device(device_info const& info) { factory::get()->set_device(info); }
auto get_device() -> device_info const& { return factory::get()->_info; }

auto texture::page::update(
    texel_box const& box, uint32_t const* data, std::size_t data_stride) -> bool
{
    auto context = factory::get()->context();
    if (!context)
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
            context->UpdateSubresource(res, 0, &d3d_box, data,
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
    pd_ = factory::get()->new_page(sz, wrap);
}

void texture::page::release_all() { factory::get()->pages.clear(); }

auto texture::sprite::native_handle() const -> void*
{
    if (auto pp = pd_.lock())
        return (void*)intptr_t(pp->srv);
    return nullptr;
}

auto factory::new_page(texture::texel_size const& sz,
    bool wrap) -> std::shared_ptr<texture::page_data>
{
    if (!sz.w || !sz.h)
        return {};

    auto device = factory::device();
    if (!device)
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
            SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &pTexture))) {
            auto srvDesc = D3D11_SHADER_RESOURCE_VIEW_DESC{};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            device->CreateShaderResourceView(pTexture, &srvDesc, &srv);
            pTexture->Release();
        }
    }

    if (!srv)
        return {};

    auto pg = std::shared_ptr<texture::page_data>(
        new texture::page_data{srv, sz, wrap});
    pages.push_back(pg);
    return pg;
}

} // namespace gtx