#include "LuaContext.h"

#include <Engine/Animation/Curves.h>

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <utility>

namespace ScriptEditor
{
    namespace
    {
        bool IsIdent(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

        struct Frame
        {
            char Open = '(';
            std::string Callee;
            std::string Receiver;
            int Arg = 0;
            bool Key = true;
            std::string CurrentKey;
            std::vector<std::string> Used;
            bool ParentIsCall = false;
            std::string ParentCallee;
            int ParentArg = 0;
        };

        void CalleeBefore(const std::string &text, size_t open, std::string &callee, std::string &receiver)
        {
            size_t end = open;
            while (end > 0 && (text[end - 1] == ' ' || text[end - 1] == '\t'))
                --end;
            size_t i = end;
            while (i > 0 && (IsIdent(text[i - 1]) || text[i - 1] == '.' || text[i - 1] == ':'))
                --i;
            callee = text.substr(i, end - i);
            receiver.clear();

            size_t colon = callee.rfind(':');
            if (colon == std::string::npos)
                return;
            const std::string name = callee.substr(colon + 1);
            size_t colonAt = i + colon;
            size_t r = colonAt;
            while (r > 0)
            {
                const char c = text[r - 1];
                if (c == ')' || c == ']')
                {
                    int depth = 0;
                    size_t k = r;
                    while (k > 0)
                    {
                        const char d = text[k - 1];
                        if (d == ')' || d == ']')
                            ++depth;
                        else if (d == '(' || d == '[')
                            --depth;
                        --k;
                        if (depth == 0)
                            break;
                    }
                    r = k;
                }
                else if (IsIdent(c) || c == '.' || c == ':')
                    --r;
                else
                    break;
            }
            receiver = text.substr(r, colonAt - r);
            callee = ":" + name;
        }

        struct FunctionSpec
        {
            const char *Name;
            std::vector<int> PropsArgs;
            int OptionsFrom;
            ArgRole OptionsRole;
            int CurveArg;
            int PropertyNameArg;
            int BusArg;
        };

        const std::vector<FunctionSpec> &Specs()
        {
            static const std::vector<FunctionSpec> specs = {
                {"Tween.To", {1}, 3, ArgRole::TweenOptionKeys, 3, -1, -1},
                {"Tween.From", {1}, 3, ArgRole::TweenOptionKeys, 3, -1, -1},
                {"Tween.FromTo", {1, 2}, 4, ArgRole::TweenOptionKeys, 4, -1, -1},
                {"Tween.Value", {}, 5, ArgRole::TweenOptionKeys, 3, -1, -1},
                {"Tween.Along", {}, 3, ArgRole::AlongOptionKeys, 3, -1, -1},
                {"Tween.Spring", {1}, 2, ArgRole::SpringOptionKeys, -1, -1, -1},
                {"Tween.Stagger", {1}, 5, ArgRole::TweenOptionKeys, 3, -1, -1},
                {"Tween.Sequence", {}, 1, ArgRole::TweenOptionKeys, -1, -1, -1},
                {"Tween.Parallel", {}, 1, ArgRole::TweenOptionKeys, -1, -1, -1},
                {"Tween.Kill", {}, -1, ArgRole::None, -1, 1, -1},
                {":MorphTo", {}, 3, ArgRole::TweenOptionKeys, 2, -1, -1},
                {":Curve", {}, -1, ArgRole::None, 0, -1, -1},
                {"Curves.Interval", {}, -1, ArgRole::None, 2, -1, -1},
                {"Curves.Flipped", {}, -1, ArgRole::None, 0, -1, -1},
                {"Curves.Reversed", {}, -1, ArgRole::None, 0, -1, -1},
                {"Curves.Evaluate", {}, -1, ArgRole::None, 0, -1, -1},
                {"Draw.Path", {}, 3, ArgRole::DrawPathOptionKeys, -1, -1, -1},
                {"Audio.Play", {}, -1, ArgRole::None, -1, -1, 4},
                {"Audio.SetBusVolume", {}, -1, ArgRole::None, -1, -1, 0},
                {"Audio.GetBusVolume", {}, -1, ArgRole::None, -1, -1, 0},
            };
            return specs;
        }

        const FunctionSpec *FindSpec(const std::string &callee)
        {
            for (const FunctionSpec &spec : Specs())
            {
                const std::string name = spec.Name;
                if (callee == name)
                    return &spec;
                if (name[0] == ':' && callee.size() >= name.size() && callee.compare(callee.size() - name.size(), name.size(), name) == 0)
                    return &spec;
            }
            return nullptr;
        }

        std::vector<Signature> BuildSignatures()
        {
            struct Raw
            {
                const char *Callee;
                const char *Params;
                const char *Summary;
            };
            static const Raw raw[] = {
                {"Tween.To", "target, props, seconds, curve?, options?", "Animate from the current values to props"},
                {"Tween.From", "target, props, seconds, curve?, options?", "Animate from props to the current values"},
                {"Tween.FromTo", "target, from, to, seconds, curve?, options?", "Animate between two sets of values"},
                {"Tween.Value", "from, to, seconds, curve, fn, options?", "Animate a plain number and call fn with it"},
                {"Tween.Along", "target, path, seconds, curve?, options?", "Move along a path or Vector entity outline"},
                {"Tween.Spring", "target, props, options?", "Physics-based motion with no duration"},
                {"Tween.Stagger", "targets, props, seconds, curve?, each?, options?", "Same tween on many entities, offset in time"},
                {"Tween.Sequence", "items, options?", "Run tweens, pauses and functions one after another"},
                {"Tween.Parallel", "items, options?", "Run tweens, pauses and functions together"},
                {"Tween.Delay", "seconds", "A pause for a sequence"},
                {"Tween.Call", "fn", "Call a function at this point of a sequence"},
                {"Tween.Kill", "target, property?", "Stop tweens on an entity"},
                {"Curves.Cubic", "x1, y1, x2, y2", "CSS style cubic bezier curve"},
                {"Curves.Interval", "begin, end, curve?", "Run a curve only between begin and end (0..1)"},
                {"Curves.Steps", "count", "Stair-step curve"},
                {"Curves.SawTooth", "count", "Repeating ramp"},
                {"Curves.Threshold", "at", "Jumps from 0 to 1 at a point"},
                {"Curves.Elastic", "mode, period?", "\"in\", \"out\" or \"inOut\" elastic curve"},
                {"Curves.Spring", "dampingRatio?, frequency?", "Damped spring curve"},
                {"Curves.Sine", "cycles?", "Sine wave that returns to 0 (shakes, pulses)"},
                {"Curves.Flipped", "curve", "Mirror a curve"},
                {"Curves.Reversed", "curve", "Play a curve backwards"},
                {"Curves.Evaluate", "curve, t", "Value of a curve at t (0..1)"},
                {":MorphTo", "path, seconds, curve?, options?", "Morph a Vector shape into another path"},
                {":Curve", "curve", "Set the curve of a tween handle"},
                {"Draw.Path", "path, x, y, options?", "Draw a Path with fill, stroke and trim"},
                {"Draw.Line", "x1, y1, x2, y2, width, r, g, b, a?", "Draw a line (inside OnRender)"},
                {"Draw.Circle", "x, y, radius, r, g, b, a?", "Draw a filled circle"},
                {"Draw.Rect", "x, y, w, h, r, g, b, a?, rotation?", "Draw a filled rectangle"},
                {"Draw.Text", "text, x, y, size, r, g, b, a?, align?", "Draw text"},
                {"Scene.Spawn", "name, x?, y?, parent?", "Create an entity"},
                {"Scene.Clone", "entityOrName, x?, y?", "Copy an entity and its children"},
                {"Scene.Find", "name", "First entity with that name, or nil"},
                {"Scene.FindAll", "name", "All entities with that name"},
                {"Audio.Play", "path, volume?, pitch?, loop?, bus?", "Play a sound, returns a voice id"},
                {"Audio.FadeTo", "voice, volume, seconds?, stopWhenSilent?", "Fade a voice"},
                {"Audio.SetBusVolume", "bus, volume", "Set the volume of sfx, music or ui"},
                {"Music.Play", "path, volume?, fadeIn?, fadeOutPrevious?, loop?", "Crossfade to a music track"},
                {"Music.Stop", "fadeOut?", "Fade the music out"},
                {"Game.Get", "name, default?", "Read a game variable"},
                {"Game.Set", "name, value", "Write a game variable"},
                {"Game.Add", "name, amount?", "Add to a game variable, returns the new value"},
                {"Game.Save", "slot?", "Write the game variables to disk"},
                {"Game.Load", "slot?", "Read the game variables from disk"},
                {"Math.Lerp", "a, b, t", "Linear interpolation"},
                {"Math.Clamp", "value, min, max", "Limit a value to a range"},
                {"Math.MoveTowards", "current, target, maxDelta", "Step toward a target without overshooting"},
                {"Math.Distance", "x1, y1, x2, y2", "Distance between two points"},
                {"Input.GetAxis", "negativeKey, positiveKey", "-1, 0 or 1"},
                {"Input.IsKeyDown", "key", "True while the key is held"},
                {"Input.IsKeyJustPressed", "key", "True on the frame the key goes down"},
            };
            std::vector<Signature> list;
            for (const Raw &r : raw)
            {
                Signature s;
                s.Callee = r.Callee;
                s.Summary = r.Summary;
                std::string params = r.Params;
                size_t from = 0;
                while (from <= params.size())
                {
                    size_t comma = params.find(", ", from);
                    s.Params.push_back(params.substr(from, comma == std::string::npos ? std::string::npos : comma - from));
                    if (comma == std::string::npos)
                        break;
                    from = comma + 2;
                }
                list.push_back(std::move(s));
            }
            return list;
        }
    }

