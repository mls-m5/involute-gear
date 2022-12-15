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
    float scale = 1.5;
    view.drawLine(p1.x * scale, p1.y * scale, p2.x * scale, p2.y * scale);
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

void drawArc(sdl::RendererView view,
             glm::vec2 center,
             float r,
             float start = 0,
             float end = pi<float>() * 2) {
    drawArc([view](auto p1, auto p2) { drawLine(view, p1, p2); },
            center,
            r,
            start,
            end);
}

struct GearProfile {
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

    void rotatePoints(float angle) {
        mat4 location = identity<mat4>();
        location = rotate(location, angle, {0, 0, 1});
        for (auto &p : points) {
            p = location *vec4{p, 0, 1};
        }
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

    void mirror() {
        auto endSize = points.size();

        auto newPoints = decltype(points){};
        for (auto p : points) {
            newPoints.push_back({p.x, -p.y});
        }
        reverse();
        points.insert(points.begin(), newPoints.begin(), newPoints.end());
    }

    void reverse() {
        std::reverse(points.begin(), points.end());
    }

    glm::vec2 pos;
    float angle = 0;

    std::vector<glm::vec2> points;
};

struct GearSettings {

    GearSettings(float referenceDiameter,
                 int numGears,
                 float gearDepth = 20,
                 float preassureAngle = 20.f / 180.f * pi<float>())
        : referenceDiameter{referenceDiameter}
        , gearDepth{gearDepth}
        , preassureAngle{preassureAngle}
        , numGears{numGears} {}

    float referenceDiameter;
    float gearDepth;
    // Between 14 and 20 deg according to google
    float preassureAngle;

    float rootDiameter = referenceDiameter - gearDepth;
    float tipDiameter = referenceDiameter + gearDepth;

    int numGears = 20;
    //    float baseRadius = 100;
    //    float addendumRadius = baseRadius + 10;
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

    auto gear1 = GearProfile{};
    auto gear2 = GearProfile{};
    auto gear3 = GearProfile{};

    auto settings = GearSettings{200, 20};

    gear1.pos = {200, 200};
    gear2.pos = {gear1.pos.x + settings.referenceDiameter, 200};
    gear3.pos = {gear1.pos.x + settings.referenceDiameter, 200};

    auto center = (gear1.pos + gear2.pos) / 2.f;

    auto contactLength = 30.f;

    auto lineOfAction = [contactLength](float i) {
        auto a = i;
        return glm::vec2{(a - .5) * 1, (a - .5) * 1} * contactLength;
    };

    float contactPointIndex = 0;

    bool isRunning = true;

    auto amountToAngle = pi<float>() * 2. / (settings.numGears) * 1.4;

    //    gear1.points.push_back({settings.referenceDiameter / 2, 0});

    for (auto amount = 0.f; amount <= 1.; amount += 1. / 100.) {
        auto angle = amountToAngle * amount;
        gear1.angle = angle;
        gear2.angle = -angle;
        gear3.angle = 0;
        gear3.pos =
            gear2.pos + vec2{0, angle * settings.referenceDiameter / 2.};

        gear1.addInverted(center + lineOfAction(amount));
        gear2.addInverted(center + lineOfAction(amount));
        gear3.addInverted(center + lineOfAction(amount));
    }

    renderer.scale(1, 1);

    gear1.rotatePoints(pi<float>() * 2 / settings.numGears);
    gear1.mirror();
    gear1.repeat(settings.numGears);
    gear2.rotatePoints(-pi<float>() * 2 / settings.numGears);

    //    gear1.mirror();
    //    gear1.repeat(settings.numGears);
    //    gear2.repeat(settings.numGears);

    for (; isRunning;) {
        for (auto event = std::optional<sdl::Event>{};
             (event = sdl::pollEvent());) {

            if (event->type == SDL_QUIT) {
                isRunning = false;
                break;
            }
            if (event->type == SDL_MOUSEMOTION) {
                gear1.angle = 1. / 100. * event->motion.y - 1.;
                gear2.angle = -gear1.angle;

                gear3.pos =
                    gear2.pos +
                    vec2{0, gear1.angle * settings.referenceDiameter / 2.};

                window.title(std::to_string(gear1.angle).c_str());
            }
        }
        renderer.drawColor({100, 0, 0, 255});
        renderer.clear();
        renderer.drawColor({100, 100, 100, 255});

        //        gear1.angle += .001;
        //        gear2.angle -= .001;

        drawArc(renderer, gear1.pos, settings.referenceDiameter / 2);
        drawArc(renderer, gear2.pos, settings.referenceDiameter / 2);
        drawArc(renderer, gear1.pos, settings.rootDiameter / 2);
        drawArc(renderer, gear2.pos, settings.rootDiameter / 2);
        drawArc(renderer, gear1.pos, settings.tipDiameter / 2);
        drawArc(renderer, gear2.pos, settings.tipDiameter / 2);

        renderer.drawColor({200, 200, 200});
        gear1.draw(renderer);
        gear2.draw(renderer);
        gear3.draw(renderer);

        for (float i = 0; i < 1; i += .1) {
            auto p1 = lineOfAction(i) + center;
            auto p2 = lineOfAction(i) + center;

            drawLine(renderer, p1, p2);
        }

        renderer.drawColor({255, 255, 255});

        auto contactPoint = lineOfAction(gear1.angle / amountToAngle) + center;
        contactPoint.x = center.x;

        drawLine(renderer,
                 contactPoint + vec2{-100, 0},
                 contactPoint + vec2{100, 0});

        drawLine(renderer,
                 gear1.pos,
                 gear1.createLocation() *
                     vec4{settings.referenceDiameter / 2, 0, 0, 1});

        renderer.present();
        std::this_thread::sleep_for(10ms);
    }

    return 0;
}
