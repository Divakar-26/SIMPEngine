#include "Levels.h"
#include "Builder.h"

namespace Mossveil
{
    namespace
    {
        SDL_Color Blend(SDL_Color a, SDL_Color b, float t)
        {
            return {static_cast<Uint8>(a.r + (b.r - a.r) * t), static_cast<Uint8>(a.g + (b.g - a.g) * t), static_cast<Uint8>(a.b + (b.b - a.b) * t), 255};
        }

        void Scenery(Builder &b, float x0, float x1, float groundY)
        {
            const Palette &p = b.Colors();
            SDL_Color sky{static_cast<Uint8>(p.Background.r * 255.0f), static_cast<Uint8>(p.Background.g * 255.0f), static_cast<Uint8>(p.Background.b * 255.0f), 255};
            b.Background(x0, x1, groundY - 260.0f, 0.2f, -70.0f, Blend(p.Silhouette, sky, 0.62f), 560.0f, 1.5f, 2.6f);
            b.Fog(x0, x1, groundY - 100.0f, 900.0f, 0.3f, -60.0f, {p.Fog.r, p.Fog.g, p.Fog.b, 22});
            b.Background(x0, x1, groundY - 160.0f, 0.45f, -45.0f, Blend(p.Silhouette, sky, 0.38f), 430.0f, 1.0f, 1.8f);
            b.Fog(x0, x1, groundY - 60.0f, 420.0f, 0.55f, -35.0f, {p.Fog.r, p.Fog.g, p.Fog.b, 18});
            for (float x = x0 + 400.0f; x < x1; x += b.Range(900.0f, 1400.0f))
                b.Light({x, groundY + b.Range(500.0f, 900.0f)}, b.Random() < 0.5f ? p.Cool : p.Accent, b.Range(1100.0f, 1600.0f), b.Range(0.35f, 0.5f), 0.6f);
        }

        void Dress(Builder &b, float x0, float x1, float top, bool lush = true)
        {
            const Palette &p = b.Colors();
            b.Foliage(x0, x1, top, lush ? 1.0f : 0.6f);
            b.Mist(x0, x1, top);
            b.Spores({(x0 + x1) * 0.5f, top + 420.0f}, {x1 - x0, 700.0f});
            if (b.Random() < 0.55f && x1 - x0 > 300.0f)
                b.GlowPlant(b.Random() < 0.5f ? Sheets::FlowerCap : Sheets::FlowerBlue, b.Range(x0 + 80.0f, x1 - 80.0f), top - 6.0f, b.Range(110.0f, 150.0f), b.Random() < 0.6f ? p.Cool : p.Accent);
            if (b.Random() < 0.4f)
                b.Fireflies({(x0 + x1) * 0.5f, top + 260.0f}, {x1 - x0, 320.0f}, 10);
        }
    }

