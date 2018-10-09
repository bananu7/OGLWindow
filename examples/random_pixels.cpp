#include <iostream>
#include <stdexcept>
#include <vector>
#include <array>
#include <cstdint>

#include "OpenGLWindow.hpp"
#include <GL/GL.h>

int main() {
    try {
        auto params = oglw::OpenGLWindowParams{};
        params.width = 800;
        params.height = 600;
        oglw::Window win(params);

        win.keydownCallback = [&win](oglw::KeyInfo const& k) { printf("lol"); };

        const std::array<uint8_t, 3> color { 255, 0, 0 };
        std::vector<std::pair<int, int>> pixels; 

        for (int i = 0; i < 500; ++i) {
            const int x = rand() / (double)RAND_MAX * win.getSizeX();
            const int y = rand() / (double)RAND_MAX * win.getSizeY();
            pixels.push_back({ x, y });
        }

        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, 800, 600, 0, -1, 1);

        win.displayFunc = [&]() {
            for (auto const& pixel : pixels) {
                glRasterPos2i(pixel.first, pixel.second);
                glDrawPixels(1, 1, GL_RGB, GL_UNSIGNED_BYTE, color.data());
            }
        };

        while (win.display(), win.process());
    }
    catch (const std::exception& e) {
        std::cerr << e.what();
    }
}