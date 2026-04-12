#include "viewer_gl.h"

#include <windows.h>
#include <filesystem>
#include <exception>
#include <iostream>

static std::string getExeDir()
{
    char buf[MAX_PATH]{};
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path().string();
}

int main()
{
    try
    {
        const std::string exePath = getExeDir();
        return runViewer(exePath);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return -1;
    }
}