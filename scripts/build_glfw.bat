@echo off
set Headers= ^
    /Iw:\extern\glfw\include

set Sources= ^
    w:\extern\glfw\src\context.c ^
    w:\extern\glfw\src\init.c ^
    w:\extern\glfw\src\input.c ^
    w:\extern\glfw\src\monitor.c ^
    w:\extern\glfw\src\monitor.c ^
    w:\extern\glfw\src\window.c ^
    w:\extern\glfw\src\vulkan.c ^
    w:\extern\glfw\src\win32_*.c ^
    w:\extern\glfw\src\wgl_context.c ^
    w:\extern\glfw\src\egl_context.c ^
    w:\extern\glfw\src\osmesa_context.c

pushd w:\build
    mkdir glfw
    pushd glfw
    cl /c /EHsc /D_GLFW_WIN32 %Sources% %Headers% /link gdi32.lib & lib /OUT:glfw3.lib *.obj
    popd
popd