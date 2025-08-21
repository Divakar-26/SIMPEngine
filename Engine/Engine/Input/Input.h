#pragma once

#include<unordered_set>


//this input class will be used as Input in owr game 
namespace SIMPEngine{
    class Input{
        public:
            //all the functions are static so that we can use this without making it's object

            //these return true if the key or button is pressed
            static bool IsKeyPressed(int keyCode);
            static bool IsMouseButtonPressed(int button);

            //return the position of mouse from the screen
            static std::pair<int,int> GetMousePosition();

            //these fuction will be used for inserting the keyValues. we will hook this up using our EventSystem
            static void OnKeyPressed(int keyCode);
            static void OnKeyReleased(int keyCode);
            static void OnMouseButtonPressed(int button);
            static void OnMouseButtonReleased(int button);
            static void OnMouseMoved(int x, int y);

            // a helper function to clear the keys and mouse Inputs
            static void ResetAllKeys();
        private:

            //these are set which hold the keycode of keys and mouseButton
            static std::unordered_set<int> s_KeysPressed;
            static std::unordered_set<int> s_MouseButtonsPressed;
            static int s_MouseX, s_MouseY;
    };
}