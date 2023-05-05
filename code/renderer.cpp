#define GLSL(x) "#version 330 core\n" #x

global render_state RenderState;

/*
================================
Utils
================================
*/

global vec4 ColorRangeZeroOne(color Color)
{
    vec4 Result = vec4(Color.R, Color.G, Color.B, Color.A);
    Result /= 255.0f;
    return Result;
}

/*
================================
Vertex Buffer
================================
*/

internal vertex_buffer CreateVertexBuffer(u64 Capacity, GLenum Usage)
{
    vertex_buffer Buffer = {};
    Buffer.Capacity = Capacity;
    Buffer.Usage = Usage;
    Buffer.Positions = (f32*)malloc(sizeof(f32) * 3 * Capacity);
    Buffer.TexCoords = (f32*)malloc(sizeof(f32) * 2 * Capacity);
    Buffer.Colors = (f32*)malloc(sizeof(f32) * 4 * Capacity);
    Buffer.Indices = (u32*)malloc(sizeof(u32) * Capacity);

    glGenBuffers(4, Buffer.Vbos);
    glGenVertexArrays(1, &Buffer.Vao);
    glBindVertexArray(Buffer.Vao);

    // Positions
    glBindBuffer(GL_ARRAY_BUFFER, Buffer.Vbos[ATTRIB_POSITION]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 3 * Capacity, 0, Usage);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // TexCoords
    glBindBuffer(GL_ARRAY_BUFFER, Buffer.Vbos[ATTRIB_TEXCOORD]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 2 * Capacity, 0, Usage);
    glEnableVertexAttribArray(ATTRIB_TEXCOORD);
    glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    // Colors
    glBindBuffer(GL_ARRAY_BUFFER, Buffer.Vbos[ATTRIB_COLOR]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 4 * Capacity, 0, Usage);
    glEnableVertexAttribArray(ATTRIB_COLOR);
    glVertexAttribPointer(ATTRIB_COLOR, 4, GL_FLOAT, GL_FALSE, 0, 0);
    
    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffer.Vbos[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * 6 * Capacity, 0, Usage);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return Buffer;
}

/*
================================
Texture
================================
*/
global texture LoadTexture(const char* Filename)
{
    s32 Width = 0;
    s32 Height = 0;
    s32 Ignore = 0;

    u8* Pixels = stbi_load(Filename, &Width, &Height, &Ignore, STBI_rgb_alpha);

    u32 Texture;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(Pixels);

    texture Result = {};
    Result.Width = Width;
    Result.Height = Height;
    Result.Handle = Texture;
    return Result;
}

/*
================================
Shader
================================
*/

internal u32 CreateShader(const char* Code, GLenum Type)
{
    u32 Shader = glCreateShader(Type);
    glShaderSource(Shader, 1, &Code, 0);
    glCompileShader(Shader);

    GLint Compiled = GL_FALSE;
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &Compiled);

    if (!Compiled)
    {
        char Message[4096];
        s32 Ignore;
        glGetShaderInfoLog(Shader, 4096, &Ignore, Message);
        printf("%s\n", Message);
        return 0;
    }
    return Shader;
}

global u32 CreateProgram(u32 VertexShader, u32 FragmentShader)
{
    u32 Program = glCreateProgram();
    glAttachShader(Program, VertexShader);
    glAttachShader(Program, FragmentShader);
    glLinkProgram(Program);

    GLint Linked = GL_FALSE;
    glGetProgramiv(Program, GL_LINK_STATUS, &Linked);

    if (!Linked)
    {
        char Message[4096];
        s32 Ignore;
        glGetProgramInfoLog(Program, 4096, &Ignore, Message);
        printf("%s\n", Message);
        return 0;
    }
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);

    return Program;
}

