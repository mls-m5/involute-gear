#include "sdlpp/events.hpp"
#include "sdlpp/render.hpp"
#include "sdlpp/window.hpp"
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <thread>

using namespace std::literals;
using namespace glm;

using FuncT = std::function<void(glm::vec2, glm::vec2)>;

void drawLine(sdl::RendererView view, glm::vec2 p1, glm::vec2 p2) {
    view.drawLine(p1.x, p1.y, p2.x, p2.y);
}

void drawArc(
    FuncT f, glm::vec2 center, float r, float start = 0, float end = 0) {

    auto step = 1. / r;

    for (float angle = start; angle <= end; angle += step) {
        auto p1 = center + glm::vec2{sin(angle), cos(angle)} * r;
        auto p2 = center + glm::vec2{sin(angle + step), cos(angle + step)} * r;

        f(p1, p2);
    }
}

struct Gear1 {
    void draw(sdl::RendererView view) {
        mat4 location = identity<mat4>();
        location = translate(location, vec3{pos, 0});
        location = rotate(location, angle, {0, 0, 1});

        for (auto &line : lines) {
            auto p1 = location *vec4{line.first, 0, 1};
            auto p2 = location *vec4{line.second, 0, 1};
            view.drawLine(p1.x, p1.y, p2.x, p2.y);
        }
    }

    glm::vec2 pos;
    float angle = 0;

    std::vector<std::pair<glm::vec2, glm::vec2>> lines;
};

int main(int argc, char **argv) {
    auto window = sdl::Window{"sdl window",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              500,
                              500,
                              SDL_WINDOW_OPENGL};

    auto renderer = sdl::Renderer{window.get(), -1, SDL_RENDERER_ACCELERATED};

    if (!renderer) {
        std::cerr << "could not create renderer\n";
        return 1;
    }

    //    drawLine(renderer, {0, 0}, {100, 100});

    auto gear = Gear1{};

    drawArc(
        [&](auto p1, auto p2) {
            //            renderer.drawLine(p1.x, p1.y, p2.x, p2.y);
            gear.lines.push_back({p1, p2});
        },
        {0, 0},
        100,
        0,
        5);

    for (auto event = std::optional<sdl::Event>{};;
         (event = sdl::pollEvent())) {

        if (event) {
            if (event->type == SDL_QUIT) {
                break;
            }
        }
        renderer.drawColor({100, 0, 0, 255});
        renderer.clear();
        renderer.drawColor({100, 100, 100, 255});

        gear.angle += .01;

        gear.pos = {200, 200};
        gear.draw(renderer);

        renderer.present();
        std::this_thread::sleep_for(10ms);
    }

    return 0;
}
