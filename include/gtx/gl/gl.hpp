#pragma once

#include <glad/glad.h>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>

namespace gtx::gl {

class shader_error : public std::runtime_error {
public:
    explicit shader_error(const std::string& m)
        : std::runtime_error(m.c_str())
    {
    }

    explicit shader_error(const char* m)
        : std::runtime_error(m)
    {
    }
};

enum class state {
    program,
    array_buffer,
    vertex_array,
};

namespace detail {

template <state S> struct save;

template <> struct save<state::program> {
    save() { glGetIntegerv(GL_CURRENT_PROGRAM, &v); }
    ~save() { glUseProgram(v); }
    GLint v;
};

template <> struct save<state::array_buffer> {
    save() { glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &v); }
    ~save() { glBindBuffer(GL_ARRAY_BUFFER, v); }
    GLint v;
};

template <> struct save<state::vertex_array> {
    save() { glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &v); }
    ~save() { glBindVertexArray(v); }
    GLint v;
};

} // namespace detail

template <state... Args> using save = std::tuple<detail::save<Args>...>;

struct named {
    constexpr named(GLuint name = 0) noexcept
        : name{name}
    {
    }
    constexpr named(named&& other) noexcept
        : name{std::exchange(other.name, 0)}
    {
    }

    operator bool() const { return name != 0; }

    auto obj() const -> GLuint { return name; }
    operator GLuint() const { return name; }

protected:
    GLuint name;
};

struct shader : public named {

    shader(GLenum shader_type, std::string_view code)
        : named{glCreateShader(shader_type)}
    {
        if (!name) {
            std::puts("failed to create shader");
            throw shader_error("failed to create shader");
        }
        if (!compile(code)) {
            auto err = shader_error{infolog()};
            release();
            std::puts(err.what());
            throw err;
        }
    }

    constexpr shader(shader&&) noexcept = default;

    ~shader() { release(); }

    operator bool() const
    {
        if (!name)
            return false;
        GLint res;
        glGetShaderiv(name, GL_COMPILE_STATUS, &res);
        return res != GL_FALSE;
    }

    void release()
    {
        if (name)
            glDeleteShader(name);
        name = 0;
    }

    auto compile(std::string_view code) -> bool
    {
        const char* s = code.data();
        GLint n = static_cast<GLint>(code.size());
        glShaderSource(name, 1, &s, &n);
        glCompileShader(name);
        GLint res;
        glGetShaderiv(name, GL_COMPILE_STATUS, &res);
        return res != GL_FALSE;
    }

    auto infolog() const -> std::string
    {
        GLint n;
        glGetShaderiv(name, GL_INFO_LOG_LENGTH, &n);
        if (n <= 0)
            return {};
        auto ret = std::string(static_cast<size_t>(n), 0);
        glGetShaderInfoLog(name, n, &n, &ret[0]);
        return ret;
    }
};

struct program : public named {

    program(std::initializer_list<std::reference_wrapper<shader>> shaders)
    {
        name = glCreateProgram();
        for (auto const& sr : shaders) {
            auto const& s = sr.get();
            if (!s)
                return;
            glAttachShader(name, s);
        }
        if (!link()) {
            auto err = shader_error{infolog()};
            puts(err.what());
            throw err;
        }
    }

    program(program&& other) noexcept = default;

    ~program() { release(); }

    operator bool() const
    {
        if (!name)
            return false;
        GLint res;
        glGetProgramiv(name, GL_LINK_STATUS, &res);
        return res != GL_FALSE;
    }

    void release()
    {
        if (name)
            glDeleteProgram(name);
        name = 0;
    }

    auto link() const -> bool
    {
        glLinkProgram(name);
        return linked();
    }

    auto linked() const -> bool
    {
        if (!name)
            return false;
        GLint res;
        glGetProgramiv(name, GL_LINK_STATUS, &res);
        return res != GL_FALSE;
    }

    void use() const { glUseProgram(name); }