    CallContext AnalyzeContext(const std::vector<std::string> &lines, int line, int column)
    {
        CallContext result;
        if (line < 0 || line >= static_cast<int>(lines.size()))
            return result;

        const int firstLine = std::max(0, line - 80);
        std::string text;
        for (int i = firstLine; i <= line; ++i)
        {
            const std::string &l = lines[static_cast<size_t>(i)];
            if (i == line)
                text += l.substr(0, std::min<size_t>(static_cast<size_t>(column), l.size()));
            else
            {
                text += l;
                text += '\n';
            }
        }

        std::vector<Frame> stack;
        enum class Mode
        {
            Normal,
            LineComment,
            BlockComment,
            String,
            LongString
        } mode = Mode::Normal;
        char quote = 0;
        int level = 0;
        std::string lastIdent;
        int stringStart = 0;
        size_t lineStart = 0;

        auto closeLongBracket = [&](size_t at, int wanted) -> bool
        {
            if (text[at] != ']')
                return false;
            size_t p = at + 1;
            int equals = 0;
            while (p < text.size() && text[p] == '=')
            {
                ++equals;
                ++p;
            }
            return equals == wanted && p < text.size() && text[p] == ']';
        };
        auto openLongBracket = [&](size_t at, int &outLevel) -> bool
        {
            if (text[at] != '[')
                return false;
            size_t p = at + 1;
            int equals = 0;
            while (p < text.size() && text[p] == '=')
            {
                ++equals;
                ++p;
            }
            if (p < text.size() && text[p] == '[')
            {
                outLevel = equals;
                return true;
            }
            return false;
        };

        for (size_t i = 0; i < text.size(); ++i)
        {
            const char c = text[i];

            if (i == lineStart && mode == Mode::Normal)
            {
                const std::string rest = text.substr(i, 14);
                auto starts = [&](const char *word)
                {
                    const size_t n = std::char_traits<char>::length(word);
                    return rest.compare(0, n, word) == 0 && (rest.size() == n || !IsIdent(rest[n]));
                };
                if (starts("function") || starts("end") || starts("return") || rest.compare(0, 14, "local function") == 0)
                    stack.clear();
            }

            if (mode == Mode::LineComment)
            {
                if (c == '\n')
                {
                    mode = Mode::Normal;
                    lineStart = i + 1;
                }
                continue;
            }
            if (mode == Mode::BlockComment || mode == Mode::LongString)
            {
                if (closeLongBracket(i, level))
                {
                    i += static_cast<size_t>(level) + 1;
                    mode = Mode::Normal;
                }
                else if (c == '\n')
                    lineStart = i + 1;
                continue;
            }
            if (mode == Mode::String)
            {
                if (c == '\\')
                    ++i;
                else if (c == quote)
                    mode = Mode::Normal;
                else if (c == '\n')
                {
                    mode = Mode::Normal;
                    lineStart = i + 1;
                }
                continue;
            }

            if (c == '\n')
            {
                lineStart = i + 1;
                continue;
            }
            if (c == '-' && i + 1 < text.size() && text[i + 1] == '-')
            {
                int blockLevel = 0;
                if (i + 2 < text.size() && openLongBracket(i + 2, blockLevel))
                {
                    mode = Mode::BlockComment;
                    level = blockLevel;
                    i += static_cast<size_t>(blockLevel) + 3;
                }
                else
                {
                    mode = Mode::LineComment;
                    ++i;
                }
                continue;
            }
            int longLevel = 0;
            if (c == '[' && openLongBracket(i, longLevel))
            {
                mode = Mode::LongString;
                level = longLevel;
                i += static_cast<size_t>(longLevel) + 1;
                continue;
            }
            if (c == '"' || c == '\'')
            {
                mode = Mode::String;
                quote = c;
                stringStart = static_cast<int>(i + 1 - lineStart);
                continue;
            }
            if (IsIdent(c) && !std::isdigit(static_cast<unsigned char>(c)))
            {
                size_t j = i;
                while (j < text.size() && IsIdent(text[j]))
                    ++j;
                lastIdent = text.substr(i, j - i);
                i = j - 1;
                continue;
            }
            if (c == '(' || c == '{' || c == '[')
            {
                Frame frame;
                frame.Open = c;
                if (c == '(')
                    CalleeBefore(text, i, frame.Callee, frame.Receiver);
                if (!stack.empty() && stack.back().Open == '(')
                {
                    frame.ParentIsCall = true;
                    frame.ParentCallee = stack.back().Callee;
                    frame.ParentArg = stack.back().Arg;
                }
                stack.push_back(std::move(frame));
                continue;
            }
            if (c == ')' || c == '}' || c == ']')
            {
                if (!stack.empty())
                    stack.pop_back();
                continue;
            }
            if (stack.empty())
                continue;
            Frame &top = stack.back();
            if (c == ',' || c == ';')
            {
                if (top.Open == '(')
                    ++top.Arg;
                else if (top.Open == '{')
                {
                    top.Key = true;
                    top.CurrentKey.clear();
                }
            }
            else if (c == '=' && top.Open == '{')
            {
                const char previous = i > 0 ? text[i - 1] : 0;
                const char next = i + 1 < text.size() ? text[i + 1] : 0;
                if (previous != '=' && previous != '~' && previous != '<' && previous != '>' && next != '=' && top.Key)
                {
                    top.Key = false;
                    top.CurrentKey = lastIdent;
                    top.Used.push_back(lastIdent);
                }
            }
        }

        if (mode == Mode::LineComment || mode == Mode::BlockComment)
        {
            result.InComment = true;
            return result;
        }
        if (mode == Mode::String)
        {
            result.InString = true;
            result.Quote = quote;
            result.StringStart = stringStart;
        }
        if (mode == Mode::LongString)
        {
            result.InString = true;
            result.Quote = 0;
        }

        for (auto it = stack.rbegin(); it != stack.rend(); ++it)
            if (it->Open == '(')
            {
                result.InCall = true;
                result.SigCallee = it->Callee;
                result.SigReceiver = it->Receiver;
                result.SigArg = it->Arg;
                break;
            }

        if (!stack.empty())
        {
            const Frame &top = stack.back();
            if (top.Open == '(')
            {
                result.DirectArg = true;
                result.ArgCallee = top.Callee;
                result.ArgIndex = top.Arg;
            }
            else if (top.Open == '{')
            {
                result.InTable = true;
                result.KeyPosition = top.Key;
                result.TableKey = top.CurrentKey;
                result.UsedKeys = top.Used;
                result.TableInCall = top.ParentIsCall;
                result.TableCallee = top.ParentCallee;
                result.TableArg = top.ParentArg;
            }
        }
        return result;
    }

