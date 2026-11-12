#pragma once

#include <string>
#include <vector>

namespace ScriptEditor
{
    enum class ArgRole
    {
        None,
        Curve,
        PropertyKeys,
        TweenOptionKeys,
        SpringOptionKeys,
        AlongOptionKeys,
        DrawPathOptionKeys,
        PropertyName,
        BusName
    };

    struct CallContext
    {
        bool InComment = false;
        bool InString = false;
        char Quote = 0;
        int StringStart = 0;

        bool InTable = false;
        bool KeyPosition = false;
        std::string TableKey;
        std::vector<std::string> UsedKeys;
        bool TableInCall = false;
        std::string TableCallee;
        int TableArg = 0;

        bool DirectArg = false;
        std::string ArgCallee;
        int ArgIndex = 0;

        bool InCall = false;
        std::string SigCallee;
        std::string SigReceiver;
        int SigArg = 0;
    };

    struct Signature
    {
        std::string Callee;
        std::vector<std::string> Params;
        std::string Summary;
    };

    struct OptionKey
    {
        const char *Name;
        const char *Detail;
        const char *Insert;
    };

    CallContext AnalyzeContext(const std::vector<std::string> &lines, int line, int column);
    ArgRole ResolveRole(const CallContext &context);

    const Signature *FindSignature(const std::string &callee, const std::string &receiverType = {});
    std::vector<std::pair<std::string, std::string>> ListSignatures();
    const std::vector<OptionKey> &OptionKeysFor(ArgRole role);
    bool IsCurveName(const std::string &name);
}
