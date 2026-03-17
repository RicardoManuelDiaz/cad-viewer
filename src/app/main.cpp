#include "viewer_gl.h"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        return runViewer();
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return -1;
    }
}