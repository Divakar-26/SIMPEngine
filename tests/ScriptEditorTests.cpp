#include "TestFramework.h"

#include <LuaCompletion.h>
#include <LuaContext.h>
#include <LuaNavigation.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

using namespace ScriptEditor;

namespace
{
    std::vector<std::string> Split(const std::string &text)
    {
        std::vector<std::string> lines;
        std::stringstream stream(text);
        std::string line;
        while (std::getline(stream, line))
            lines.push_back(line);
        if (lines.empty() || (!text.empty() && text.back() == '\n'))
            lines.emplace_back();
        return lines;
    }

    struct Cursor
    {
        std::vector<std::string> Lines;
        int Line = 0;
        int Column = 0;
    };

    Cursor At(const std::string &textWithBar)
    {
        const size_t bar = textWithBar.find('|');
        std::string text = textWithBar;
        text.erase(bar, 1);
        Cursor c;
        c.Lines = Split(text);
        int line = 0;
        size_t lineStart = 0;
        for (size_t i = 0; i < bar; ++i)
            if (text[i] == '\n')
            {
                ++line;
                lineStart = i + 1;
            }
        c.Line = line;
        c.Column = static_cast<int>(bar - lineStart);
        return c;
    }

    CallContext Analyze(const std::string &textWithBar)
    {
        Cursor c = At(textWithBar);
        return AnalyzeContext(c.Lines, c.Line, c.Column);
    }

    bool Has(const CompletionResult &r, const std::string &label)
    {
        return std::any_of(r.Items.begin(), r.Items.end(), [&](const CompletionItem &i)
                           { return i.Label == label; });
    }

    CompletionResult Query(LuaCompletion &completion, const std::string &textWithBar, bool force = false)
    {
        Cursor c = At(textWithBar);
        return completion.Query(c.Lines, c.Line, c.Column, force);
    }
}

TEST_CASE("script context: finds the call, argument and table position")
{
    CallContext a = Analyze("Tween.To(self.entity, {|");
    CHECK(a.InTable && a.KeyPosition && a.TableInCall);
    CHECK(a.TableCallee == "Tween.To" && a.TableArg == 1);
    CHECK(ResolveRole(a) == ArgRole::PropertyKeys);

    CallContext b = Analyze("Tween.To(self.entity, {x = 5, |");
    CHECK(b.KeyPosition);
    CHECK(b.UsedKeys.size() == 1 && b.UsedKeys[0] == "x");

    CallContext c = Analyze("Tween.To(self.entity, {x = |");
    CHECK(!c.KeyPosition);
    CHECK(ResolveRole(c) == ArgRole::None);

    CallContext d = Analyze("Tween.To(e, {x = 1}, 0.5, |");
    CHECK(d.DirectArg && d.ArgIndex == 3);
    CHECK(ResolveRole(d) == ArgRole::Curve);

    CallContext e = Analyze("Tween.To(e, {x = 1}, 0.5, \"ease|\"");
    CHECK(e.InString);
    CHECK(ResolveRole(e) == ArgRole::Curve);

    CallContext f = Analyze("Tween.To(e, {x = 1}, 0.5, \"easeOut\", {|");
    CHECK(ResolveRole(f) == ArgRole::TweenOptionKeys);

    CallContext g = Analyze("Tween.Spring(e, {x = 0}, {|");
    CHECK(ResolveRole(g) == ArgRole::SpringOptionKeys);

    CallContext h = Analyze("Tween.To(e, {position = {|");
    CHECK(ResolveRole(h) == ArgRole::None);

    CallContext i = Analyze("-- Tween.To(e, {|");
    CHECK(i.InComment);

    CallContext j = Analyze("local s = \"Tween.To(e, {\"\nfoo(|");
    CHECK(j.SigCallee == "foo");

    CallContext k = Analyze("vector:MorphTo(Path.Heart(10), 0.5, |");
    CHECK(ResolveRole(k) == ArgRole::Curve);

    CallContext l = Analyze("Tween.To(e,\n    {\n        x = 1,\n        |");
    CHECK(l.KeyPosition && ResolveRole(l) == ArgRole::PropertyKeys);

    CallContext m = Analyze("function Foo:OnUpdate(dt)\n    Tween.To(e, {x = 1})\nend\n\nTween.Kill(e, \"|");
    CHECK(ResolveRole(m) == ArgRole::PropertyName);
}

