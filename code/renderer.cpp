#define GLSL(x) "#version 330 core\n" #x

global render_state RenderState;

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

    GLint A1 = glGetAttribLocation(Program, "VertPosition");
    GLint A2 = glGetAttribLocation(Program, "VertTexCoord");
    GLint A3 = glGetAttribLocation(Program, "VertColor");

    return Program;
}

global render_batch R_CreateRenderBatch(s32 MaxVertexCount)
{
    GLuint Vao;
    glGenVertexArrays(1, &Vao);
    glBindVertexArray(Vao);

    GLuint PositionVbo;
    glGenBuffers(1, &PositionVbo);
    glBindBuffer(GL_ARRAY_BUFFER, PositionVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 3 * MaxVertexCount, 0, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(ATTRIB_POSITION);
    glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    GLuint TexCoordVbo;
    glGenBuffers(1, &TexCoordVbo);
    glBindBuffer(GL_ARRAY_BUFFER, TexCoordVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 2 * MaxVertexCount, 0, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(ATTRIB_TEXCOORD);
    glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, 0);

    GLuint ColorVbo;
    glGenBuffers(1, &ColorVbo);
    glBindBuffer(GL_ARRAY_BUFFER, ColorVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 4 * MaxVertexCount, 0, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(ATTRIB_COLOR);
    glVertexAttribPointer(ATTRIB_COLOR, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    render_batch Batch = {};
    Batch.Vao = Vao;
    Batch.Vbos[ATTRIB_POSITION] = PositionVbo;
    Batch.Vbos[ATTRIB_TEXCOORD] = TexCoordVbo;
    Batch.Vbos[ATTRIB_COLOR] = ColorVbo;
    Batch.MaxVertexCount = MaxVertexCount;
    Batch.Positions = (f32*)malloc(sizeof(f32) * 3 * MaxVertexCount);
    Batch.TexCoords = (f32*)malloc(sizeof(f32) * 2 * MaxVertexCount);
    Batch.Colors = (f32*)malloc(sizeof(f32) * 4 * MaxVertexCount);
    Batch.Mode = GL_TRIANGLES;
    return Batch;
}

global void R_Flush()
{
    if (!RenderState.Batch->VertexCount) return;

    // Update vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, RenderState.Batch->Vbos[ATTRIB_POSITION]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(f32) * 3 * RenderState.Batch->VertexCount, (const void*)RenderState.Batch->Positions);
    
    glBindBuffer(GL_ARRAY_BUFFER, RenderState.Batch->Vbos[ATTRIB_TEXCOORD]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(f32) * 2 * RenderState.Batch->VertexCount, (const void*)RenderState.Batch->TexCoords);
    
    glBindBuffer(GL_ARRAY_BUFFER, RenderState.Batch->Vbos[ATTRIB_COLOR]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(f32) * 4 * RenderState.Batch->VertexCount, (const void*)RenderState.Batch->Colors);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Perform rendition
    glUseProgram(RenderState.Program);

    GLint ProjectionID = glGetUniformLocation(RenderState.Program, "Projection");
    GLint ModelViewID = glGetUniformLocation(RenderState.Program, "ModelView");

    glUniformMatrix4fv(ProjectionID, 1, 0, &RenderState.Projection[0][0]);
    glUniformMatrix4fv(ModelViewID, 1, 0, &RenderState.ModelView[0][0]);

    glBindVertexArray(RenderState.Batch->Vao);

    if (RenderState.Batch->Texture)
    {
        glBindTexture(GL_TEXTURE_2D, RenderState.Batch->Texture);
    }

    glDrawArrays(RenderState.Batch->Mode, 0, RenderState.Batch->VertexCount);
    RenderState.Batch->VertexCount = 0;
    RenderState.DrawCalls++;
}

global void R_PushVertex(glm::vec3 Position, glm::vec2 TexCoord, glm::vec4 Color)
{
    if (RenderState.Batch->VertexCount + 6 >= RenderState.Batch->MaxVertexCount - 1)
    {
        R_Flush();
    }

    memcpy(RenderState.Batch->Positions + RenderState.Batch->VertexCount * 3, &Position[0], sizeof(f32) * 3);
    memcpy(RenderState.Batch->TexCoords + RenderState.Batch->VertexCount * 2, &TexCoord[0], sizeof(f32) * 2);
    memcpy(RenderState.Batch->Colors + RenderState.Batch->VertexCount * 4, &Color[0], sizeof(f32) * 4);

    RenderState.Batch->VertexCount++;
}