    auto attrib_location(const char* attr_name) const -> GLint
    {
        return glGetAttribLocation(name, attr_name);
    }

    auto num_active_attribs() const -> GLint
    {
        auto n = GLint{};
        glGetProgramiv(name, GL_ACTIVE_ATTRIBUTES, &n);
        return n;
    }

    auto num_active_uniforms() const -> GLint
    {
        auto n = GLint{};
        glGetProgramiv(name, GL_ACTIVE_UNIFORMS, &n);
        return n;
    }

    void active_attrib(
        GLuint index, GLint& size, GLenum& type, std::string& name) const
    {
        static constexpr auto buf_size = GLsizei{32};
        static GLchar buf[buf_size];
        auto length = GLsizei{};
        glGetActiveAttrib(
            this->name, index, buf_size, &length, &size, &type, buf);
        name = buf;
    }

    void active_uniform(
        GLuint index, GLint& size, GLenum& type, std::string& name) const
    {
        static constexpr auto buf_size = GLsizei{32};
        static GLchar buf[buf_size];
        auto length = GLsizei{};
        glGetActiveUniform(
            this->name, index, buf_size, &length, &size, &type, buf);
        name = buf;
    }

    auto infolog() const -> std::string
    {
        GLint n;
        glGetProgramiv(name, GL_INFO_LOG_LENGTH, &n);
        if (n <= 0)
            return {};
        auto ret = std::string(static_cast<size_t>(n), 0);
        glGetProgramInfoLog(name, n, &n, &ret[0]);
        return ret;
    }
};

struct uniform {

    uniform() noexcept {}

    uniform(program const& prg, char const* name)
    {
        if (!prg)
            throw shader_error("invalid program");
        _program = prg.obj();
        _location = glGetUniformLocation(_program, name);
        // if (location < 0)
        //    throw shader_error("invalid uniform name");
    }

    auto write(int value) const
    {
        if (_location >= 0)
            glUniform1i(_location, value);
    }
    auto write(float value) const
    {
        if (_location >= 0)
            glUniform1f(_location, value);
    }
    auto write(float v0, float v1) const
    {
        if (_location >= 0)
            glUniform2f(_location, v0, v1);
    }
    auto write(float v0, float v1, float v2) const
    {
        if (_location >= 0)
            glUniform3f(_location, v0, v1, v2);
    }
    auto write(float v0, float v1, float v2, float v3) const
    {
        if (_location >= 0)
            glUniform4f(_location, v0, v1, v2, v3);
    }

    using matrix33_t = float[3][3];
    using matrix44_t = float[4][4];
    auto write(matrix33_t const& matrix) const
    {
        if (_location >= 0)
            glUniformMatrix3fv(_location, 1, GL_FALSE, &matrix[0][0]);
    }
    auto write(matrix44_t const& matrix) const
    {
        if (_location >= 0)
            glUniformMatrix4fv(_location, 1, GL_FALSE, &matrix[0][0]);
    }

    template <typename T> auto operator=(T const& value) const -> const uniform&
    {
        write(value);
        return *this;
    }

private:
    GLuint _program = 0;
    GLint _location = 0;
};

template <GLenum Target> struct buffer : public named {
    static constexpr auto target = Target;

    buffer() noexcept { glGenBuffers(1, &name); }
    ~buffer() { release(); }
    buffer(buffer&& other) noexcept = default;

    auto operator=(buffer&& other) noexcept -> buffer&
    {
        if (&other != this)
            name = std::exchange(other.name, 0);
        return *this;
    }

    void release()
    {
        if (name)
            glDeleteBuffers(1, &name);
        name = 0;
    }

    void bind() const { glBindBuffer(target, name); }

    void data(size_t size, void const* data, GLenum usage)
    {
        bind();
        glBufferData(target, size, data, usage);
    }

    void subdata(size_t offset, size_t size, void const* data)
    {
        glBufferSubData(target, offset, size, data);
    }
};