    void BuildMossway(Builder &b)
    {
        Palette p;
        p.Background = {0.20f, 0.36f, 0.36f};
        p.Ambient = {0.34f, 0.43f, 0.45f};
        p.Warm = {255, 200, 135, 255};
        p.Cool = {120, 225, 255, 255};
        p.Accent = {170, 255, 190, 255};
        p.Silhouette = {62, 98, 98, 255};
        p.Fog = {150, 210, 205, 24};
        b.Atmosphere(p);
        Scenery(b, -300.0f, 7400.0f, 0.0f);

        b.Wall(-900.0f, -300.0f, -2600.0f, 3200.0f);
        b.Wall(7400.0f, 8000.0f, -2600.0f, 3200.0f);

        b.Ground(-300.0f, 1500.0f, 0.0f, -2600.0f, EdgeRight);
        b.Ceiling(-300.0f, 1150.0f, 760.0f, 3200.0f, EdgeRight);
        b.HangingFoliage(-300.0f, 1150.0f, 760.0f, 1.2f);
        b.Drips(-200.0f, 1100.0f, 760.0f);
        b.Shaft(520.0f, 760.0f, 900.0f, p.Warm, 15.0f);
        Dress(b, -280.0f, 1480.0f, 0.0f);
        b.GlowPlant(Sheets::FlowerCap, 1180.0f, -6.0f, 125.0f, p.Cool);
        b.Wisp({660.0f, 190.0f});

        b.Wisp({1610.0f, 300.0f});
        b.Ground(1720.0f, 2350.0f, 80.0f);
        Dress(b, 1730.0f, 2340.0f, 80.0f);
        b.Platform(2140.0f, 300.0f, 250.0f);
        b.Wisp({2140.0f, 430.0f});

        b.Ground(2350.0f, 3250.0f, 200.0f, -2600.0f, EdgeRight);
        Dress(b, 2360.0f, 3240.0f, 200.0f);
        b.Checkpoint(2660.0f, 200.0f);
        b.GlowPlant(Sheets::FlowerBlue, 3060.0f, 194.0f, 140.0f, p.Cool);
        b.Shaft(2900.0f, 1700.0f, 1500.0f, p.Cool, 12.0f);

        b.Platform(3430.0f, 330.0f, 240.0f);
        b.Platform(3720.0f, 420.0f, 240.0f);
        b.Wisp({3430.0f, 460.0f});
        b.Wisp({3720.0f, 550.0f});
        b.Fireflies({3600.0f, 150.0f}, {700.0f, 500.0f}, 14);
        b.Spores({3600.0f, 400.0f}, {700.0f, 700.0f});

        b.Ground(3950.0f, 5250.0f, 120.0f);
        b.Ceiling(4250.0f, 5250.0f, 640.0f);
        b.HangingFoliage(4250.0f, 5250.0f, 640.0f, 1.4f);
        b.Drips(4300.0f, 5200.0f, 640.0f);
        Dress(b, 3960.0f, 5240.0f, 120.0f);
        b.GlowPlant(Sheets::FlowerCap, 4420.0f, 114.0f, 115.0f, p.Cool);
        b.GlowPlant(Sheets::FlowerBlue, 4980.0f, 114.0f, 135.0f, p.Accent);
        b.Light({4700.0f, 430.0f}, p.Warm, 620.0f, 0.9f, 1.0f, 0.2f);
        b.Fireflies({4750.0f, 380.0f}, {900.0f, 380.0f}, 16);
        b.Wisp({4500.0f, 270.0f});
        b.Wisp({5000.0f, 270.0f});

        b.Platform(5430.0f, 330.0f, 230.0f);
        b.Platform(5700.0f, 500.0f, 230.0f);
        b.Wisp({5700.0f, 630.0f});

        b.Ground(5880.0f, 7400.0f, 360.0f, -2600.0f, EdgeLeft);
        b.Ceiling(6400.0f, 7400.0f, 1500.0f, 3200.0f, EdgeLeft);
        b.HangingFoliage(6400.0f, 7400.0f, 1500.0f, 1.0f);
        b.Shaft(6820.0f, 1500.0f, 1300.0f, p.Cool, 14.0f);
        Dress(b, 5890.0f, 7390.0f, 360.0f);
        b.Checkpoint(6100.0f, 360.0f);
        b.Wisp({6450.0f, 560.0f});
        b.Exit(7000.0f, 360.0f);

        glm::vec2 spawn{100.0f, 60.0f};
        b.Player(spawn);
        b.Camera(spawn, {-300.0f, -500.0f}, {7400.0f, 1500.0f});
    }

