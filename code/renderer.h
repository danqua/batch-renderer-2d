#pragma once

#define RENDER_BATCH_MAX_CAPACITY 2048

#define COLOR_WHITE color{ 255, 255, 255, 255 }
#define COLOR_BLACK color{   0,   0,   0, 255 }

struct color
{
    u8 R;
    u8 G;
    u8 B;
    u8 A;
};

struct rect
{
    s32 X;
    s32 Y;
    s32 Width;
    s32 Height;
};

enum attribute
{
    ATTRIB_POSITION,
    ATTRIB_TEXCOORD,
    ATTRIB_COLOR,
    ATTRIB_COUNT
};

struct vertex_buffer
{
    u32 Vao;
    u32 Vbos[4];
    f32 *Positions;
    f32 *TexCoords;
    f32 *Colors;
    u32 *Indices;
    u64 Capacity;
    u32 VertexCount;
    u32 ElementCount;
    s32 Usage;
};

struct texture
{
    u32 Handle;
    s32 Width;
    s32 Height;
};

struct render_batch
{
    s32 Mode;
    u32 Texture;
    vertex_buffer Buffer;
};

enum render_mode
{
    R_POINTS,
    R_LINES,
    R_TRIANGLES,
    R_TEXTURES,
    R_MODE_COUNT
};

struct render_state
{
    s32 FramebufferWidth;
    s32 FramebufferHeight;
    GLuint Program;
    glm::mat4 Projection;
    glm::mat4 ModelView;
    f32 CurrentDepth;
    render_batch RenderBatches[R_MODE_COUNT];
};
