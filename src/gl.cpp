#include <gtx/device.hpp>
#include <gtx/tx-page.hpp>

#include <glad/glad.h>

namespace gtx {

struct texture::page_data {
    page_data(const page_data&) = delete;
    ~page_data()
    {
        if (name)
            glDeleteTextures(1, &name);
    }

    GLuint name = 0;
    texel_size sz = {0, 0};
    bool wrap = false;
    page_data(GLuint name, const texel_size& sz, bool wrap)
        : name{name}
        , sz{sz}
        , wrap{wrap}
    {
    }
    friend struct texture::page;
    friend struct texture::sprite;
};

std::vector<std::shared_ptr<texture::page_data>> pages;

void set_device(device_info const& info)
{
    // noop for OpenGL
}

auto new_page(const texture::texel_size& sz,
    bool wrap) -> std::shared_ptr<texture::page_data>
{
    if (!sz.w || !sz.h)
        return {};

    if (glGetError())
        return {};

    auto gln = GLuint{0};

    glGenTextures(1, &gln);
    glBindTexture(GL_TEXTURE_2D, gln);
    if (wrap) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    else {
        float color[] = {0.0f, 0.0f, 0.0f, 0.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GLsizei(sz.w), GLsizei(sz.h), 0,
        GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    if (glGetError()) {
        glDeleteTextures(1, &gln);
        return {};
    }

    auto p = std::make_shared<texture::page_data>(gln, sz, wrap);
    pages.push_back(p);
    return p;
}

auto texture::page::update(texture::texel_box const& box, const uint32_t* data,
    size_t data_stride) -> bool
{
    if (!data || data_stride < size_t(box.w))
        return false;

    if (auto pp = pd_.lock()) {
        auto& pd = *pp;
        if (box.x + box.w > pd.sz.w || box.y + box.h > pd.sz.h)
            return false;

        glPixelStorei(GL_UNPACK_ROW_LENGTH, int(data_stride));
        glBindTexture(GL_TEXTURE_2D, pd.name);
        glTexSubImage2D(GL_TEXTURE_2D, 0, box.x, box.y, box.w, box.h, GL_RGBA,
            GL_UNSIGNED_BYTE, data);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        return true;
    }
    return false;
}

auto texture::page::native_handle() const -> void*
{
    if (auto pp = pd_.lock())
        return (void*)intptr_t(pp->name);
    return nullptr;
}

auto texture::page::get_size() const -> texture::texel_size
{
    if (auto pp = pd_.lock())
        return pp->sz;
    return {0, 0};
}

void texture::page::setup(texture::texel_size const& sz, bool wrap)
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
        return (void*)intptr_t(pp->name);
    return nullptr;
}

} // namespace gtx