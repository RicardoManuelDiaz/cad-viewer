// THIS IS A HEADER-ONLY HELPER!

#pragma once
#include <GLFW/glfw3.h>
#include <algorithm>

class GlfwFullscreenController
{
    public:

        enum class Mode
        {
            Windowed,
            Borderless,
            //Exclusive
        };

    public:

        explicit GlfwFullscreenController(GLFWwindow* window)
            : m_window(window)
        {
            // Capture initial windowed state
            glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
            glfwGetWindowSize(m_window, &m_windowedW, &m_windowedH);
        }

        void Toggle()
        {
            switch (m_mode)
            {
                case Mode::Windowed:   SetMode(Mode::Borderless); break;
                case Mode::Borderless: SetMode(Mode::Windowed);  break;
                //case Mode::Borderless: SetMode(Mode::Exclusive);  break;
                //case Mode::Exclusive:  SetMode(Mode::Windowed);   break;
            }
        }

        void SetMode(Mode mode)
        {
            if (mode == m_mode)
                return;

            // Save windowed state before leaving it
            if (m_mode == Mode::Windowed)
            {
                glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
                glfwGetWindowSize(m_window, &m_windowedW, &m_windowedH);
            }

            m_mode = mode;

            switch (m_mode)
            {
                case Mode::Windowed:   ApplyWindowed();   break;
                case Mode::Borderless: ApplyBorderless(); break;
                //case Mode::Exclusive:  ApplyExclusive();  break;
            }
        }

        Mode GetMode() const { return m_mode; }

    private:

        void ApplyWindowed()
        {
            glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_TRUE);

            glfwSetWindowMonitor(
                m_window,
                nullptr,
                m_windowedX,
                m_windowedY,
                m_windowedW,
                m_windowedH,
                0
            );
        }

        void ApplyBorderless()
        {
            GLFWmonitor* monitor = GetCurrentMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);

            glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_FALSE);

            glfwSetWindowMonitor(
                m_window,
                nullptr,
                0,
                0,
                mode->width,
                mode->height,
                0
            );
        }

        void ApplyExclusive()
        {
            GLFWmonitor* monitor = GetCurrentMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);

            glfwSetWindowAttrib(m_window, GLFW_DECORATED, GLFW_TRUE);

            glfwSetWindowMonitor(
                m_window,
                monitor,
                0,
                0,
                mode->width,
                mode->height,
                mode->refreshRate
            );
        }

        GLFWmonitor* GetCurrentMonitor() const
        {
            int nmonitors;
            GLFWmonitor** monitors = glfwGetMonitors(&nmonitors);

            int wx, wy, ww, wh;
            glfwGetWindowPos(m_window, &wx, &wy);
            glfwGetWindowSize(m_window, &ww, &wh);

            GLFWmonitor* best = monitors[0];
            int bestOverlap = 0;

            for (int i = 0; i < nmonitors; ++i)
            {
                int mx, my;
                glfwGetMonitorPos(monitors[i], &mx, &my);

                const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);

                int overlap =
                    std::max(0, std::min(wx + ww, mx + mode->width) - std::max(wx, mx)) *
                    std::max(0, std::min(wy + wh, my + mode->height) - std::max(wy, my));

                if (overlap > bestOverlap)
                {
                    bestOverlap = overlap;
                    best = monitors[i];
                }
            }

            return best;
        }

    private:

        GLFWwindow* m_window;

        int m_windowedX = 0;
        int m_windowedY = 0;
        int m_windowedW = 0;
        int m_windowedH = 0;

        Mode m_mode = Mode::Windowed;
};
