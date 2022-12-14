#include "sdlpp/events.hpp"
#include "sdlpp/render.hpp"
#include "sdlpp/window.hpp"
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
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

struct Gear {
    auto createLocation() {

        mat4 location = identity<mat4>();
        location = translate(location, vec3{pos, 0});
        location = rotate(location, angle, {0, 0, 1});
        return location;
    }

    void draw(sdl::RendererView view) {
        auto location = createLocation();

        for (auto &line : lines) {
            auto p1 = location *vec4{line.first, 0, 1};
            auto p2 = location *vec4{line.second, 0, 1};
            view.drawLine(p1.x, p1.y, p2.x, p2.y);
        }
    }

    void addInverted(vec2 p) {
        auto inv = affineInverse(createLocation());

        auto local = inv *vec4{p, 0, 1};

        lines.emplace_back(lines.back().second, vec2{local});
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

    auto gear1 = Gear{};
    auto gear2 = Gear{};

    auto radius = 100;

    gear1.pos = {200, 200};

    auto overlap = 20;

    gear2.pos = {gear1.pos.x + radius * 2 - 20, 200};

    auto center = (gear1.pos + gear2.pos) / 2.f;

    drawArc(
        [&](auto p1, auto p2) {
            //            renderer.drawLine(p1.x, p1.y, p2.x, p2.y);
            gear1.lines.push_back({p1, p2});
            gear2.lines.push_back({p1, p2});
        },
        {0, 0},
        100,
        0,
        5);

    auto contactLength = 20.f;

    auto lineOfAction = [](float i) { return glm::vec2{i / 2.f, i}; };

    int contactPointIndex = 0;

    bool isRunning = true;

    for (; isRunning;) {
        for (auto event = std::optional<sdl::Event>{};
             (event = sdl::pollEvent());) {

            if (event->type == SDL_QUIT) {
                isRunning = false;
                break;
            }
        }
        renderer.drawColor({100, 0, 0, 255});
        renderer.clear();
        renderer.drawColor({100, 100, 100, 255});

        if (gear1.angle < pi<float>() * 2) {
            gear1.angle += .01;
            gear2.angle -= .01;
        }

        gear1.draw(renderer);
        gear2.draw(renderer);

        for (float i = -contactLength; i < contactLength; i += 1) {
            auto p1 = lineOfAction(i) + center;
            auto p2 = lineOfAction(i) + center;

            drawLine(renderer, p1, p2);
        }

        renderer.drawColor({255, 255, 255});

        contactPointIndex += 1;
        if (contactPointIndex > contactLength) {
            contactPointIndex = -contactLength;
        }

        auto contactPoint = lineOfAction(contactPointIndex) + center;

        if (gear1.angle < pi<float>() * 2) {
            gear1.addInverted(contactPoint);
            gear2.addInverted(contactPoint);
        }

        renderer.drawPoint(contactPoint);

        renderer.present();
        std::this_thread::sleep_for(10ms);
    }

    return 0;
}
