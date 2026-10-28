#pragma once
#include <Engine/Layer/Layer.h>
#include <algorithm>
#include <vector>
#include <memory>

namespace SIMPEngine
{   

    //this is actual layerstack which will be owned by our application.
    //this is where we will push our layer
    class LayerStack
    { 
    public:
        LayerStack() {}
        ~LayerStack() = default;

        // Push the layer on layerStack and increment the index and also call the OnAttach so that we don't have to call it manually. We will only call OnUpdate, OnRender and OnEvent manually
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

        //this will push the given layer at the end, like overlays(UI, HUD etc)
        void PushOverlay(std::unique_ptr<Layer> overlay)
        {
            Layer *ptr = overlay.get();

            m_Layers.emplace_back(std::move(overlay));

            ptr->OnAttach();
        }

        //pops the layer and decremenet the index value
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

        //pops the last layer
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
        
        //actual layer stack
        std::vector<std::unique_ptr<Layer>> m_Layers;
        unsigned int m_LayerInsertIndex = 0;
    };
} // namespace SIMPEngine
