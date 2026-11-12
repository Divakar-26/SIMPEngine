#include <Engine/Core/Log.h>
#include <Engine/Core/VFS.h>
#include <Engine/Core/Window.h>
#include <Engine/Motion/MotionHost.h>
#include <Engine/Project/Project.h>
#include <Engine/Rendering/Font.h>
#include <Engine/Rendering/Framebuffer.h>
#include <Engine/Rendering/GLRenderingAPI.h>
#include <Engine/Rendering/ImageWriter.h>
#include <Engine/Rendering/Renderer.h>
#include <Engine/Scripting/LuaRuntime.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#ifdef _WIN32
#define SIMP_POPEN _popen
#define SIMP_PCLOSE _pclose
#define SIMP_NULL "NUL"
#else
#define SIMP_POPEN popen
#define SIMP_PCLOSE pclose
#define SIMP_NULL "/dev/null"
#endif

namespace
{
    struct Quality
    {
        const char *name;
        int width, height, fps, samples, crf;
        const char *preset;
    };

    const Quality QUALITIES[] = {
        {"draft", 960, 540, 30, 2, 26, "veryfast"},
        {"hd", 1280, 720, 30, 4, 18, "medium"},
        {"fhd", 1920, 1080, 60, 4, 17, "medium"},
        {"4k", 3840, 2160, 30, 4, 16, "fast"},
    };

    const Quality *FindQuality(const std::string &name)
    {
        for (const Quality &q : QUALITIES)
            if (name == q.name)
                return &q;
        return nullptr;
    }

    void Usage()
    {
        std::fprintf(stderr,
                     "SIMPRecord <script.lua> [options]\n"
                     "  --out FILE        output (default <script>.mp4; .png writes a sequence)\n"
                     "  --quality Q       draft | hd | fhd (default) | 4k\n"
                     "  --size W H        custom resolution\n"
                     "  --fps N           frames per second\n"
                     "  --seconds S       length (default: Video.Duration set by the script)\n"
                     "  --from S          start at S seconds (the scene is simulated up to there)\n"
                     "  --still T         write a PNG of the frame at T seconds (repeat for several), no video\n"
                     "  --assets DIR      folder behind assets:// (default: ../assets next to the program)\n");
    }

    std::string WithoutExtension(const std::string &path)
    {
        const size_t dot = path.find_last_of('.');
        const size_t slash = path.find_last_of("/\\");
        if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
            return path;
        return path.substr(0, dot);
    }

    std::string Stem(const std::string &path)
    {
        std::string s = path;
        const size_t slash = s.find_last_of("/\\");
        if (slash != std::string::npos)
            s = s.substr(slash + 1);
        const size_t dot = s.find_last_of('.');
        if (dot != std::string::npos)
            s = s.substr(0, dot);
        return s;
    }
}

