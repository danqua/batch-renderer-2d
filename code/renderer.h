#pragma once

#define GLSL(x) "#version 330 core\n" #x

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
    u32 *Indices;

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