global u32 CreateShaderProgram()
{
    const char* VertexShaderCode = GLSL(
        in vec3 VertPosition;
        in vec2 VertTexCoord;
        in vec4 VertColor;

        out vec4 Color;
        out vec2 TexCoord;

        uniform mat4 Projection;
        uniform mat4 ModelView;

        void main()
        {
            gl_Position = Projection * ModelView * vec4(VertPosition, 1.0);
            Color = VertColor;
            TexCoord = VertTexCoord;
        }
    );

    const char* FragmentShaderCode = GLSL(
        in vec4 Color;
        in vec2 TexCoord;
        out vec4 FragColor;

        uniform float HasTexture;
        uniform sampler2D Texture;

        void main()
        {
            FragColor = mix(Color, texture(Texture, TexCoord) * Color, HasTexture);
            //FragColor = vec4(1.0, 0.0, HasTexture, 1.0);
            //FragColor = vec4(1.0);
        }
    );

    u32 VertexShader = CreateShader(VertexShaderCode, GL_VERTEX_SHADER);
    u32 FragmentShader = CreateShader(FragmentShaderCode, GL_FRAGMENT_SHADER);
    u32 Program = CreateProgram(VertexShader, FragmentShader);

    glBindAttribLocation(Program, ATTRIB_POSITION, "VertPosition");
    glBindAttribLocation(Program, ATTRIB_TEXCOORD, "VertTexCoord");
    glBindAttribLocation(Program, ATTRIB_COLOR,    "VertColor");

    return Program;
}

/*
================================
Render Batch
================================
*/

internal void PushVertex(render_batch* Batch, const vec3& Position, const vec2& TexCoord, const vec4& Color)
{
    memcpy(Batch->Buffer.Positions + Batch->Buffer.VertexCount * 3, &Position[0], sizeof(vec3));
    memcpy(Batch->Buffer.TexCoords + Batch->Buffer.VertexCount * 2, &TexCoord[0], sizeof(vec2));
    memcpy(Batch->Buffer.Colors + Batch->Buffer.VertexCount * 4, &Color[0], sizeof(vec4));
    Batch->Buffer.VertexCount++;
}

internal void PushVertex(render_batch* Batch, f32* Position, f32* TexCoord, f32* Color)
{
    memcpy(Batch->Buffer.Positions + Batch->Buffer.VertexCount * 3, Position, sizeof(f32) * 3);
    memcpy(Batch->Buffer.TexCoords + Batch->Buffer.VertexCount * 2, TexCoord, sizeof(f32) * 2);
    memcpy(Batch->Buffer.Colors + Batch->Buffer.VertexCount * 4, Color, sizeof(f32) * 4);
    Batch->Buffer.VertexCount++;
}

internal void PushVertex(render_batch* Batch, f32* Position, f32* Color)
{
    memcpy(Batch->Buffer.Positions + Batch->Buffer.VertexCount * 3, Position, sizeof(f32) * 3);
    memcpy(Batch->Buffer.Colors    + Batch->Buffer.VertexCount * 4, Color, sizeof(f32) * 4);
    Batch->Buffer.VertexCount++;
}

internal void PushIndex(render_batch* Batch, u32* Indices, u64 Count)
{
    memcpy(Batch->Buffer.Indices + Batch->Buffer.ElementCount, Indices, sizeof(u32) * Count);
    Batch->Buffer.ElementCount += Count;
}

internal void FlushRenderBatch(render_batch* Batch)
{
    if (!Batch->Buffer.VertexCount) return;

    struct buffer_layout
    {
        u64 Size;
        f32* Data;
    };

    buffer_layout Layout[] = {
        { 3, Batch->Buffer.Positions },
        { 2, Batch->Buffer.TexCoords },
        { 4, Batch->Buffer.Colors },
    };

    // Update vertex buffers
    for (s32 i = 0; i < ATTRIB_COUNT; i++)
    {
        glBindBuffer(GL_ARRAY_BUFFER, Batch->Buffer.Vbos[i]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(f32) * Layout[i].Size * Batch->Buffer.VertexCount, (const void*)Layout[i].Data);
    }

    // Update index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Batch->Buffer.Vbos[3]);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(u32) * Batch->Buffer.ElementCount, (const void*)Batch->Buffer.Indices);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Perform rendition
    glUseProgram(RenderState.Program);

    GLint ProjectionID = glGetUniformLocation(RenderState.Program, "Projection");
    GLint ModelViewID = glGetUniformLocation(RenderState.Program, "ModelView");
    GLint HasSamplerID = glGetUniformLocation(RenderState.Program, "HasTexture");

    glUniformMatrix4fv(ProjectionID, 1, 0, &RenderState.Projection[0][0]);
    glUniformMatrix4fv(ModelViewID, 1, 0, &RenderState.ModelView[0][0]);
    glUniform1f(HasSamplerID, (f32)(Batch->Texture != 0));

    glBindTexture(GL_TEXTURE_2D, Batch->Texture);

    glBindVertexArray(Batch->Buffer.Vao);

    if (Batch->Mode == GL_TRIANGLES)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Batch->Buffer.Vbos[3]);
        glDrawElements(Batch->Mode, Batch->Buffer.ElementCount, GL_UNSIGNED_INT, 0);
    }
    else
    {
        glDrawArrays(Batch->Mode, 0, Batch->Buffer.VertexCount);
    }

    Batch->Buffer.VertexCount = 0;
    Batch->Buffer.ElementCount = 0;
}