TEST_CASE("script completion: Tween property keys, options, curves and strings")
{
    LuaCompletion completion;

    CompletionResult keys = Query(completion, "function S:OnCreate()\n    Tween.To(self.entity, {tr|");
    CHECK(Has(keys, "trimEnd") && Has(keys, "trimStart") && Has(keys, "trimOffset"));
    CHECK(keys.ForceOpen);
    for (const CompletionItem &item : keys.Items)
        if (item.Label == "trimEnd")
            CHECK(item.Insert == "trimEnd = ");

    CompletionResult empty = Query(completion, "Tween.To(self.entity, {|");
    CHECK(Has(empty, "x") && Has(empty, "alpha") && Has(empty, "fillColor"));

    CompletionResult unused = Query(completion, "Tween.To(self.entity, {x = 1, |");
    CHECK(!Has(unused, "x") && Has(unused, "y"));

    CompletionResult options = Query(completion, "Tween.To(e, {x = 1}, 1, \"easeOut\", {|");
    CHECK(Has(options, "delay") && Has(options, "yoyo") && Has(options, "onComplete") && !Has(options, "stiffness"));

    CompletionResult curves = Query(completion, "Tween.To(e, {x = 1}, 1, easeInO|");
    CHECK(Has(curves, "easeInOut") && Has(curves, "easeInOutBack"));
    for (const CompletionItem &item : curves.Items)
        if (item.Label == "easeInOut")
        {
            CHECK(item.Curve);
            CHECK(item.Insert == "\"easeInOut\"");
        }

    CompletionResult inString = Query(completion, "Tween.To(e, {x = 1}, 1, \"bounce|\")");
    CHECK(inString.InString);
    CHECK(Has(inString, "bounceOut") && Has(inString, "bounceIn"));
    for (const CompletionItem &item : inString.Items)
        CHECK(item.Insert.empty());

    CompletionResult otherString = Query(completion, "log.info(\"bounce|\")");
    CHECK(otherString.Items.empty());

    CompletionResult bus = Query(completion, "Audio.Play(\"a.wav\", 1, 1, false, \"|\")", true);
    CHECK(Has(bus, "music") && Has(bus, "sfx") && Has(bus, "ui"));

    CompletionResult normal = Query(completion, "local speed = 3\nlocal x = spe|");
    CHECK(Has(normal, "speed"));

    CompletionResult comment = Query(completion, "-- Tween.To(e, {tr|");
    CHECK(comment.Items.empty());
}

TEST_CASE("script signatures: known engine calls have parameter lists")
{
    const Signature *to = FindSignature("Tween.To");
    CHECK(to != nullptr);
    if (to)
    {
        CHECK(to->Params.size() == 5);
        CHECK(to->Params[1] == "props");
    }
    CHECK(FindSignature("entity:GetVector():MorphTo") != nullptr);
    CHECK(FindSignature("Nothing.Here") == nullptr);
    CHECK(IsCurveName("easeOutBack") && IsCurveName("EASEOUTBACK") && !IsCurveName("notACurve"));
}

