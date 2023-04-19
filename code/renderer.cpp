#define GLSL(x) "#version 330 core\n" #x

global render_state RenderState;

/*
================================
Utils
================================
*/

global glm::vec4 ColorRangeZeroOne(color Color)
{
    glm::vec4 Result = glm::vec4(Color.R, Color.G, Color.B, Color.A);
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
    
    // Idices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffer.Vbos[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * Capacity, 0, Usage);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return Buffer;
}

/*
================================
Shader
================================
*/

internal GLuint CreateShader(const char* Code, GLenum Type)
{
    GLuint Shader = glCreateShader(Type);
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

global GLuint CreateProgram(GLuint VertexShader, GLuint FragmentShader)
{
    GLuint Program = glCreateProgram();
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

global GLuint CreateShaderProgram()
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

        uniform sampler2D Texture;

        void main()
        {
            FragColor = Color;
        }
    );

    GLuint VertexShader = CreateShader(VertexShaderCode, GL_VERTEX_SHADER);
    GLuint FragmentShader = CreateShader(FragmentShaderCode, GL_FRAGMENT_SHADER);
    GLuint Program = CreateProgram(VertexShader, FragmentShader);

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

internal void PushVertex(render_batch* Batch, f32* Position, f32* TexCoord, f32* Color)
{
    memcpy(Batch->Buffer.Positions + Batch->Buffer.VertexCount * 3, Position, sizeof(f32) * 3);
    memcpy(Batch->Buffer.TexCoords + Batch->Buffer.VertexCount * 2, TexCoord, sizeof(f32) * 2);
    memcpy(Batch->Buffer.Colors    + Batch->Buffer.VertexCount * 4, Color, sizeof(f32) * 4);
    Batch->Buffer.VertexCount++;
}

internal void FlushRenderBatch(render_batch* Batch)
{
    if (!Batch->Buffer.VertexCount) return;

    // Update vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, Batch->Buffer.Vbos[ATTRIB_POSITION]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(f32) * 3 * Batch->Buffer.VertexCount, (const void*)Batch->Buffer.Positions);
    
    glBindBuffer(GL_ARRAY_BUFFER, Batch->Buffer.Vbos[ATTRIB_TEXCOORD]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(f32) * 2 * Batch->Buffer.VertexCount, (const void*)Batch->Buffer.TexCoords);
    
    glBindBuffer(GL_ARRAY_BUFFER, Batch->Buffer.Vbos[ATTRIB_COLOR]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(f32) * 4 * Batch->Buffer.VertexCount, (const void*)Batch->Buffer.Colors);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Perform rendition
    glUseProgram(RenderState.Program);

    GLint ProjectionID = glGetUniformLocation(RenderState.Program, "Projection");
    GLint ModelViewID = glGetUniformLocation(RenderState.Program, "ModelView");

    glUniformMatrix4fv(ProjectionID, 1, 0, &RenderState.Projection[0][0]);
    glUniformMatrix4fv(ModelViewID, 1, 0, &RenderState.ModelView[0][0]);

    glBindVertexArray(Batch->Buffer.Vao);

    glDrawArrays(Batch->Mode, 0, Batch->Buffer.VertexCount);
    Batch->Buffer.VertexCount = 0;
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

    GLenum DrawModes[] = { GL_POINTS, GL_LINES, GL_TRIANGLES };

    for (s32 BatchIndex = 0; BatchIndex < R_MODE_COUNT; BatchIndex++)
    {
        RenderState.RenderBatches[BatchIndex].Buffer = CreateVertexBuffer(RENDER_BATCH_MAX_CAPACITY, GL_DYNAMIC_DRAW);
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

    glm::vec3 VertPosition = glm::vec3(X, Y, RenderState.CurrentDepth);
    glm::vec2 VertTexCoord = glm::vec2(0.0f, 0.0f);
    glm::vec4 VertColor = ColorRangeZeroOne(Color);
    PushVertex(RenderBatch, &VertPosition[0], &VertTexCoord[0], &VertColor[0]);
}

global void DrawLine(s32 X1, s32 Y1, s32 X2, s32 Y2, color Color)
{
    render_batch *RenderBatch = &RenderState.RenderBatches[R_LINES];

    if (RenderBatch->Buffer.VertexCount + 2 >= RenderBatch->Buffer.Capacity)
    {
        FlushRenderBatch(RenderBatch);
    }

    glm::vec3 VertPosition1 = glm::vec3(X1, Y1, RenderState.CurrentDepth);
    glm::vec3 VertPosition2 = glm::vec3(X2, Y2, RenderState.CurrentDepth);
    glm::vec2 VertTexCoord = glm::vec2(0.0f, 0.0f);
    glm::vec4 VertColor = ColorRangeZeroOne(Color);
    PushVertex(RenderBatch, &VertPosition1[0], &VertTexCoord[0], &VertColor[0]);
    PushVertex(RenderBatch, &VertPosition2[0], &VertTexCoord[0], &VertColor[0]);
}