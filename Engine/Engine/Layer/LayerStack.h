#pragma once
#include <vector>
#include <algorithm>
#include "Layer.h"

namespace SIMPEngine
{
    class LayerStack
    {
    public:
        LayerStack() {}
        ~LayerStack()
        {
            for (Layer *layer : m_Layers)
            {
                delete layer;
            }
        }

        void PushLayer(Layer *layer)
        {
            m_Layers.insert(m_Layers.begin() + m_LayerInsertIndex, layer);
            m_LayerInsertIndex++;
            layer->OnAttach();
        }

        void PushOverlay(Layer *overlay)
        {
            m_Layers.emplace_back(overlay);
            overlay->OnAttach();
        }

        void PopLayer(Layer *layer)
        {
            auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
            if (it != m_Layers.end())
            {
                layer->OnDetach();
                m_Layers.erase(it);
                m_LayerInsertIndex--;
            }
        }

        void PopOverlay(Layer *overlay)
        {
            auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
            if (it != m_Layers.end())
            {
                overlay->OnDetach();
                m_Layers.erase(it);
            }
        }

        std::vector<Layer *>::iterator begin() { return m_Layers.begin(); }
        std::vector<Layer *>::iterator end() { return m_Layers.end(); }

    private:
        std::vector<Layer *> m_Layers;
        unsigned int m_LayerInsertIndex = 0;
    };
} // namespace SIMPEngine
