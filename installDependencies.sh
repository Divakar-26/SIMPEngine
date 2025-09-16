mkdir vendor
cd vendor
git clone https://github.com/libsdl-org/SDL.git
git clone https://github.com/libsdl-org/SDL_image.git
git clone https://github.com/gabime/spdlog.git
git clone https://github.com/skypjack/entt.git
git clone https://github.com/g-truc/glm.git
git clone  --branch docking --single-branch https://github.com/ocornut/imgui.git "$TARGET_DIR"
git clone https://github.com/CedricGuillemet/ImGuizmo
git clone https://github.com/jbeder/yaml-cpp