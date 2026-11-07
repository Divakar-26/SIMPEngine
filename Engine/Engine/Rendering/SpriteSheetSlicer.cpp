#include <Engine/Rendering/SpriteSheetSlicer.h>
#include <Engine/Core/VFS.h>
#include <Engine/Core/Log.h>

#include <stb_image.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <utility>
#include <vector>

namespace SIMPEngine
{
    namespace
    {
        struct Mask
        {
            int Width = 0;
            int Height = 0;
            std::vector<unsigned char> Solid;

            bool At(int x, int y) const { return Solid[static_cast<size_t>(y * Width + x)] != 0; }
        };

        Mask BuildMask(const unsigned char *rgba, int width, int height)
        {
            Mask mask;
            mask.Width = width;
            mask.Height = height;
            mask.Solid.resize(static_cast<size_t>(width * height));

            size_t pixels = static_cast<size_t>(width * height);
            size_t transparent = 0;
            for (size_t i = 0; i < pixels; ++i)
                if (rgba[i * 4 + 3] < 16)
                    ++transparent;

            if (transparent > pixels / 50)
            {
                for (size_t i = 0; i < pixels; ++i)
                    mask.Solid[i] = rgba[i * 4 + 3] >= 16 ? 1 : 0;
                return mask;
            }

            const std::array<size_t, 4> corners = {0, static_cast<size_t>(width - 1), static_cast<size_t>((height - 1) * width),
                                                   static_cast<size_t>(height * width - 1)};
            auto distance = [&](size_t a, size_t b)
            {
                return std::abs(rgba[a * 4] - rgba[b * 4]) + std::abs(rgba[a * 4 + 1] - rgba[b * 4 + 1]) +
                       std::abs(rgba[a * 4 + 2] - rgba[b * 4 + 2]);
            };

            size_t background = corners[0];
            int bestVotes = -1;
            for (size_t candidate : corners)
            {
                int votes = 0;
                for (size_t other : corners)
                    if (distance(candidate, other) < 30)
                        ++votes;
                if (votes > bestVotes)
                {
                    bestVotes = votes;
                    background = candidate;
                }
            }

            for (size_t i = 0; i < pixels; ++i)
                mask.Solid[i] = distance(i, background) >= 30 ? 1 : 0;
            return mask;
        }

        std::vector<std::pair<int, int>> Segments(const std::vector<bool> &has)
        {
            std::vector<std::pair<int, int>> segments;
            int start = -1;
            for (int i = 0; i <= static_cast<int>(has.size()); ++i)
            {
                bool on = i < static_cast<int>(has.size()) && has[static_cast<size_t>(i)];
                if (on && start < 0)
                    start = i;
                else if (!on && start >= 0)
                {
                    segments.push_back({start, i - 1});
                    start = -1;
                }
            }
            return segments;
        }

        int DetectAxis(const std::vector<bool> &has)
        {
            int n = static_cast<int>(has.size());
            auto segments = Segments(has);
            if (segments.size() < 2)
                return 0;

            int contentEnd = segments.back().second + 1;
            auto valid = [&](int cell)
            {
                if (cell < 4 || cell > n / 2)
                    return false;
                int cells = n / cell;
                if (n % cell != 0 && contentEnd > cells * cell)
                    return false;

                std::vector<bool> filled(static_cast<size_t>(cells), false);
                for (const auto &[a, b] : segments)
                {
                    if (a / cell != b / cell || a / cell >= cells)
                        return false;
                    filled[static_cast<size_t>(a / cell)] = true;
                }

                int first = -1, last = -1;
                for (int k = 0; k < cells; ++k)
                {
                    if (!filled[static_cast<size_t>(k)])
                        continue;
                    if (first < 0)
                        first = k;
                    last = k;
                }
                for (int k = first; k <= last; ++k)
                    if (!filled[static_cast<size_t>(k)])
                        return false;
                return true;
            };

            if (segments.size() >= 3)
            {
                std::vector<int> diffs;
                for (size_t k = 1; k < segments.size(); ++k)
                    diffs.push_back(segments[k].first - segments[k - 1].first);
                std::vector<int> sorted = diffs;
                std::sort(sorted.begin(), sorted.end());
                int median = sorted[sorted.size() / 2];

                int consistent = 0, sum = 0;
                for (int d : diffs)
                {
                    if (std::abs(d - median) * 10 <= median)
                    {
                        ++consistent;
                        sum += d;
                    }
                }
                if (consistent > 0 && consistent * 10 >= static_cast<int>(diffs.size()) * 6)
                {
                    int period = (sum + consistent / 2) / consistent;
                    for (int delta = 0; delta <= std::max(1, period / 10); ++delta)
                    {
                        if (valid(period - delta))
                            return period - delta;
                        if (delta && valid(period + delta))
                            return period + delta;
                    }
                }
            }

            for (int pass = 0; pass < 2; ++pass)
                for (int cell = 4; cell <= n / 2; ++cell)
                    if ((n % cell == 0) == (pass == 0) && valid(cell))
                        return cell;
            return 0;
        }

