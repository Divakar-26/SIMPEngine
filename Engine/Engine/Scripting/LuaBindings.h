#pragma once

struct lua_State;

namespace SIMPEngine
{
    class Entity;
    class Scene;

    namespace LuaBindings
    {
        void Register(lua_State *L);
        void PushEntity(lua_State *L, const Entity &entity);
        void SetActiveScene(Scene *scene);
        Scene *GetActiveScene();

        class SceneScope
        {
        public:
            explicit SceneScope(Scene *scene) : m_Previous(GetActiveScene())
            {
                SetActiveScene(scene);
            }

            ~SceneScope()
            {
                SetActiveScene(m_Previous);
            }

            SceneScope(const SceneScope &) = delete;
            SceneScope &operator=(const SceneScope &) = delete;

        private:
            Scene *m_Previous;
        };
    }
}
