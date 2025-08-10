    #include "Engine/Application.h"
    #include "Engine/Log.h"
    #include "Engine/LayerStack.h"
    #include "Engine/ImGuiLayer.h"
    #include "Engine/RenderingLayer.h"

    class MyGame : public SIMPEngine::Application {
    public:
        MyGame() {
            // Custom game initialization
            PushOverlay(new SIMPEngine::RenderingLayer()); // Add it here only if needed
            PushOverlay(new SIMPEngine::ImGuiLayer()); // Add it here only if needed
        }
    };

    SIMPEngine::Application* SIMPEngine::CreateApplication() {
        return new MyGame();
    }

    int main() {

        SIMPEngine::Log::Init();

        CORE_INFO("Engine Starting up....");
        auto app = SIMPEngine::CreateApplication();
        app->Run();
        delete app;
    }
