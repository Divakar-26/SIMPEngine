#include "Input/Input.h"
#include "Log.h"

//this is Input class which will be used for our actual game, this prevent us to call dispatcher everytime. so we can just execute or trigger something based on this class but the events will be delived to this class via our event system
namespace SIMPEngine
{
    
    //holds all the keyPressed
    std::unordered_set<int> Input::s_KeysPressed;

    //holds mouse presses and mouse corrdinates
    std::unordered_set<int> Input::s_MouseButtonsPressed;
    int Input::s_MouseX = 0;
    int Input::s_MouseY = 0;

    //returns ture is the keycode is found in the set. we can use it like if(Input::IsKeyPressed(KEYCODE));
    bool Input::IsKeyPressed(int keycode)
    {
        // CORE_INFO("Checking key {} pressed? {}", keycode, pressed);
        return s_KeysPressed.find(keycode) != s_KeysPressed.end();
    }

    //same as keys, it return true if mouse button press is found in the set
    bool Input::IsMouseButtonPressed(int button)
    {
        return s_MouseButtonsPressed.find(button) != s_MouseButtonsPressed.end();
    }


    //returns the mouse coordinates
    std::pair<int, int> Input::GetMousePosition()
    {
        return {s_MouseX, s_MouseY};
    }

    //this will be called by our own event system like this. If keyPressedEvent happen the dispatcher give this function the keycode, it will save it in the set and we can use it whenever we want. However as soon as we release the key it erases from the set.
    void Input::OnKeyPressed(int keycode)
    {
        s_KeysPressed.insert(keycode);
        // CORE_INFO("Key pressed: {}", keycode);
    }


    //OnKeyReleased OnMouseButtonPressed and similar functions works like it's keycode counterpart
    void Input::OnKeyReleased(int keycode)
    {
        s_KeysPressed.erase(keycode);
    }

    void Input::OnMouseButtonPressed(int button)
    {
        s_MouseButtonsPressed.insert(button);
    }

    void Input::OnMouseButtonReleased(int button)
    {
        s_MouseButtonsPressed.erase(button);
    }

    void Input::OnMouseMoved(int x, int y)
    {
        s_MouseX = x;
        s_MouseY = y;
    }

    //make the set empty
    void Input::ResetAllKeys()
    {
        s_KeysPressed.clear();

    }
} // namespace SIMPEngine