    ArgRole ResolveRole(const CallContext &context)
    {
        if (context.InComment)
            return ArgRole::None;

        if (context.DirectArg)
        {
            if (const FunctionSpec *spec = FindSpec(context.ArgCallee))
            {
                if (spec->CurveArg == context.ArgIndex)
                    return ArgRole::Curve;
                if (spec->PropertyNameArg == context.ArgIndex)
                    return ArgRole::PropertyName;
                if (spec->BusArg == context.ArgIndex)
                    return ArgRole::BusName;
            }
            return ArgRole::None;
        }

        if (context.InTable && context.TableInCall && context.KeyPosition && !context.InString)
        {
            if (const FunctionSpec *spec = FindSpec(context.TableCallee))
            {
                if (std::find(spec->PropsArgs.begin(), spec->PropsArgs.end(), context.TableArg) != spec->PropsArgs.end())
                    return ArgRole::PropertyKeys;
                if (spec->OptionsFrom >= 0 && context.TableArg >= spec->OptionsFrom)
                    return spec->OptionsRole;
            }
        }
        return ArgRole::None;
    }

    namespace
    {
        struct GeneratedEntry
        {
            const char *Key;
            const char *Params;
        };

        const GeneratedEntry GeneratedTable[] = {
#include "LuaSignatureData.inc"
        };

