#pragma once
#include "imgui.h"
#include "implot.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

class ImGuiApp
{
public:
    ImGuiApp(const char* title, int width, int height)
    {
        if (!glfwInit())
            throw std::runtime_error("glfwInit failed");

        m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!m_window)
        {
            glfwTerminate();
            throw std::runtime_error("glfwCreateWindow failed");
        }

        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1);

        ImGui::CreateContext();
        ImPlot::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 22.0f);

        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init("#version 130");
    }

    ~ImGuiApp()
    {
        ImPlot::DestroyContext();      // 1. уничтожаем ImPlot
        ImGui_ImplOpenGL3_Shutdown();  // 2. отключаем ImGui от OpenGL
        ImGui_ImplGlfw_Shutdown();     // 3. отключаем ImGui от GLFW
        ImGui::DestroyContext();       // 4. уничтожаем ImGui
        glfwDestroyWindow(m_window);   // 5. уничтожаем окно
        glfwTerminate();               // 6. завершаем GLFW
    }

    void beginFrame()
    {
        glfwPollEvents();

        int w, h;
        glfwGetFramebufferSize(m_window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void endFrame()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
    }

    bool shouldClose() const
    {
        return glfwWindowShouldClose(m_window);
    }

private:
    GLFWwindow* m_window = nullptr;
};