    void BuildThornroot(Builder &b)
    {
        Palette p;
        p.Background = {0.22f, 0.22f, 0.37f};
        p.Ambient = {0.29f, 0.30f, 0.43f};
        p.Warm = {255, 185, 125, 255};
        p.Cool = {150, 175, 255, 255};
        p.Accent = {215, 150, 255, 255};
        p.Silhouette = {64, 62, 96, 255};
        p.Fog = {170, 160, 230, 24};
        b.Atmosphere(p, 0.7f, 0.6f);
        Scenery(b, -300.0f, 8600.0f, 0.0f);

        b.Wall(-900.0f, -300.0f, -2600.0f, 3200.0f);
        b.Wall(8600.0f, 9200.0f, -2600.0f, 3200.0f);

        b.Ground(-300.0f, 1900.0f, 0.0f, -2600.0f, EdgeRight);
        b.Ceiling(-300.0f, 2000.0f, 720.0f, 3200.0f, EdgeRight);
        b.HangingFoliage(-300.0f, 2000.0f, 720.0f, 1.3f);
        b.Drips(-200.0f, 1900.0f, 720.0f);
        Dress(b, -280.0f, 1890.0f, 0.0f);
        b.Thorns(620.0f, 0.0f, 140.0f);
        b.Thorns(1360.0f, 0.0f, 180.0f);
        b.Wisp({620.0f, 250.0f});
        b.Wisp({1360.0f, 280.0f});
        b.GlowPlant(Sheets::FlowerBlue, 980.0f, -6.0f, 130.0f, p.Accent);
        b.Light({250.0f, 420.0f}, p.Warm, 520.0f, 0.8f, 1.0f, 0.2f);

        b.Ground(1900.0f, 2700.0f, -350.0f);
        Dress(b, 1910.0f, 2690.0f, -350.0f, false);
        b.Thorns(2300.0f, -350.0f, 200.0f);
        b.Wisp({2300.0f, -110.0f});

        b.Platform(2900.0f, -150.0f, 230.0f);
        b.Platform(3150.0f, 60.0f, 230.0f);
        b.Platform(2920.0f, 270.0f, 230.0f);
        b.Platform(3180.0f, 480.0f, 230.0f);
        b.Wisp({2920.0f, 400.0f});
        b.Wisp({3180.0f, 610.0f});
        b.Spores({3050.0f, 200.0f}, {600.0f, 1000.0f});
        b.Shaft(3050.0f, 1600.0f, 1900.0f, p.Accent, 11.0f);

        b.Ground(3350.0f, 4300.0f, 650.0f, -2600.0f, EdgeLeft | EdgeRight);
        b.Ceiling(3500.0f, 4300.0f, 1180.0f);
        b.HangingFoliage(3500.0f, 4300.0f, 1180.0f, 1.2f);
        Dress(b, 3360.0f, 4290.0f, 650.0f);
        b.Checkpoint(3480.0f, 650.0f);
        b.DashShrine(3950.0f, 650.0f);

        b.Ground(4300.0f, 4740.0f, -120.0f);
        b.Thorns(4410.0f, -120.0f, 190.0f);
        b.Thorns(4630.0f, -120.0f, 190.0f);
        b.Wisp({4520.0f, 820.0f});

        b.Ground(4740.0f, 5700.0f, 650.0f);
        b.Ceiling(4740.0f, 5700.0f, 1030.0f);
        b.HangingFoliage(4760.0f, 5680.0f, 1030.0f, 1.6f);
        b.Drips(4800.0f, 5650.0f, 1030.0f);
        Dress(b, 4750.0f, 5690.0f, 650.0f);
        b.Thorns(5250.0f, 650.0f, 150.0f);
        b.GlowPlant(Sheets::FlowerCap, 4950.0f, 644.0f, 110.0f, p.Cool);
        b.GlowPlant(Sheets::FlowerCap, 5520.0f, 644.0f, 110.0f, p.Accent);
        b.Wisp({5250.0f, 880.0f});

        b.Platform(5910.0f, 520.0f, 230.0f);
        b.Platform(6150.0f, 380.0f, 230.0f);
        b.Wisp({6150.0f, 510.0f});

        b.Ground(6300.0f, 7200.0f, 250.0f);
        Dress(b, 6310.0f, 7190.0f, 250.0f);
        b.Checkpoint(6450.0f, 250.0f);
        b.Thorns(6820.0f, 250.0f, 200.0f);
        b.Wisp({6820.0f, 480.0f});
        b.Shaft(6700.0f, 1800.0f, 1600.0f, p.Warm, 13.0f);

        b.Ground(7200.0f, 7620.0f, -300.0f);
        b.Thorns(7300.0f, -300.0f, 180.0f);
        b.Thorns(7510.0f, -300.0f, 180.0f);
        b.Wisp({7410.0f, 450.0f});

        b.Ground(7620.0f, 8600.0f, 330.0f, -2600.0f, EdgeLeft);
        b.Ceiling(7900.0f, 8600.0f, 1350.0f, 3200.0f, EdgeLeft);
        b.HangingFoliage(7900.0f, 8600.0f, 1350.0f, 1.0f);
        Dress(b, 7630.0f, 8590.0f, 330.0f);
        b.Exit(8250.0f, 330.0f);

        glm::vec2 spawn{80.0f, 60.0f};
        b.Player(spawn);
        b.Camera(spawn, {-300.0f, -900.0f}, {8600.0f, 2200.0f});
    }