        std::vector<std::string> SplitParams(const std::string &params)
        {
            std::vector<std::string> list;
            if (params.empty())
                return list;
            size_t from = 0;
            while (from <= params.size())
            {
                const size_t comma = params.find(", ", from);
                list.push_back(params.substr(from, comma == std::string::npos ? std::string::npos : comma - from));
                if (comma == std::string::npos)
                    break;
                from = comma + 2;
            }
            return list;
        }

        struct GeneratedIndex
        {
            std::unordered_map<std::string, std::string> Exact;
            std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> Methods;

            GeneratedIndex()
            {
                for (const GeneratedEntry &entry : GeneratedTable)
                {
                    const std::string key = entry.Key;
                    Exact[key] = entry.Params;
                    const size_t colon = key.find(':');
                    if (colon != std::string::npos)
                        Methods[key.substr(colon + 1)].emplace_back(key.substr(0, colon), entry.Params);
                }
            }
        };

        const GeneratedIndex &Generated()
        {
            static const GeneratedIndex index;
            return index;
        }

        const Signature *Cached(const std::string &cacheKey, const std::string &callee, const std::string &params, const std::string &summary)
        {
            static std::unordered_map<std::string, Signature> cache;
            auto it = cache.find(cacheKey);
            if (it == cache.end())
            {
                Signature s;
                s.Callee = callee;
                s.Params = SplitParams(params);
                s.Summary = summary;
                it = cache.emplace(cacheKey, std::move(s)).first;
            }
            return &it->second;
        }
    }

