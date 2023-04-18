#pragma once

enum attribute
{
    ATTRIB_POSITION,
    ATTRIB_TEXCOORD,
    ATTRIB_COLOR,
    ATTRIB_COUNT
};

struct render_batch
{
    GLuint Vao;
    GLuint Vbos[ATTRIB_COUNT];
    GLuint Texture;

    f32 *Positions;
    f32 *Colors;
    f32 *TexCoords;
    s32 VertexCount;
    s32 MaxVertexCount;

    GLenum Mode;
};

struct render_state
{
    GLuint Program;
    s32 DrawCalls;
    render_batch *Batch;
    glm::mat4 Projection;
    glm::mat4 ModelView;
};

struct bitmap_font
{
    s32 CharWidth;
    s32 CharHeight;
    s32 Columns;
    s32 Rows;
    GLuint TextureHandle;
};

struct bitmap_font_vertex
{
    glm::vec3 Position;
    glm::vec2 UV;
    glm::vec4 Color;
};

struct bitmap_font_renderer
{
    GLuint Vbo;
    GLuint Vao;
    GLuint Program;
    bitmap_font_vertex *Vertices;
};