        int RefineInBands(const Mask &mask, bool columnsAcrossRows, int band, int current)
        {
            int length = columnsAcrossRows ? mask.Width : mask.Height;
            int span = columnsAcrossRows ? mask.Height : mask.Width;
            if (band <= 0 || span / band < 2)
                return current;

            std::vector<int> votes;
            for (int b = 0; b + band <= span; b += band)
            {
                std::vector<bool> has(static_cast<size_t>(length), false);
                for (int i = 0; i < length; ++i)
                    for (int j = b; j < b + band && !has[static_cast<size_t>(i)]; ++j)
                        has[static_cast<size_t>(i)] = columnsAcrossRows ? mask.At(i, j) : mask.At(j, i);
                int size = DetectAxis(has);
                if (size > 0 && length % size == 0)
                    votes.push_back(size);
            }
            if (votes.empty())
                return current;

            std::vector<int> unique = votes;
            std::sort(unique.begin(), unique.end());
            unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
            int quorum = std::max(1, static_cast<int>(votes.size()) / 4);
            for (int candidate : unique)
            {
                int count = static_cast<int>(std::count(votes.begin(), votes.end(), candidate));
                bool divides = std::all_of(votes.begin(), votes.end(), [candidate](int v)
                                           { return v % candidate == 0; });
                if (count >= quorum && divides && candidate <= current && current % candidate == 0)
                    return candidate;
            }
            return current;
        }

        bool CellHasContent(const Mask &mask, int x0, int y0, int w, int h)
        {
            for (int y = y0; y < std::min(mask.Height, y0 + h); ++y)
                for (int x = x0; x < std::min(mask.Width, x0 + w); ++x)
                    if (mask.At(x, y))
                        return true;
            return false;
        }
    }

    SpriteSheetSlice SpriteSheetSlicer::Detect(const unsigned char *rgba, int width, int height)
    {
        SpriteSheetSlice result;
        result.FrameWidth = width;
        result.FrameHeight = height;
        if (!rgba || width <= 0 || height <= 0)
            return result;

        Mask mask = BuildMask(rgba, width, height);

        std::vector<bool> columns(static_cast<size_t>(width), false);
        std::vector<bool> rows(static_cast<size_t>(height), false);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                if (mask.At(x, y))
                {
                    columns[static_cast<size_t>(x)] = true;
                    rows[static_cast<size_t>(y)] = true;
                }
            }
        }

        int cw = DetectAxis(columns);
        int ch = DetectAxis(rows);
        result.Method = "gaps between frames";

        if (cw && !ch)
            ch = height % cw == 0 ? cw : height;
        else if (ch && !cw)
            cw = width % ch == 0 ? ch : width;
        else if (!cw && !ch)
        {
            if (width > height && width % height == 0)
            {
                cw = ch = height;
                result.Method = "square frames in a strip";
            }
            else if (height > width && height % width == 0)
            {
                cw = ch = width;
                result.Method = "square frames in a strip";
            }
        }

        if (cw && ch)
        {
            cw = RefineInBands(mask, true, ch, cw);
            ch = RefineInBands(mask, false, cw, ch);
        }

        if (!cw || !ch)
        {
            result.Method = "single frame";
            return result;
        }

        result.FrameWidth = cw;
        result.FrameHeight = ch;
        result.Columns = std::max(1, width / cw);
        result.Rows = std::max(1, height / ch);

        int last = 0;
        for (int r = 0; r < result.Rows; ++r)
            for (int c = 0; c < result.Columns; ++c)
                if (CellHasContent(mask, c * cw, r * ch, cw, ch))
                    last = r * result.Columns + c;
        result.FrameCount = last + 1;
        result.Found = result.Columns * result.Rows > 1;
        return result;
    }

    SpriteSheetSlice SpriteSheetSlicer::Detect(const std::string &path)
    {
        std::string real = path;
        if (auto resolved = VFS::Resolve(path))
            real = *resolved;

        stbi_set_flip_vertically_on_load(false);
        int width = 0, height = 0, channels = 0;
        unsigned char *pixels = stbi_load(real.c_str(), &width, &height, &channels, 4);
        stbi_set_flip_vertically_on_load(true);
        if (!pixels)
        {
            CORE_WARN("Slicer: cannot read '{}'", path);
            return {};
        }

        SpriteSheetSlice result = Detect(pixels, width, height);
        stbi_image_free(pixels);
        return result;
    }
}
