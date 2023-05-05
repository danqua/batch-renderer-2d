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

typedef glm::ivec2 ivec2;

typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef glm::mat4 mat4;

#include "renderer.h"

#include "renderer.cpp"

global s32 WindowWidth = 1280;
global s32 WindowHeight = 720;
global s32 MouseX = 0;
global s32 MouseY = 0;

struct moving_rect
{
    vec2 Position;
    vec2 Dir;
    vec4 Color;
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
        f32 WorldMouseX = (MouseX / 32.0f);
        f32 WorldMouseY = (MouseY / 32.0f);

        vec2 MousePosition = vec2(WorldMouseX, WorldMouseY);
        vec2 LineMouseRect = Rects[i].Position - MousePosition;
        f32 MaxDistance = 2.0f;

        f32 Distance2 = glm::dot(LineMouseRect, LineMouseRect);

        if (Distance2 < MaxDistance*MaxDistance)
        {
            glm::vec2 Dir = glm::normalize(LineMouseRect);
            Rects[i].Dir = Dir;
        }

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
        DrawPoint(
            (s32)(Rects[i].Position.x * 32.0f),
            (s32)(Rects[i].Position.y * 32.0f),
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

    Window = glfwCreateWindow(WindowWidth, WindowHeight, "Batch Renderer 2D - OpenGL 3.3", NULL, NULL);

    if (!Window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(Window);
    glfwSwapInterval(0);
    gladLoadGL(glfwGetProcAddress);


    glfwSetCursorPosCallback(Window, [](GLFWwindow* Widow, double PosX, double PosY) {
        MouseX = (s32)PosX;
        MouseY = (s32)PosY;
    });

    InitRenderer(WindowWidth, WindowHeight);

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
        MovingRects[i].Color = glm::vec4(
            glm::clamp(GenerateRandomNumber(), 0.7f, 1.0f),
            glm::clamp(GenerateRandomNumber(), 0.7f, 1.0f),
            glm::clamp(GenerateRandomNumber(), 0.7f, 1.0f),
            1.0f);
        MovingRects[i].Scale = 0.25f;
        MovingRects[i].Speed = 10.0f;
    }

    texture Texture = LoadTexture("sprite.png");

    f64 LastTime = glfwGetTime();
    f64 Timer = 0;

    s32 FramesPerSecond = 0;
    s32 NumFrames = 0;

    while (!glfwWindowShouldClose(Window))
    {
        f64 ElapsedTime = glfwGetTime() - LastTime;
        f64 DeltaTime = ElapsedTime;
        LastTime = glfwGetTime();
        Timer += DeltaTime;
        NumFrames++;
        FramesPerSecond += (s32)(1.0 / DeltaTime);

        if (Timer > 1.0f)
        {
            FramesPerSecond /= NumFrames;
            printf("%d\n", FramesPerSecond);
            Timer = 0;
            NumFrames = 0;
            FramesPerSecond = 0;
        }

        UpdateMovingRects(MovingRects, MovingRectsCount, DeltaTime);
                
        BeginFrame();
        ClearScreen(COLOR_BLACK);

        rect SrcRect = { 0, 0, Texture.Width / 2, Texture.Height / 2 };
        rect DstRect = { 32, 32, 32, 32 };
        DrawTexture(&Texture, SrcRect, DstRect, COLOR_WHITE);
        //DrawRect(0, 0, 32, 32, COLOR_WHITE);

        //DrawMovingRects(MovingRects, MovingRectsCount);
        EndFrame();
        
        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}