int main(int argc, char **argv)
{
    std::string script, out, assets;
    Quality q = *FindQuality("fhd");
    float seconds = 0.0f, from = 0.0f;
    std::vector<float> stills;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        auto next = [&](const char *what) -> const char *
        {
            if (i + 1 >= argc)
            {
                std::fprintf(stderr, "%s needs a value\n", what);
                std::exit(1);
            }
            return argv[++i];
        };
        if (arg == "--out")
            out = next("--out");
        else if (arg == "--quality")
        {
            const Quality *found = FindQuality(next("--quality"));
            if (!found)
            {
                std::fprintf(stderr, "unknown quality (draft, hd, fhd, 4k)\n");
                return 1;
            }
            q = *found;
        }
        else if (arg == "--size")
        {
            q.width = std::max(16, std::atoi(next("--size")));
            q.height = std::max(16, std::atoi(next("--size")));
        }
        else if (arg == "--fps")
            q.fps = std::max(1, std::atoi(next("--fps")));
        else if (arg == "--seconds")
            seconds = static_cast<float>(std::atof(next("--seconds")));
        else if (arg == "--from")
            from = static_cast<float>(std::atof(next("--from")));
        else if (arg == "--still")
            stills.push_back(static_cast<float>(std::atof(next("--still"))));
        else if (arg == "--assets")
            assets = next("--assets");
        else if (arg == "--help" || arg == "-h")
        {
            Usage();
            return 0;
        }
        else if (script.empty())
            script = arg;
        else
        {
            std::fprintf(stderr, "unexpected argument '%s'\n", arg.c_str());
            Usage();
            return 1;
        }
    }
    if (script.empty())
    {
        Usage();
        return 1;
    }
    if (out.empty())
        out = Stem(script) + ".mp4";

    using namespace SIMPEngine;
    Log::Init();
    VFS::Mount("assets", assets.empty() ? "../assets" : assets);
    Project::MountEngineResources();

    Window window;
    if (!window.Init("SIMPRecord", 320, 180, SDL_WINDOW_HIDDEN))
    {
        std::fprintf(stderr, "could not create an OpenGL context\n");
        return 3;
    }
    Renderer::Init(std::make_unique<GLRenderingAPI>(), 320, 180);

    int status = 0;
    {
        MotionHost host;
        if (!host.Load(script, true))
        {
            std::fprintf(stderr, "%s\n", host.Error().c_str());
            status = 2;
        }
        else
        {
            const float dt = 1.0f / static_cast<float>(q.fps);
            const float length = seconds > 0.0f ? seconds : host.Duration();
            Framebuffer fb(q.width, q.height, false, q.samples);
            std::vector<uint8_t> pixels;
            int frame = 0;

            auto advanceTo = [&](int target)
            {
                while (frame < target)
                {
                    host.Step(dt);
                    ++frame;
                }
            };
            auto render = [&]()
            {
                Renderer::BeginRenderTarget(fb);
                host.Render(q.width, q.height);
                fb.ReadPixels(pixels);
                Renderer::EndRenderTarget();
            };

            if (!stills.empty())
            {
                std::sort(stills.begin(), stills.end());
                for (float t : stills)
                {
                    advanceTo(static_cast<int>(std::lround(t * q.fps)));
                    render();
                    char name[512];
                    std::snprintf(name, sizeof(name), "%s_%.2f.png", WithoutExtension(out).c_str(), t);
                    if (ImageWriter::WritePng(name, q.width, q.height, pixels))
                        std::printf("wrote %s\n", name);
                }
            }
            else if (length <= 0.0f)
            {
                std::fprintf(stderr, "the script does not set Video.Duration; pass --seconds S\n");
                status = 2;
            }
            else
            {
                const int total = static_cast<int>(std::ceil(length * q.fps));
                const bool sequence = out.size() > 4 && out.compare(out.size() - 4, 4, ".png") == 0;
                bool haveFfmpeg = !sequence && std::system("ffmpeg -version > " SIMP_NULL " 2>&1") == 0;
                if (!sequence && !haveFfmpeg)
                {
                    std::fprintf(stderr, "ffmpeg was not found; writing a PNG sequence instead\n");
                    out = Stem(out) + "_.png";
                }
                FILE *pipe = nullptr;
                if (haveFfmpeg)
                {
                    char cmd[1024];
                    std::snprintf(cmd, sizeof(cmd),
                                  "ffmpeg -y -loglevel error -f rawvideo -pix_fmt rgba -s %dx%d -r %d -i - -c:v libx264 -preset %s -crf %d -pix_fmt yuv420p -movflags +faststart \"%s\"",
                                  q.width, q.height, q.fps, q.preset, q.crf, out.c_str());
                    pipe = SIMP_POPEN(cmd, "w");
                    if (!pipe)
                        status = 3;
                }
                if (status == 0)
                {
                    advanceTo(static_cast<int>(std::lround(from * q.fps)));
                    const int first = frame;
                    for (int f = first; f < total; ++f)
                    {
                        advanceTo(f);
                        render();
                        if (pipe)
                            std::fwrite(pixels.data(), 1, pixels.size(), pipe);
                        else
                        {
                            char name[512];
                            std::snprintf(name, sizeof(name), "%s%05d.png", out.substr(0, out.size() - 4).c_str(), f - first);
                            ImageWriter::WritePng(name, q.width, q.height, pixels);
                        }
                        if ((f - first) % (q.fps * 5) == 0)
                            std::fprintf(stderr, "%.1f / %.1f s\n", static_cast<float>(f) / static_cast<float>(q.fps), length);
                    }
                    if (pipe && SIMP_PCLOSE(pipe) != 0)
                        status = 4;
                    if (status == 0)
                        std::printf("wrote %s (%dx%d, %d fps, %.1f s)\n", out.c_str(), q.width, q.height, q.fps, length - from);
                }
            }
            if (status == 0 && !host.Error().empty())
            {
                std::fprintf(stderr, "script reported an error while recording:\n%s\n", host.Error().c_str());
                status = 5;
            }
        }
    }
    LuaRuntime::Get().ShutDown();
    Font::ClearCache();
    Renderer::Shutdown();
    window.ShutDown();
    return status;
}
