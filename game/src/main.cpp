#include "Engine/Application.h"

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
    auto app = SIMPEngine::CreateApplication();
    app->Run();
    delete app;
}