/*
================================
Renderer
================================
*/

global void InitRenderer(s32 Width, s32 Height)
{
    RenderState.Program = CreateShaderProgram();
    RenderState.FramebufferWidth = Width;
    RenderState.FramebufferHeight = Height;

    GLenum DrawModes[] = { GL_POINTS, GL_LINES, GL_TRIANGLES, GL_TRIANGLES };

    for (s32 BatchIndex = 0; BatchIndex < R_MODE_COUNT; BatchIndex++)
    {
        RenderState.RenderBatches[BatchIndex].Buffer = CreateVertexBuffer(RENDER_BATCH_MAX_CAPACITY, GL_STREAM_DRAW);
        RenderState.RenderBatches[BatchIndex].Mode = DrawModes[BatchIndex];
    }
}

global void BeginFrame()
{
    RenderState.Projection = glm::ortho(0.0f,
        (f32)RenderState.FramebufferWidth,
        (f32)RenderState.FramebufferHeight,
        0.0f, -1.0f, 1.0f);
    RenderState.ModelView = glm::mat4(1.0f);
}

global void EndFrame()
{
    for (s32 BatchIndex = 0; BatchIndex < R_MODE_COUNT; ++BatchIndex)
    {
        FlushRenderBatch(RenderState.RenderBatches + BatchIndex);
    }
}

global void ClearScreen(color Color)
{
    glClear(GL_COLOR_BUFFER_BIT);
}

global void DrawPoint(s32 X, s32 Y, color Color)
{
    render_batch *RenderBatch = &RenderState.RenderBatches[R_POINTS];
    if (RenderBatch->Buffer.VertexCount + 1 >= RenderBatch->Buffer.Capacity)
    {
        FlushRenderBatch(RenderBatch);
    }
    vec3 Position = glm::vec3(X, Y, RenderState.CurrentDepth);
    vec2 TexCoord = glm::vec2(0.0f, 0.0f);
    PushVertex(RenderBatch, Position, TexCoord, ColorRangeZeroOne(Color));
}

global void DrawLine(s32 X1, s32 Y1, s32 X2, s32 Y2, color Color)
{
    render_batch *RenderBatch = &RenderState.RenderBatches[R_LINES];
    if (RenderBatch->Buffer.VertexCount + 2 >= RenderBatch->Buffer.Capacity)
    {
        FlushRenderBatch(RenderBatch);
    }
    vec3 V1 = vec3(X1, Y1, RenderState.CurrentDepth);
    vec3 V2 = vec3(X2, Y2, RenderState.CurrentDepth);
    vec2 TexCoord = vec2(0.0f, 0.0f);
    PushVertex(RenderBatch, V1, TexCoord, ColorRangeZeroOne(Color));
    PushVertex(RenderBatch, V2, TexCoord, ColorRangeZeroOne(Color));
}

