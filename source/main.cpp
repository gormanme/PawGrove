#include <windows.h>
#include <d3d11.h>

#pragma comment (lib, "d3d11.lib")

//Global declarations
IDXGISwapChain *swapChain = {};				//Pointer to swap chain interface
ID3D11Device *device = {};					//Pointer to Direct3D device interface
ID3D11DeviceContext *deviceContext = {};	//Pointer to Direct3D device context
ID3D11RenderTargetView *backBuffer = {};	//Pointer to the back buffer

//Function declarations:
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow);

void InitD3D(HWND hWnd);			//Sets up and initializes Direct3D
void RenderFrame();					//Renders a single frame
void CleanD3D();					//Closes Direct3D and releases memory


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
    //Handle for the window
    HWND hWnd = {};

    //Holds information for the window class
    WNDCLASSEX wc = {};

    //Windows event messages struct
    MSG msg = {};

    //Fill in the window class with needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"WindowClass1";

    //Register the window class
    RegisterClassEx(&wc);

	//Calculate the size of the client area
	RECT wr = { 0, 0, 500, 400 }; //Set the size, but not the position
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE); //Adjust the size

    //Create the window and use the result as the handle
    hWnd = CreateWindowEx(NULL,
        L"WindowClass1",    // name of the window class
        L"PawGrove",   // title of the window
        WS_OVERLAPPEDWINDOW,    // window style
        300,    // x-position of the window
        300,    // y-position of the window
        wr.right - wr.left,    // width of the window
        wr.bottom - wr.top,    // height of the window
        NULL,    // we have no parent window, NULL
        NULL,    // we aren't using menus, NULL
        hInstance,    // application handle
        NULL);    // used with multiple windows, NULL

    //Display the window on the screen
    ShowWindow(hWnd, nCmdShow);

	//Sets up and initialize Direct3D
	InitD3D(hWnd);

    //Main loop:
    //Check to see if any messages are waiting in the queue
	while (TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{

			//Translate keystroke messages into the right format
			TranslateMessage(&msg);

			//Send the message to the WindowProc function
			DispatchMessage(&msg);

			//Check to see if it's time to quit
			if (msg.message == WM_QUIT)
			{
				break;
			}
		}
		
		//Render the frame:
		RenderFrame();

	}

	//Clean up DirectX
	CleanD3D();

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


//Initialize and prepare Direct3D for use
void InitD3D(HWND hWnd)
{
	//Create a struct to hold information abuot the swap chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

	//Fill in teh swap chain description struct
	swapChainDesc.BufferCount = 1;									//One back buffer
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//Use 32-bit color
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//How swap chain is to be used
	swapChainDesc.OutputWindow = hWnd;								//The window to be used
	swapChainDesc.SampleDesc.Count = 4;								//How many multisamples
	swapChainDesc.Windowed = true;									//Windowed/full-screen mode

	//Create a device, device context, and swap chain using the information in the swapChainDesc struct
	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL, D3D11_SDK_VERSION,
		&swapChainDesc, &swapChain, &device, NULL, &deviceContext);

	//Get the address of the back buffer
	ID3D11Texture2D *pBackBuffer = {};
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	//Use the back buffer address to create the render target
	device->CreateRenderTargetView(pBackBuffer, NULL, &backBuffer);
	pBackBuffer->Release();

	//Set the render target as the back buffer
	deviceContext->OMSetRenderTargets(1, &backBuffer, NULL);

	//Set the viewport
	D3D11_VIEWPORT viewPort = {};

	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = 800;
	viewPort.Height = 600;

	deviceContext->RSSetViewports(1, &viewPort);
}


//Renders a single frame
void RenderFrame()
{
	float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };

	//Clear the back buffer to a deep blue
	deviceContext->ClearRenderTargetView(backBuffer, color);

	//Do 3D rendering on the back buffer here

	//Switch the back buffer and the front buffer to present to screen
	swapChain->Present(0, 0);
}

//Cleans up Direct3D
void CleanD3D()
{
	//Close and release all existing COM objects
	swapChain->Release();
	backBuffer->Release();
	device->Release();
	deviceContext->Release();
}