TEST_CASE("script navigation: locals, functions, parameters, self methods and require")
{
    const std::string source =
        "local Util = require(\"util\")\n"
        "local speed = 5\n"
        "local function helper(a, b)\n"
        "    return a + b + speed\n"
        "end\n"
        "\n"
        "local Script = {}\n"
        "\n"
        "function Script:OnUpdate(dt)\n"
        "    local total = helper(dt, speed)\n"
        "    self:Fire(total)\n"
        "    Util.double(total)\n"
        "end\n"
        "\n"
        "function Script:Fire(amount)\n"
        "    print(amount)\n"
        "end\n";
    const std::vector<std::string> lines = Split(source);

    ModuleLoader loader = [](const std::string &module, std::string &vpath, std::vector<std::string> &moduleLines)
    {
        if (module != "util")
            return false;
        vpath = "assets://scripts/util.lua";
        moduleLines = Split("local M = {}\n\nfunction M.double(x)\n    return x * 2\nend\n\nreturn M\n");
        return true;
    };
    auto describe = [](const std::string &, const std::string &) { return std::string(); };

    auto column = [&](int line, const std::string &word, int occurrence = 0)
    {
        size_t at = std::string::npos;
        for (int i = 0; i <= occurrence; ++i)
            at = lines[static_cast<size_t>(line)].find(word, at == std::string::npos ? 0 : at + 1);
        return static_cast<int>(at);
    };

    Definition helper = ResolveSymbol(lines, 9, column(9, "helper") + 2, loader, describe);
    CHECK(helper.Found && helper.Line == 2 && helper.VPath.empty());

    Definition speedUse = ResolveSymbol(lines, 3, column(3, "speed") + 1, loader, describe);
    CHECK(speedUse.Found && speedUse.Line == 1);

    Definition parameter = ResolveSymbol(lines, 3, column(3, "a") , loader, describe);
    CHECK(parameter.Found && parameter.Line == 2);

    Definition total = ResolveSymbol(lines, 10, column(10, "total") + 1, loader, describe);
    CHECK(total.Found && total.Line == 9);

    Definition method = ResolveSymbol(lines, 10, column(10, "Fire") + 1, loader, describe);
    CHECK(method.Found && method.Line == 14 && method.VPath.empty());

    Definition external = ResolveSymbol(lines, 11, column(11, "double") + 1, loader, describe);
    CHECK(external.Found && external.VPath == "assets://scripts/util.lua" && external.Line == 2);

    Definition moduleString = ResolveSymbol(lines, 0, column(0, "util") + 1, loader, describe);
    CHECK(moduleString.Found && moduleString.VPath == "assets://scripts/util.lua");

    Definition alias = ResolveSymbol(lines, 11, column(11, "Util") + 1, loader, describe);
    CHECK(alias.Found && alias.Line == 0);

    Definition atDeclaration = ResolveSymbol(lines, 2, column(2, "helper") + 1, loader, describe);
    CHECK(!atDeclaration.Found);

    Definition keyword = ResolveSymbol(lines, 3, column(3, "return") + 1, loader, describe);
    CHECK(!keyword.Found);

    Definition inString = ResolveSymbol(Split("local a = \"speed\"\nlocal speed = 1\n"), 0, 13, loader, describe);
    CHECK(!inString.Found);
}

TEST_CASE("script signatures: every registered Lua function has one")
{
    LuaCompletion completion;
    completion.RefreshApi();

    std::vector<std::string> missing;
    int checked = 0;
    for (const auto &[table, items] : completion.Tables())
        for (const CompletionItem &item : items)
            if (item.Callable)
            {
                ++checked;
                if (!FindSignature(table + "." + item.Label))
                    missing.push_back(table + "." + item.Label);
            }
    for (const auto &[type, items] : completion.Types())
        for (const CompletionItem &item : items)
        {
            ++checked;
            if (!FindSignature(":" + item.Label, type))
                missing.push_back(type + ":" + item.Label);
        }
    for (const CompletionItem &global : completion.Globals())
        if (global.Callable)
        {
            ++checked;
            if (!FindSignature(global.Label))
                missing.push_back(global.Label);
        }

    CHECK(checked > 500);
    for (const std::string &name : missing)
        SIMPTest::Fail(__FILE__, __LINE__, "no signature for " + name + " (run python3 Editor/ScriptEditor/tools/gen_signatures.py)");
}

TEST_CASE("script signatures: lookups are type aware and use real parameter names")
{
    const Signature *tint = FindSignature(":SetTint", "Sprite");
    CHECK(tint != nullptr);
    if (tint)
    {
        CHECK(tint->Params.size() == 4);
        CHECK(tint->Params[0] == "r" && tint->Params[3] == "a?");
    }

    const Signature *play = FindSignature("Audio.Play");
    CHECK(play != nullptr && play->Params.size() == 5 && play->Params[0] == "path");
    const Signature *handlePlay = FindSignature(":Play", "Audio");
    CHECK(handlePlay != nullptr && handlePlay->Params.empty());

    const Signature *floor = FindSignature("math.floor");
    CHECK(floor != nullptr && floor->Params.size() == 1 && floor->Params[0] == "x");
    const Signature *format = FindSignature("string.format");
    CHECK(format != nullptr && format->Params.size() == 2 && format->Params[1] == "...");
    CHECK(FindSignature("print") != nullptr);

    const Signature *anyType = FindSignature(":SetSize");
    CHECK(anyType != nullptr && anyType->Params.size() == 2);

    CallContext chained = Analyze("self.entity:GetSprite():SetTint(255, |");
    CHECK(chained.SigCallee == ":SetTint");
    CHECK(chained.SigReceiver == "self.entity:GetSprite()");
    CHECK(chained.SigArg == 1);

    CallContext plain = Analyze("math.floor(|");
    CHECK(plain.SigCallee == "math.floor" && plain.SigReceiver.empty());

    CallContext handle = Analyze("sprite:SetTint(|");
    CHECK(handle.SigCallee == ":SetTint" && handle.SigReceiver == "sprite");
}
