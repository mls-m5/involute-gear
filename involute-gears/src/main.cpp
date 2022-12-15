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
    float scale = 10;
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

struct GearSettings {
    // Input parameters
    int numTeeth = 10;
    int module = 1;
    float preassureAngle = 20; // Degrees 20 is standard for most gears

    // Results. Don't set these unless you know what you're doing
    float pitchD = module * numTeeth;
    float addendumD = pitchD + module * 2;
    float clearingD = pitchD - module * 2;
    float dedendumD = pitchD - module * 2 * 1.5; // Root angle
    float baseD = pitchD * std::cos(preassureAngle / 180.f * pi<float>());
    float pitchAngle = pi<float>() * 2. / numTeeth;
    float gearPitch = pitchAngle * pitchD / 2.f;

    float thresholdAngle(float d) {
        auto angle = profileThresholdAngle(d);
        auto p = involuteProfile(angle);
        return std::atan2(p.y, p.x);
    }

    // Calculate the profile
    // This is the most important function when calculating gears
    glm::vec2 involuteProfile(float angle) {
        auto r = baseD / 2.f;
        return r *vec2{std::cos(angle), -std::sin(angle)} +
               r * angle *vec2{std::sin(angle), std::cos(angle)};
    }

    // Note this is only the angle that is used as input to the involuteProfile
    // function. Use thresholdAngle to get the real angle
    float profileThresholdAngle(float d) const {
        float x = d / 2;
        float x2 = x * x;
        float r = baseD / 2;
        float r2 = r * r;
        auto inside = x2 / r2 - 1;
        if (inside <= 0) {
            return 0;
        }
        return std::sqrt(inside);
    }
};

struct GearProfile {
    GearSettings settings;

    GearProfile(GearSettings settings)
        : settings{settings} {
        computeProfile();
    }

    void computeProfile() {
        points.clear();
        glm::vec2 v = {};
        auto correctionAngle = 0;

        float l = 0;

        for (auto angle = settings.thresholdAngle(settings.dedendumD);
             v = settings.involuteProfile(angle),
                  (l = length(v)) <= settings.addendumD / 2.f;
             angle += settings.module * .01) {

            if (l < settings.clearingD / 2.f) {
                continue;
            }

            points.push_back(v);
        }

        auto last = normalize(points.front()) * settings.dedendumD / 2.f;
        points.insert(points.begin(), last);

        rotatePoints(-settings.thresholdAngle(settings.pitchD) +
                     settings.pitchAngle / 2.f / 2.f);
        mirror();
        repeat(settings.numTeeth);
        points.push_back(points.front()); // Close loop
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

    std::vector<glm::vec2> points;
};

struct GearView {
    GearProfile &gear;
    glm::vec2 pos;
    float angle = 0;

    auto createLocation() {
        mat4 location = identity<mat4>();
        location = translate(location, vec3{pos, 0});
        location = rotate(location, angle, {0, 0, 1});
        return location;
    }

    void draw(sdl::RendererView view) {
        auto location = createLocation();

        auto &points = gear.points;

        for (size_t i = 1; i < points.size(); ++i) {
            auto p1 = location *vec4{points.at(i - 1), 0, 1};
            auto p2 = location *vec4{points.at(i), 0, 1};
            drawLine(view, p1, p2);
        }
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

    auto gear =
        GearProfile{{.numTeeth = 30, .module = 1, .preassureAngle = 20.}};

    auto gearView1 = GearView{gear};
    auto gearView2 = GearView{gear};

    auto &settings = gear.settings;

    gearView1.pos = {settings.pitchD / 2., settings.pitchD / 2.};
    gearView2.pos = {gearView1.pos.x + settings.pitchD, gearView1.pos.x};

    auto center = (gearView1.pos + gearView2.pos) / 2.f;

    bool isRunning = true;

    for (; isRunning;) {
        for (auto event = std::optional<sdl::Event>{};
             (event = sdl::pollEvent());) {

            if (event->type == SDL_QUIT) {
                isRunning = false;
                break;
            }
            if (event->type == SDL_MOUSEMOTION) {
                gearView1.angle = 1. / 100. * event->motion.y - 1.;
                gearView2.angle =
                    -gearView1.angle + pi<float>() + settings.pitchAngle / 2.f;

                window.title(std::to_string(gearView1.angle).c_str());
            }
        }
        renderer.drawColor({100, 0, 0, 255});
        renderer.clear();
        renderer.drawColor({100, 100, 100, 255});

        drawLine(renderer,
                 gearView1.pos,
                 gearView1.pos +
                     settings.pitchD / 2.f *
                         vec2{cos(gearView1.angle), sin(gearView1.angle)});
        drawLine(renderer,
                 gearView1.pos,
                 gearView1.pos +
                     settings.pitchD / 2.f *
                         vec2{cos(gearView1.angle + settings.pitchAngle),
                              sin(gearView1.angle + settings.pitchAngle)});

        drawArc(renderer, gearView1.pos, settings.addendumD / 2);
        drawArc(renderer, gearView1.pos, settings.clearingD / 2);
        drawArc(renderer, gearView2.pos, settings.pitchD / 2);

        renderer.drawColor({0, 0, 30});
        drawArc(renderer, gearView1.pos, settings.dedendumD / 2);

        renderer.drawColor({200, 0, 0});
        drawArc(renderer, gearView1.pos, settings.baseD / 2);

        renderer.drawColor({200, 200, 200});
        drawArc(renderer, gearView1.pos, settings.pitchD / 2);

        gearView1.draw(renderer);
        gearView2.draw(renderer);

        renderer.drawColor({255, 255, 255});

        renderer.present();
        std::this_thread::sleep_for(10ms);
    }

    return 0;
}
