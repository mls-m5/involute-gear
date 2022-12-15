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

    // Input parameters
    int numTeeth;
    int module = 1;
    float preassureAngle = 20; // Degrees 20 is standard for most gears

    // Results. Don't set these
    float pitchD = module * preassureAngle;
    float addendumD = pitchD + module * 2;
    float clearingD = pitchD - module * 2;
    float dedendumD = pitchD - module * 2 * 1.5; // Root angle
    float baseD = pitchD * std::cos(preassureAngle / 180.f * pi<float>());
    float pitchAngle = pi<float>() / numTeeth;
    float gearPitch = pitchAngle * pitchD / 2.f;

    float thresholdAngle(float d) {
        auto angle = profileThresholdAngle(d);
        auto p = involuteProfile(angle);
        return std::atan2(p.y, p.x);
    }

    glm::vec2 involuteProfile(float angle) {
        auto r = baseD / 2.f;
        return r *vec2{std::cos(angle), -std::sin(angle)} +
               r * angle *vec2{std::sin(angle), std::cos(angle)};
    }

    // Note this is only the angle that is used as input to the involuteProfile
    // function. Use realAngleFromThreshold to get the real angle
    float profileThresholdAngle(float d) const {
        float x = d / 2;
        float x2 = x * x;
        float r = baseD / 2;
        float r2 = r * r;
        return std::sqrt(x2 / r2 - 1);
    }
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
    auto gear3 = Gear{};

    auto settings = GearSettings{40, 10};

    gear1.pos = {200, 200};
    gear2.pos = {gear1.pos.x + settings.pitchD, 200};
    gear3.pos = {gear1.pos.x + settings.pitchD, 200};

    auto center = (gear1.pos + gear2.pos) / 2.f;

    {
        glm::vec2 v = {};
        auto correctionAngle = 0;

        for (auto angle = 0.f; length(v) <= settings.addendumD / 2.f;
             angle += .01) {

            v = settings.involuteProfile(angle);
            gear1.points.push_back(v);
        }
    }

    gear1.rotatePoints(-settings.thresholdAngle(settings.pitchD)
                       //                       +settings.pitchAngle
    );

    gear1.mirror();

    gear2.points = gear1.points;

    bool isRunning = true;

    for (; isRunning;) {
        for (auto event = std::optional<sdl::Event>{};
             (event = sdl::pollEvent());) {

            if (event->type == SDL_QUIT) {
                isRunning = false;
                break;
            }
            if (event->type == SDL_MOUSEMOTION) {
                gear1.angle = 1. / 100. * event->motion.y - 1.;
                gear2.angle = -gear1.angle + pi<float>();

                gear3.pos =
                    gear2.pos + vec2{0, gear1.angle * settings.pitchD / 2.};

                window.title(std::to_string(gear1.angle).c_str());
            }
        }
        renderer.drawColor({100, 0, 0, 255});
        renderer.clear();
        renderer.drawColor({100, 100, 100, 255});

        drawLine(renderer,
                 gear1.pos,
                 gear1.pos + settings.pitchD / 2.f *
                                 vec2{cos(gear1.angle), sin(gear1.angle)});

        drawArc(renderer, gear1.pos, settings.addendumD / 2);
        drawArc(renderer, gear1.pos, settings.clearingD / 2);
        drawArc(renderer, gear2.pos, settings.pitchD / 2);

        renderer.drawColor({0, 0, 30});
        drawArc(renderer, gear1.pos, settings.dedendumD / 2);

        renderer.drawColor({200, 0, 0});
        drawArc(renderer, gear1.pos, settings.baseD / 2);

        renderer.drawColor({200, 200, 200});
        drawArc(renderer, gear1.pos, settings.pitchD / 2);

        gear1.draw(renderer);
        gear2.draw(renderer);

        renderer.drawColor({255, 255, 255});

        renderer.present();
        std::this_thread::sleep_for(10ms);
    }

    return 0;
}