global void DrawRectLines(s32 X, s32 Y, s32 Width, s32 Height, color Color)
{
    render_batch* RenderBatch = &RenderState.RenderBatches[R_LINES];
    if (RenderBatch->Buffer.VertexCount + 8 >= RenderBatch->Buffer.Capacity)
    {
        FlushRenderBatch(RenderBatch);
    }

    ivec2 Factors[] = {
        { 0, 0 }, { 1, 0 },
        { 1, 0 }, { 1, 1 },
        { 1, 1 }, { 0, 1 },
        { 0, 1 }, { 0, 0 }
    };

    for (s32 FactorIndex = 0; FactorIndex < 8; FactorIndex += 2)
    {
        s32 X1 = X + Width  * Factors[FactorIndex + 0][0];
        s32 Y1 = Y + Height * Factors[FactorIndex + 0][1];
        s32 X2 = X + Width  * Factors[FactorIndex + 1][0];
        s32 Y2 = Y + Height * Factors[FactorIndex + 1][1];

        DrawLine(X1, Y1, X2, Y2, Color);
    }
}

global void DrawRect(s32 X, s32 Y, s32 Width, s32 Height, color Color)
{
    render_batch* RenderBatch = &RenderState.RenderBatches[R_TRIANGLES];

    if (RenderBatch->Buffer.VertexCount + 4 >= RenderBatch->Buffer.Capacity)
    {
        FlushRenderBatch(RenderBatch);
    }

    vec3 Vertices[] = {
        vec3(X        , Y         , RenderState.CurrentDepth),
        vec3(X        , Y + Height, RenderState.CurrentDepth),
        vec3(X + Width, Y + Height, RenderState.CurrentDepth),
        vec3(X + Width, Y         , RenderState.CurrentDepth)
    };

    vec2 TexCoord[] = {
        { 0.0f, 0.0f },
        { 0.0f, 1.0f },
        { 1.0f, 1.0f },
        { 1.0f, 0.0f }
    };

    vec4 VertColor = ColorRangeZeroOne(Color);

    u32 Indices[] = {
        RenderBatch->Buffer.VertexCount + 0,
        RenderBatch->Buffer.VertexCount + 1,
        RenderBatch->Buffer.VertexCount + 2,
        RenderBatch->Buffer.VertexCount + 0,
        RenderBatch->Buffer.VertexCount + 2,
        RenderBatch->Buffer.VertexCount + 3
    };

    for (s32 i = 0; i < 4; i++)
    {
        PushVertex(RenderBatch, Vertices[i], TexCoord[i], VertColor);
    }

    PushIndex(RenderBatch, Indices, 6);
}

global void DrawTexture(texture* Texture, const rect& SrcRect, const rect& DstRect, color Color)
{
    render_batch* RenderBatch = &RenderState.RenderBatches[R_TEXTURES];

    if (RenderBatch->Buffer.VertexCount + 4 >= RenderBatch->Buffer.Capacity)
    {
        FlushRenderBatch(RenderBatch);
    }

    vec3 Vertices[] = {
        vec3(DstRect.X                , DstRect.Y                 , RenderState.CurrentDepth),
        vec3(DstRect.X                , DstRect.Y + DstRect.Height, RenderState.CurrentDepth),
        vec3(DstRect.X + DstRect.Width, DstRect.Y + DstRect.Height, RenderState.CurrentDepth),
        vec3(DstRect.X + DstRect.Width, DstRect.Y                 , RenderState.CurrentDepth)
    };

    f32 U = (f32)SrcRect.X / Texture->Width;
    f32 V = (f32)SrcRect.Y / Texture->Height;
    f32 W = (f32)SrcRect.Width / Texture->Width;
    f32 H = (f32)SrcRect.Height / Texture->Height;

    vec2 TexCoord[4] = {
        { U    , V     },        
        { U    , V + H },        
        { U + W, V + H },        
        { U + W, V     }
    };

    vec4 VertColor = ColorRangeZeroOne(Color);

    u32 Indices[] = {
        RenderBatch->Buffer.VertexCount + 0,
        RenderBatch->Buffer.VertexCount + 1,
        RenderBatch->Buffer.VertexCount + 2,
        RenderBatch->Buffer.VertexCount + 0,
        RenderBatch->Buffer.VertexCount + 2,
        RenderBatch->Buffer.VertexCount + 3
    };

    for (s32 i = 0; i < 4; i++)
    {
        PushVertex(RenderBatch, Vertices[i], TexCoord[i], VertColor);
    }

    PushIndex(RenderBatch, Indices, 6);

    RenderBatch->Texture = Texture->Handle;
}