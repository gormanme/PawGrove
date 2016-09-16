#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

//Global declarations
IDXGISwapChain *swapChain = {};				//Pointer to swap chain interface
ID3D11Device *device = {};					//Pointer to Direct3D device interface
ID3D11DeviceContext *deviceContext = {};	//Pointer to Direct3D device context
ID3D11RenderTargetView *backBuffer = {};	//Pointer to the back buffer
ID3D11InputLayout *pLayout = {};			//Pointer to the input layout
ID3D11VertexShader *pVS = {};				//Pointer to vertex shader
ID3D11PixelShader *pPS = {};				//Pointer to pixel shader
ID3D11Buffer *pVBuffer = {};				//Pointer to vertex buffer

int winWidth = 800;
int winHeight = 600;

//Struct for a single vertex
struct VERTEX {
    FLOAT X, Y, Z;
    FLOAT color[4];
};



//Function declarations:
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow);
void InitD3D(HWND hWnd);			//Sets up and initializes Direct3D
void RenderFrame();					//Renders a single frame
void CleanD3D();					//Closes Direct3D and releases memory
void InitGraphics();				//Creates the shape to render
void InitPipeline();				//Loads and prepares the shaders


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
    RECT wr = { 0, 0, winWidth, winHeight }; //Set the size, but not the position
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
    viewPort.Width = (float)winWidth;
    viewPort.Height = (float)winHeight;

    deviceContext->RSSetViewports(1, &viewPort);

    InitPipeline();
    InitGraphics();
}


//Renders a single frame
void RenderFrame()
{
    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };

    //Clear the back buffer to a deep blue
    deviceContext->ClearRenderTargetView(backBuffer, color);

    //Select which vertex buffer to display
    UINT stride = sizeof(VERTEX);
    UINT offset = 0;
    deviceContext->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);

    //Select which primitive type we are using
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //Draw the vertex buffer to the back buffer
    deviceContext->Draw(6, 0);

    //Switch the back buffer and the front buffer to present to screen
    swapChain->Present(0, 0);
}

//Cleans up Direct3D
void CleanD3D()
{
    //Close and release all existing COM objects
    pLayout->Release();
    pVS->Release();
    pPS->Release();
    pVBuffer->Release();
    swapChain->Release();
    backBuffer->Release();
    device->Release();
    deviceContext->Release();
}

//Creates the shape to render
void InitGraphics()
{
    //Create a triangle using the VERTEX struct
    //VERTEX OurVertices[] =
    //{
    //    { 0.0f, 0.5f, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f} },
    //    { 0.45f, -0.5, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f} },
    //    { -0.45f, -0.5f, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f} }
    //};

	VERTEX OurVertices[] =
	{
		{ 0.45f, 0.5, 0.0f,{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ 0.45f, -0.5f, 0.0f,{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ -0.45f, 0.5f, 0.0f,{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ -0.45f, 0.5f, 0.0f,{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ 0.45f, -0.5f, 0.0f,{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ -0.45f, -0.5f, 0.0f,{ 0.0f, 1.0f, 0.0f, 1.0f } },
	};

    //Create the vertex buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;				//Write access by CPU and GPU
    bufferDesc.ByteWidth = sizeof(VERTEX) * 6;			//Size is the VERTEX struct * 3
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;	//Use as a vertex buffer
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;	//Allow CPU to write in buffer

    device->CreateBuffer(&bufferDesc, NULL, &pVBuffer);

    //Copy the vertices into the buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    deviceContext->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &mappedResource);	//map the buffer
    memcpy(mappedResource.pData, OurVertices, sizeof(OurVertices));						//copy the data
    deviceContext->Unmap(pVBuffer, NULL);
}


//Loads and prepares the shaders
void InitPipeline()
{
    //Load and compile the two shaders
    ID3D10Blob *VS, *PS = {};
    D3DCompileFromFile(L"source/shader.hlsl", NULL, NULL, "VShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &VS, 0);
    D3DCompileFromFile(L"source/shader.hlsl", NULL, NULL, "PShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &PS, 0);

    //Encapsulate both shaders into the shader objects
    device->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
    device->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);

    //Set the shader objects
    deviceContext->VSSetShader(pVS, 0, 0);
    deviceContext->PSSetShader(pPS, 0, 0);

    //Create the input layout object
    D3D11_INPUT_ELEMENT_DESC elementDesc[] = 
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    device->CreateInputLayout(elementDesc, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
    deviceContext->IASetInputLayout(pLayout);
}