    const Signature *FindSignature(const std::string &callee, const std::string &receiverType)
    {
        static const std::vector<Signature> signatures = BuildSignatures();
        for (const Signature &s : signatures)
        {
            if (s.Callee == callee)
                return &s;
            if (s.Callee[0] == ':' && callee.size() >= s.Callee.size() && callee.compare(callee.size() - s.Callee.size(), s.Callee.size(), s.Callee) == 0)
                return &s;
        }

        if (callee.empty())
            return nullptr;
        const GeneratedIndex &index = Generated();

        if (callee[0] != ':')
        {
            auto it = index.Exact.find(callee);
            if (it == index.Exact.end())
                return nullptr;
            return Cached(callee, callee, it->second, std::string());
        }

        const std::string method = callee.substr(1);
        auto candidates = index.Methods.find(method);
        if (candidates == index.Methods.end())
            return nullptr;
        const auto &list = candidates->second;

        if (!receiverType.empty())
            for (const auto &[type, params] : list)
                if (type == receiverType)
                    return Cached(receiverType + ":" + method, receiverType + ":" + method, params, std::string());

        bool sameParams = true;
        for (const auto &candidate : list)
            if (candidate.second != list.front().second)
                sameParams = false;

        std::string types;
        size_t shown = 0;
        for (const auto &candidate : list)
        {
            if (shown++ < 4)
                types += (types.empty() ? "" : ", ") + candidate.first;
        }
        if (list.size() > 4)
            types += ", ...";

        const auto *chosen = &list.front();
        for (const auto &candidate : list)
            if (candidate.first == "Entity")
                chosen = &candidate;
        const std::string summary = sameParams ? "on " + types : chosen->first + "; other types differ (" + types + ")";
        return Cached("?" + method + "|" + summary, ":" + method, chosen->second, summary);
    }

