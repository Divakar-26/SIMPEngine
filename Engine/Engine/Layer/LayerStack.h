#pragma once
#include <Engine/Layer/Layer.h>
#include <algorithm>
#include <vector>
#include <memory>

namespace SIMPEngine
{
    class LayerStack
    {
    public:
        LayerStack() {}
        ~LayerStack() { Clear(); }

        void Clear()
        {
            for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it)
            {
                if (*it)
                    (*it)->OnDetach();
            }
            m_Layers.clear();
            m_LayerInsertIndex = 0;
        }

        void PushLayer(std::unique_ptr<Layer> layer)
        {
            Layer *ptr = layer.get();

            m_Layers.insert(
                m_Layers.begin() + m_LayerInsertIndex,
                std::move(layer)
            );

            ++m_LayerInsertIndex;

            ptr->OnAttach();
        }

        void PushOverlay(std::unique_ptr<Layer> overlay)
        {
            Layer *ptr = overlay.get();

            m_Layers.emplace_back(std::move(overlay));

            ptr->OnAttach();
        }

        void PopLayer(Layer *layer)
        {
            auto it = std::find_if(
                m_Layers.begin(),
                m_Layers.end(),
                [layer](const std::unique_ptr<Layer> &ptr)
                {
                    return ptr.get() == layer;
                }
            );

            if (it != m_Layers.end())
            {
                (*it)->OnDetach();

                m_Layers.erase(it);

                --m_LayerInsertIndex;
            }
        }

        void PopOverlay(Layer *overlay)
        {
            auto it = std::find_if(
                m_Layers.begin(),
                m_Layers.end(),
                [overlay](const std::unique_ptr<Layer> &ptr)
                {
                    return ptr.get() == overlay;
                }
            );

            if (it != m_Layers.end())
            {
                (*it)->OnDetach();

                m_Layers.erase(it);
            }
        }

        auto begin() { return m_Layers.begin(); }
        auto end() { return m_Layers.end(); }

    private:

        std::vector<std::unique_ptr<Layer>> m_Layers;
        unsigned int m_LayerInsertIndex = 0;
    };
}
