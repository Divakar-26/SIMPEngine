#pragma once
#include <Engine/Layer/Layer.h>
#include <algorithm>
#include <vector>

namespace SIMPEngine
{   

    //this is actual layerstack which will be owned by our application.
    //this is where we will push our layer
    class LayerStack
    {
    public:
        LayerStack() {}
        ~LayerStack()
        {
            //delete the layer
            for (Layer *layer : m_Layers)
            {
                delete layer;
            }
        }

        // Push the layer on layerStack and increment the index and also call the OnAttach so that we don't have to call it manually. We will only call OnUpdate, OnRender and OnEvent manually
        void PushLayer(Layer *layer)
        {
            m_Layers.insert(m_Layers.begin() + m_LayerInsertIndex, layer);
            m_LayerInsertIndex++;
            layer->OnAttach();
        }

        //this will push the given layer at the end, like overlays(UI, HUD etc)
        void PushOverlay(Layer *overlay)
        {
            m_Layers.emplace_back(overlay);
            overlay->OnAttach();
        }

        //pops the layer and decremenet the index value
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

        //pops the last layer
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
        
        //actual layer stack
        std::vector<Layer *> m_Layers;
        unsigned int m_LayerInsertIndex = 0;
    };
} // namespace SIMPEngine