    std::vector<std::pair<std::string, std::string>> ListSignatures()
    {
        std::vector<std::pair<std::string, std::string>> result;
        for (const GeneratedEntry &entry : GeneratedTable)
            result.emplace_back(entry.Key, entry.Params);
        return result;
    }

    const std::vector<OptionKey> &OptionKeysFor(ArgRole role)
    {
        static const std::vector<OptionKey> tween = {
            {"delay", "seconds before it starts", nullptr},
            {"loop", "extra plays (true = forever)", nullptr},
            {"yoyo", "play back and forth", nullptr},
            {"speed", "playback speed multiplier", nullptr},
            {"onStart", "callback when it starts", "onStart = function() end"},
            {"onUpdate", "callback every frame (eased value)", "onUpdate = function(eased) end"},
            {"onComplete", "callback when it finishes", "onComplete = function() end"},
            {"onRepeat", "callback on every repeat", "onRepeat = function() end"},
        };
        static const std::vector<OptionKey> spring = {
            {"stiffness", "spring strength (default 180)", nullptr},
            {"damping", "how fast it settles (default 14)", nullptr},
            {"mass", "heavier is slower", nullptr},
            {"threshold", "stop when this close to the target", nullptr},
            {"onComplete", "callback when it settles", "onComplete = function() end"},
        };
        static const std::vector<OptionKey> along = {
            {"orient", "rotate to face the direction of travel", nullptr},
            {"orientOffset", "extra rotation in degrees", nullptr},
            {"relative", "path is relative to the start position", nullptr},
        };
        static const std::vector<OptionKey> drawPath = {
            {"fill", "{r, g, b, a} or false", nullptr},
            {"stroke", "{r, g, b, a}", nullptr},
            {"width", "stroke width", nullptr},
            {"cap", "butt, round or square", nullptr},
            {"join", "miter, round or bevel", nullptr},
            {"rotation", "degrees", nullptr},
            {"scale", "scale factor", nullptr},
            {"trim", "{start, end, offset}", nullptr},
        };
        static const std::vector<OptionKey> none;
        switch (role)
        {
        case ArgRole::TweenOptionKeys:
            return tween;
        case ArgRole::SpringOptionKeys:
            return spring;
        case ArgRole::AlongOptionKeys:
            return along;
        case ArgRole::DrawPathOptionKeys:
            return drawPath;
        default:
            return none;
        }
    }

    bool IsCurveName(const std::string &name)
    {
        SIMPEngine::Curve curve;
        return !name.empty() && SIMPEngine::Curve::FromName(name, curve);
    }
}
