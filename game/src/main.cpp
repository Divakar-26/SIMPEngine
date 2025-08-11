    #include "Application/Application.h"
    #include "Log.h"
    #include "Layer/LayerStack.h"
    #include "UI/ImGuiLayer.h"
    #include "Layer/RenderingLayer.h"

    class MyGame : public SIMPEngine::Application {
    public:
        MyGame() {
            // Custom game initialization
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
