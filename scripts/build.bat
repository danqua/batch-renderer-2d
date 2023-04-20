@echo off
set IncludeDirectories= ^
    /Iw:\code ^
    /Iw:\extern\glfw\include ^
    /Iw:\extern\glm ^
    /Iw:\extern\stb ^
    /Iw:\extern\glad\include

set LibraryDirectories= ^
    /LIBPATH:w:\build\glfw

set SourceFiles= ^
    w:\extern\glad\src\gl.c ^
    w:\code\main.cpp

pushd ..\build
cl /Zi /FC /EHsc %SourceFiles% glfw3.lib User32.lib Gdi32.lib Shell32.lib %IncludeDirectories% /Febatch.exe /link %LibraryDirectories% /SUBSYSTEM:CONSOLE
popd