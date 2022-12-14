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

void drawArc(FuncT f,
             glm::vec2 center,
             float r,
             float start = 0,
             float end = pi<float>() * 2) {

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

        for (size_t i = 1; i < points.size(); ++i) {
            auto p1 = location *vec4{points.at(i - 1), 0, 1};
            auto p2 = location *vec4{points.at(i), 0, 1};
            drawLine(view, p1, p2);
        }
    }

    void addInverted(vec2 p1) {
        auto inv = affineInverse(createLocation());
        auto local = inv *vec4{p1, 0, 1};
        points.emplace_back(vec2{local});
    }

    void repeat(int num) {
        auto tmp = points;
        for (int i = 1; i < num; ++i) {
            auto angle = pi<float>() * 2.f * i / num;
            mat4 location = identity<mat4>();
            location = rotate(location, angle, {0, 0, 1});

            for (auto p : tmp) {
                auto pt = location *vec4{p, 0, 1};
                points.push_back(pt);
            }
        }
    }

    glm::vec2 pos;
    float angle = 0;

    std::vector<glm::vec2> points;
};

struct GearSettings {
    int numGears = 20;
    float baseRadius = 100;
    float addendumRadius = baseRadius + 10;
};

int main(int argc, char **argv) {
    auto window = sdl::Window{"sdl window",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              800,
                              600,
                              SDL_WINDOW_OPENGL};

    auto renderer = sdl::Renderer{window.get(), -1, SDL_RENDERER_ACCELERATED};

    if (!renderer) {
        std::cerr << "could not create renderer\n";
        return 1;
    }

    auto gear1 = Gear{};
    auto gear2 = Gear{};

    auto settings = GearSettings{};

    gear1.pos = {200, 200};

    auto overlap = 20;

    gear2.pos = {gear1.pos.x + settings.baseRadius + settings.addendumRadius,
                 200};

    auto center = (gear1.pos + gear2.pos) / 2.f;

    auto contactLength = 30.f;

    auto lineOfAction = [contactLength](float i) {
        return glm::vec2{i / 2.f, i} * contactLength;
    };

    float contactPointIndex = 0;

    bool isRunning = true;

    for (auto amount = 0.f; amount <= 1.; amount += 1. / 100.) {
        auto angle = pi<float>() * 2 / settings.numGears * amount;
        gear1.angle = angle;
        gear2.angle = -angle;

        gear1.addInverted(center + lineOfAction(amount));
        gear2.addInverted(center + lineOfAction(amount));
    }

    gear1.repeat(settings.numGears);
    gear2.repeat(settings.numGears);

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

        gear1.angle += .001;
        gear2.angle -= .001;

        gear1.draw(renderer);
        gear2.draw(renderer);

        drawArc(
            [&](auto p1, auto p2) {
                drawLine(renderer, p1 + gear1.pos, p2 + gear1.pos);
                drawLine(renderer, p1 + gear2.pos, p2 + gear2.pos);
            },
            {0, 0},
            settings.baseRadius);

        drawArc(
            [&](auto p1, auto p2) {
                drawLine(renderer, p1 + gear1.pos, p2 + gear1.pos);
                drawLine(renderer, p1 + gear2.pos, p2 + gear2.pos);
            },
            {0, 0},
            settings.addendumRadius);

        for (float i = 0; i < 1; i += .1) {
            auto p1 = lineOfAction(i) + center;
            auto p2 = lineOfAction(i) + center;

            drawLine(renderer, p1, p2);
        }

        renderer.drawColor({255, 255, 255});

        contactPointIndex += .1;
        if (contactPointIndex > 1) {
            contactPointIndex = 0;
        }

        auto contactPoint = lineOfAction(contactPointIndex) + center;

        //        renderer.drawPoint(contactPoint);

        renderer.present();
        std::this_thread::sleep_for(10ms);
    }

    return 0;
}