enum class comp {
    f32_unorm,
    f32_norm,
    i8,
    i8_unorm,
    i8_norm,
    i16,
    i16_unorm,
    i16_norm,
    i32,
    i32_unorm,
    i32_norm,
    u8,
    u8_unorm,
    u8_norm,
    u16,
    u16_unorm,
    u16_norm,
    u32,
    u32_unorm,
    u32_norm,
    f64,
};

struct vertex_attrib {
    char const* name;
    size_t ncomps;
    comp comptype;
    size_t stride;
    size_t offset;
};

struct vertex_array : public named {
    vertex_array() noexcept { glGenVertexArrays(1, &name); }
    vertex_array(buffer<GL_ARRAY_BUFFER> const& vertex_buffer, program const& p,
        std::initializer_list<vertex_attrib> attribs)
    {
        glGenVertexArrays(1, &name);
        bind();
        vertex_buffer.bind();
        for (auto&& attr : attribs) {
            auto const location = p.attrib_location(attr.name);
            if (location < 0)
                continue;

            glEnableVertexAttribArray(location);

            auto as_unorm = [&](GLenum type) {
                glVertexAttribPointer(location, attr.ncomps, type, GL_FALSE,
                    static_cast<GLsizei>(attr.stride),
                    reinterpret_cast<GLvoid const*>(attr.offset));
            };
            auto as_norm = [&](GLenum type) {
                glVertexAttribPointer(location, attr.ncomps, type, GL_FALSE,
                    static_cast<GLsizei>(attr.stride),
                    reinterpret_cast<GLvoid const*>(attr.offset));
            };
            auto as_int = [&](GLenum type) {
                glVertexAttribIPointer(location, attr.ncomps, type,
                    static_cast<GLsizei>(attr.stride),
                    reinterpret_cast<GLvoid const*>(attr.offset));
            };

            auto as_dbl = [&]() {
                glVertexAttribLPointer(location, attr.ncomps, GL_DOUBLE,
                    static_cast<GLsizei>(attr.stride),
                    reinterpret_cast<GLvoid const*>(attr.offset));
            };

            switch (attr.comptype) {
            case comp::f32_unorm:
                as_unorm(GL_FLOAT);
                break;
            case comp::f32_norm:
                as_unorm(GL_FLOAT);
                break;

            case comp::i8:
                as_int(GL_BYTE);
                break;
            case comp::i8_unorm:
                as_unorm(GL_BYTE);
                break;
            case comp::i8_norm:
                as_norm(GL_BYTE);
                break;

            case comp::i16:
                as_int(GL_SHORT);
                break;
            case comp::i16_unorm:
                as_unorm(GL_SHORT);
                break;
            case comp::i16_norm:
                as_norm(GL_SHORT);
                break;

            case comp::i32:
                as_int(GL_INT);
                break;
            case comp::i32_unorm:
                as_unorm(GL_INT);
                break;
            case comp::i32_norm:
                as_norm(GL_INT);
                break;

            case comp::u8:
                as_int(GL_UNSIGNED_BYTE);
                break;
            case comp::u8_unorm:
                as_unorm(GL_UNSIGNED_BYTE);
                break;
            case comp::u8_norm:
                as_norm(GL_UNSIGNED_BYTE);
                break;

            case comp::u16:
                as_int(GL_UNSIGNED_SHORT);
                break;
            case comp::u16_unorm:
                as_unorm(GL_UNSIGNED_SHORT);
                break;
            case comp::u16_norm:
                as_norm(GL_UNSIGNED_SHORT);
                break;

            case comp::u32:
                as_int(GL_UNSIGNED_INT);
                break;
            case comp::u32_unorm:
                as_unorm(GL_UNSIGNED_INT);
                break;
            case comp::u32_norm:
                as_norm(GL_UNSIGNED_INT);
                break;

            case comp::f64:
                as_dbl();
                break;
            }
        }
    }

    vertex_array(vertex_array&& other) noexcept = default;
    ~vertex_array() { glDeleteVertexArrays(1, &name); }
    void bind() { glBindVertexArray(name); }
};

} // namespace gtx::gl