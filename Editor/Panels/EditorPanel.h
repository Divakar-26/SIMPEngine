#pragma once

#include <Engine/Events/Event.h>

class EditorPanel
{
public:
    virtual ~EditorPanel() = default;

    virtual void OnAttach() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnImGuiRender() = 0;
    virtual void OnEvent(SIMPEngine::Event& e) {}
};