    void BuildShiftingDeep(Builder &b, uint32_t seed)
    {
        const Palette palettes[3] = {
            {{0.18f, 0.34f, 0.33f}, {0.30f, 0.40f, 0.41f}, {255, 196, 125, 255}, {110, 230, 240, 255}, {180, 255, 170, 255}, {58, 95, 92, 255}, {150, 215, 200, 24}},
            {{0.26f, 0.20f, 0.34f}, {0.34f, 0.28f, 0.42f}, {255, 170, 120, 255}, {170, 160, 255, 255}, {255, 160, 210, 255}, {84, 60, 92, 255}, {200, 170, 220, 24}},
            {{0.14f, 0.24f, 0.36f}, {0.26f, 0.34f, 0.46f}, {255, 215, 150, 255}, {110, 190, 255, 255}, {130, 255, 225, 255}, {50, 78, 104, 255}, {140, 190, 230, 24}}};
        const Palette &p = palettes[seed % 3];
        b.Atmosphere(p, 0.7f, 0.6f);

        b.Wall(-900.0f, -300.0f, -2600.0f, 3200.0f);

        float x = 700.0f;
        float top = 0.0f;
        float minTop = 0.0f, maxTop = 0.0f;
        b.Ground(-300.0f, x, top, -2600.0f, 0);
        b.Ceiling(-300.0f, 900.0f, 760.0f);
        b.HangingFoliage(-300.0f, 900.0f, 760.0f, 1.2f);
        Dress(b, -280.0f, x, top);
        b.Shaft(350.0f, 760.0f, 900.0f, p.Warm, 15.0f);

        auto clampTop = [](float value) { return std::clamp(value, -350.0f, 950.0f); };
        int segments = 12 + static_cast<int>(seed % 5);
        int sinceCheckpoint = 0;

        for (int i = 0; i < segments; ++i)
        {
            float roll = b.Random();
            float gap = 0.0f;
            float nextTop = top;
            float length = b.Range(600.0f, 1100.0f);
            bool ceiling = false;

            if (roll < 0.36f)
            {
                gap = b.Range(150.0f, 250.0f);
                nextTop = clampTop(top + b.Range(-220.0f, 170.0f));
                b.Wisp({x + gap * 0.5f, std::max(top, nextTop) + 230.0f});
            }
            else if (roll < 0.62f)
            {
                float px = x;
                float ptop = top;
                int count = 2 + b.Pick(2);
                for (int k = 0; k < count; ++k)
                {
                    float width = b.Range(220.0f, 260.0f);
                    px += b.Range(230.0f, 300.0f);
                    ptop = clampTop(ptop + b.Range(-110.0f, 170.0f));
                    b.Platform(px, ptop, width);
                    if (b.Random() < 0.6f)
                        b.Wisp({px, ptop + 130.0f});
                }
                b.Spores({(x + px) * 0.5f, ptop + 200.0f}, {px - x + 300.0f, 700.0f});
                if (b.Random() < 0.5f)
                    b.Fireflies({(x + px) * 0.5f, ptop}, {px - x + 200.0f, 400.0f}, 12);
                gap = px - x + b.Range(180.0f, 240.0f);
                nextTop = clampTop(ptop + b.Range(-150.0f, 150.0f));
            }
            else if (roll < 0.78f)
            {
                gap = 0.0f;
                length = b.Range(900.0f, 1300.0f);
            }
            else if (roll < 0.88f)
            {
                gap = b.Range(380.0f, 430.0f);
                nextTop = clampTop(top + b.Range(-120.0f, 40.0f));
                b.Wisp({x + gap * 0.5f, top + 160.0f});
            }
            else
            {
                gap = b.Range(150.0f, 230.0f);
                length = b.Range(1000.0f, 1400.0f);
                ceiling = true;
            }

            float x0 = x + gap;
            float x1 = x0 + length;
            b.Ground(x0, x1, nextTop, -2600.0f, static_cast<uint8_t>(gap > 0.0f ? (EdgeLeft | EdgeRight) : EdgeRight));
            Dress(b, x0 + 10.0f, x1 - 10.0f, nextTop);
            minTop = std::min(minTop, nextTop);
            maxTop = std::max(maxTop, nextTop);

            if (gap == 0.0f)
            {
                float tx = x0 + length * b.Range(0.4f, 0.6f);
                b.Thorns(tx, nextTop, b.Range(140.0f, 200.0f));
                b.Wisp({tx, nextTop + 250.0f});
            }

            if (ceiling)
            {
                float height = b.Range(560.0f, 700.0f);
                b.Ceiling(x0 + 100.0f, x1 - 100.0f, nextTop + height);
                b.HangingFoliage(x0 + 100.0f, x1 - 100.0f, nextTop + height, 1.5f);
                b.Drips(x0 + 150.0f, x1 - 150.0f, nextTop + height);
                b.Light({(x0 + x1) * 0.5f, nextTop + height * 0.6f}, p.Warm, 620.0f, 0.9f, 1.0f, 0.2f);
                b.Fireflies({(x0 + x1) * 0.5f, nextTop + height * 0.5f}, {length * 0.8f, height * 0.6f}, 16);
                b.GlowPlant(Sheets::FlowerCap, x0 + length * 0.3f, nextTop - 6.0f, 115.0f, p.Cool);
            }
            else if (b.Random() < 0.45f)
            {
                b.Shaft(b.Range(x0 + 100.0f, x1 - 100.0f), nextTop + 1500.0f, 1400.0f, b.Random() < 0.5f ? p.Warm : p.Cool, b.Range(11.0f, 15.0f));
            }

            if (++sinceCheckpoint >= 4 && length > 500.0f)
            {
                b.Checkpoint(x0 + 140.0f, nextTop);
                sinceCheckpoint = 0;
            }

            x = x1;
            top = nextTop;
        }

        float gap = b.Range(150.0f, 220.0f);
        float finalTop = clampTop(top + b.Range(-80.0f, 120.0f));
        float x0 = x + gap;
        float x1 = x0 + 1500.0f;
        b.Ground(x0, x1, finalTop, -2600.0f, EdgeLeft);
        b.Ceiling(x0 + 500.0f, x1, finalTop + 1250.0f, 3200.0f, EdgeLeft);
        b.HangingFoliage(x0 + 500.0f, x1, finalTop + 1250.0f, 1.0f);
        Dress(b, x0 + 10.0f, x1 - 10.0f, finalTop);
        b.Exit(x0 + 1000.0f, finalTop);
        b.Wall(x1, x1 + 600.0f, -2600.0f, 3200.0f);
        maxTop = std::max(maxTop, finalTop);
        minTop = std::min(minTop, finalTop);

        Scenery(b, -300.0f, x1, 0.0f);

        glm::vec2 spawn{100.0f, 60.0f};
        b.Player(spawn);
        b.Camera(spawn, {-300.0f, minTop - 500.0f}, {x1, maxTop + 1300.0f});
    }
}
