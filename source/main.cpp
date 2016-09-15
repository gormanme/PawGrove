#include <windows.h>

//Function declarations:
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow);



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
    //Handle for the window
    HWND hWnd = {};

    //Holds information for the window class
    WNDCLASSEX wc = {};

    //Windows event messages struct
    MSG msg = {};

    //Fill in the dinwo class with needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"WindowClass1";

    //Register the window class
    RegisterClassEx(&wc);

    //Create the window and use the result as the handle
    hWnd = CreateWindowEx(NULL,
        L"WindowClass1",    // name of the window class
        L"PawGrove",   // title of the window
        WS_OVERLAPPEDWINDOW,    // window style
        300,    // x-position of the window
        300,    // y-position of the window
        500,    // width of the window
        400,    // height of the window
        NULL,    // we have no parent window, NULL
        NULL,    // we aren't using menus, NULL
        hInstance,    // application handle
        NULL);    // used with multiple windows, NULL

    //Display the window on the screen
    ShowWindow(hWnd, nCmdShow);

    //Main loop:
    //Wait for the next message in the queue, store the result in 'msg'
    while (GetMessage(&msg, NULL, 0, 0))
    {
        //Translate keystroke messages into the right format
        TranslateMessage(&msg);

        //Send the message to the WindowProc function
        DispatchMessage(&msg);
    }

    //Return this part of the WM_QUIT message to Windows
    return (int) msg.wParam;
}


//Main message handler 
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //Sort through and find what code to run for the message given
    switch (message)
    {
        //This message is read when the window is closed
        case WM_DESTROY:
        {
            //Close the application entirely
            PostQuitMessage(0);
            return 0;
        }
        break;
    }

    //Handle any messages the switch statement didn't
    return DefWindowProc(hWnd, message, wParam, lParam);
}