## Simple 2D Batch Renderer

![Preview](assets/preview_100k_points.jpeg)
*100k Points rendererd bouncing inside the window and cursor running at 600+ fps with /O2*

This is a super simple 2D batch renderer using OpenGL 3.3.

You can build it yourself using Windows. Just set the correct path to your MSVC in the env.bat and also change the substitute directory path and run it to setup the environment.

```
@echo off
subst w: Path\To\This\Repository
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

You also need to build GLFW first before compiling the batch renderer.

```
w:\scripts\build_glfw.bat
w:\scripts\build.bat
```