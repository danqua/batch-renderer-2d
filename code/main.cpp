#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>

#define internal static
#define global static

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#include "renderer.h"

#include "renderer.cpp"

global s32 WindowWidth = 640;
global s32 WindowHeight = 480;

struct rect
{
    s32 X;
    s32 Y;
    s32 Width;
    s32 Height;
};

struct color
{
    u8 R;
    u8 G;
    u8 B;
    u8 A;
};

struct texture
{
    GLuint ID;
    s32 Width;
    s32 Height;
};

GLuint LoadTexture(const char* Filename)
{
    s32 Width = 0;
    s32 Height = 0;
    s32 Ignore = 0;

    u8* Pixels = stbi_load(Filename, &Width, &Height, &Ignore, STBI_rgb_alpha);

    GLuint Texture;
    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(Pixels);
    return Texture;
}

glm::vec4 ColorRangeZeroOne(color Color)
{
    glm::vec4 Result = glm::vec4(Color.R, Color.G, Color.B, Color.A);
    Result /= 255.0f;
    return Result;
}

void DrawRect(s32 X, s32 Y, s32 W, s32 H, color Color)
{
    glm::vec4 RangedColor = ColorRangeZeroOne(Color);
    
    glm::vec3 Position1 = glm::vec3(X    , Y    , 0);
    glm::vec3 Position2 = glm::vec3(X    , Y + H, 0);
    glm::vec3 Position3 = glm::vec3(X + W, Y + H, 0);
    glm::vec3 Position4 = glm::vec3(X + W, Y    , 0);

    glm::vec2 UV1 = glm::vec2(0.0f, 0.0f);
    glm::vec2 UV2 = glm::vec2(0.0f, 1.0f);
    glm::vec2 UV3 = glm::vec2(1.0f, 1.0f);
    glm::vec2 UV4 = glm::vec2(1.0f, 0.0f);

    R_PushVertex(Position1, UV1, RangedColor);
    R_PushVertex(Position2, UV2, RangedColor);
    R_PushVertex(Position3, UV3, RangedColor);
    R_PushVertex(Position1, UV1, RangedColor);
    R_PushVertex(Position3, UV3, RangedColor);
    R_PushVertex(Position4, UV4, RangedColor);
}

struct moving_rect
{
    glm::vec2 Position;
    glm::vec2 Dir;
    glm::vec4 Color;
    f32 Scale;
    f32 Speed;

};

internal f32 GenerateRandomNumber()
{
    f32 Result = (f32)rand() / RAND_MAX;
    return Result;
}

internal void UpdateMovingRects(moving_rect* Rects, s32 RectCount, f32 DeltaTime)
{
    for (s32 i = 0; i < RectCount; i++)
    {
        Rects[i].Position += Rects[i].Dir * Rects[i].Speed * DeltaTime;

        f32 WorldWidth = (WindowWidth / 32.0f);
        f32 WorldHeight = (WindowHeight / 32.0f);
        if (Rects[i].Position.x < 0)
        {
            Rects[i].Position.x = 0.0f;
            Rects[i].Dir.x *= -1.0f;
        }
        else if (Rects[i].Position.x + Rects[i].Scale > WorldWidth)
        {
            
            Rects[i].Position.x = WorldWidth - Rects[i].Scale;
            Rects[i].Dir.x *= -1.0f;
        }

        if (Rects[i].Position.y < 0)
        {
            Rects[i].Position.y = 0.0f;
            Rects[i].Dir.y *= -1.0f;
        }
        else if (Rects[i].Position.y + Rects[i].Scale > WorldHeight)
        {
            Rects[i].Position.y = WorldHeight - Rects[i].Scale;
            Rects[i].Dir.y *= -1.0f;
        }
    }
}

internal void DrawMovingRects(moving_rect* Rects, s32 RectCount)
{
    for (s32 i = 0; i < RectCount; i++)
    {
        DrawRect(
            (s32)(Rects[i].Position.x * 32.0f),
            (s32)(Rects[i].Position.y * 32.0f),
            (s32)(Rects[i].Scale * 32.0f),
            (s32)(Rects[i].Scale * 32.0f),
            color{
                (u8)(Rects[i].Color.r * 255.0f),
                (u8)(Rects[i].Color.g * 255.0f),
                (u8)(Rects[i].Color.b * 255.0f),
                (u8)(Rects[i].Color.a * 255.0f)
            }
        );
    }
}



int main(int Argc, char* Argv)
{
    srand(time(NULL));

    GLFWwindow* Window;

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);

    Window = glfwCreateWindow(WindowWidth, WindowHeight, "Hello World", NULL, NULL);

    if (!Window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(Window);
    gladLoadGL(glfwGetProcAddress);

    render_batch RenderBatch = R_CreateRenderBatch(1024);
    RenderBatch.Texture = LoadTexture("tileset.png");
    RenderState.Program = CreateShaderProgram();
    RenderState.Batch = &RenderBatch;
    RenderState.Projection = glm::ortho(0.0f, (f32)WindowWidth, (f32)WindowHeight, 0.0f, -1.0f, 1.0f);
    RenderState.ModelView = glm::mat4(1.0f);
    RenderState.DrawCalls = 0;

    s32 MovingRectsCount = 1000;
    moving_rect *MovingRects = (moving_rect*)malloc(sizeof *MovingRects * MovingRectsCount);

    for (s32 i = 0; i < MovingRectsCount; i++)
    {
        f32 SignX = GenerateRandomNumber();
        f32 SignY = GenerateRandomNumber();
        SignX = (SignX > 0.5f) ? SignX = 1.0f : SignX = -1.0f;
        SignY = (SignY > 0.5f) ? SignY = 1.0f : SignY = -1.0f;

        f32 RandX = GenerateRandomNumber() * SignX;
        f32 RandY = GenerateRandomNumber() * SignY;
        MovingRects[i].Dir = glm::normalize(glm::vec2(RandX, RandY));

        MovingRects[i].Position.x = GenerateRandomNumber() * (WindowWidth / 32.0f);
        MovingRects[i].Position.y = GenerateRandomNumber() * (WindowHeight / 32.0f);
        MovingRects[i].Color = glm::vec4(1.0f);
        MovingRects[i].Scale = 0.5f;
        MovingRects[i].Speed = 10.0f;
    }


    f64 LastTime = glfwGetTime();
    s32 FramesPerSecond = 0;
    s32 NumFrames = 0;
    while (!glfwWindowShouldClose(Window))
    {
        f64 ElapsedTime = glfwGetTime() - LastTime;
        f64 DeltaTime = ElapsedTime;
        LastTime = glfwGetTime();
        
        UpdateMovingRects(MovingRects, MovingRectsCount, DeltaTime);
        
        RenderState.DrawCalls = 0;
        glClear(GL_COLOR_BUFFER_BIT);
        DrawMovingRects(MovingRects, MovingRectsCount);
        R_Flush();
        
        static f32 TotalTime;
        TotalTime += DeltaTime;
        FramesPerSecond += (s32)(1.0 / DeltaTime);
        NumFrames++;

        if (TotalTime > 1.0f)
        {
            printf("DRAW CALLS = %d, FPS = %d\n", RenderState.DrawCalls, FramesPerSecond / NumFrames);
            TotalTime = 0.0f;
            FramesPerSecond = 0;
            NumFrames = 0;
        }

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}