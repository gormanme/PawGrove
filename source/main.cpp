#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <stdio.h>

using namespace DirectX;

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

//Global declarations
IDXGISwapChain *swapChain = nullptr;             //Pointer to swap chain interface
ID3D11Device *device = nullptr;                  //Pointer to Direct3D device interface
ID3D11DeviceContext *deviceContext = nullptr;    //Pointer to Direct3D device context
ID3D11RenderTargetView *backBuffer = nullptr;    //Pointer to the back buffer
ID3D11InputLayout *pLayout = nullptr;            //Pointer to the input layout
ID3D11VertexShader *pVS = nullptr;               //Pointer to vertex shader
ID3D11PixelShader *pPS = nullptr;                //Pointer to pixel shader
ID3D11Buffer *pVBuffer = nullptr;                //Pointer to vertex buffer
ID3D11Buffer *pIBuffer = nullptr;                //Pointer to index buffer
ID3D11Buffer *pConstantBuffer = nullptr;         //Pointer to constant buffer
ID3D11Texture2D *pTexture = nullptr;
ID3D11ShaderResourceView *pShaderView = nullptr;
ID3D11SamplerState *pSamplerState = nullptr;
XMMATRIX worldMatrix = {};
XMMATRIX viewMatrix = {};
XMMATRIX projectionMatrix = {};
unsigned char* targaData = nullptr;

int winWidth = 800;
int winHeight = 600;

//Struct for a single vertex
struct VERTEX {
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 texture;
};

//Constant buffer struct for shader
struct ConstantBuffer
{
    XMMATRIX mWorld;
    XMMATRIX mView;
    XMMATRIX mProjection;
    XMFLOAT4 vLightDir;
    XMFLOAT4 vLightColor;
    XMFLOAT4 vOutputColor;
};

//Struct for the targa texture file
struct TargaHeader
{
    unsigned char data1[12];
    unsigned short width;
    unsigned short height;
    unsigned char bpp;
    unsigned char data2;
};

//Function declarations:
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow);
void InitD3D(HWND hWnd);            //Sets up and initializes Direct3D
void RenderFrame();                 //Renders a single frame
void CleanD3D();                    //Closes Direct3D and releases memory
void InitGraphics();                //Creates the shape to render
void InitPipeline();                //Loads and prepares the shaders
bool LoadTarga(char* filename, int& height, int& width);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
    //Handle for the window
    HWND hWnd = nullptr;

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
        L"WindowClass1",           // name of the window class
        L"PawGrove",               // title of the window
        WS_OVERLAPPEDWINDOW,       // window style
        300,                       // x-position of the window
        300,                       // y-position of the window
        wr.right - wr.left,        // width of the window
        wr.bottom - wr.top,        // height of the window
        NULL,                      // we have no parent window, NULL
        NULL,                      // we aren't using menus, NULL
        hInstance,                 // application handle
        NULL);                     // used with multiple windows, NULL

    //Display the window on the screen
    ShowWindow(hWnd, nCmdShow);

    //Sets up and initialize Direct3D
    InitD3D(hWnd);

    //Main loop:
    //Check to see if any messages are waiting in the queue
    while (true)
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

        //When escape key is pressed, quit the application
        case WM_KEYUP:
        {
            if (wParam == VK_ESCAPE)
            {
                PostQuitMessage(0);
                return 0;
            }
        }
        break;
    }

    //Handle any messages the switch statement didn't
    return DefWindowProc(hWnd, message, wParam, lParam);
}


//Initialize and prepare Direct3D for use
void InitD3D(HWND hWnd)
{
    HRESULT hr = S_OK;
    //Create a struct to hold information abuot the swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};

    //Fill in teh swap chain description struct
    swapChainDesc.BufferCount = 1;                                  //One back buffer
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;   //Use 32-bit color
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;    //How swap chain is to be used
    swapChainDesc.OutputWindow = hWnd;                              //The window to be used
    swapChainDesc.SampleDesc.Count = 4;                             //How many multisamples
    swapChainDesc.Windowed = true;                                  //Windowed/full-screen mode

    //Create a device, device context, and swap chain using the information in the swapChainDesc struct
    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, NULL, NULL, D3D11_SDK_VERSION,
        &swapChainDesc, &swapChain, &device, NULL, &deviceContext);

    //Get the address of the back buffer
    ID3D11Texture2D *pBackBuffer = {};
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    //Use the back buffer address to create the render target
    hr = device->CreateRenderTargetView(pBackBuffer, NULL, &backBuffer);
    assert(SUCCEEDED(hr));

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

    //Update our time for animating the cube
    static float t = 0.0f;
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0)
    {
        timeStart = timeCur;
    }
    t = (timeCur - timeStart) / 1000.0f;

    //Animate the cube
    worldMatrix = XMMatrixRotationY(t);

    float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };

    //Set up lighting parameters
    XMFLOAT4 LightDir = { 0.0f, 0.0f, 1.0f, 1.0f };
    XMFLOAT4 LightColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    //Clear the back buffer to a deep blue
    deviceContext->ClearRenderTargetView(backBuffer, color);

    //Select which vertex buffer to display
    UINT stride = sizeof(VERTEX);
    UINT offset = 0;
    deviceContext->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);

    //Set index buffer
    deviceContext->IASetIndexBuffer(pIBuffer, DXGI_FORMAT_R16_UINT, 0);

    //Select which primitive type we are using
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //Update variables for shader
    ConstantBuffer cb = {};
    cb.mWorld = XMMatrixTranspose(worldMatrix);
    cb.mView = XMMatrixTranspose(viewMatrix);
    cb.mProjection = XMMatrixTranspose(projectionMatrix);
    cb.vLightDir = LightDir;
    cb.vLightColor = LightColor;
    deviceContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);

    //Render the light
    XMMATRIX light = XMMatrixTranslationFromVector(5.0f * XMLoadFloat4(&LightDir));
    XMMATRIX lightScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
    light = lightScale * light;

    //Pass the shader resource view to the shader
    deviceContext->PSSetShaderResources(0, 1, &pShaderView);

    deviceContext->PSSetSamplers(0, 1, &pSamplerState);


    //Update world variable to reflect current light
    //cb.mWorld = XMMatrixTranspose(light);
    //deviceContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);

    //Draw the vertex buffer to the back buffer
    deviceContext->DrawIndexed(36, 0, 0);

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
    pIBuffer->Release();
    pConstantBuffer->Release();
    pTexture->Release();
    pShaderView->Release();
    pSamplerState->Release();
    swapChain->Release();
    backBuffer->Release();
    device->Release();
    deviceContext->Release();

    delete[] targaData;
    targaData = 0;
}

//Creates the shape to render
void InitGraphics()
{
    HRESULT hr = S_OK;
    //Create a triangle using the VERTEX struct
    //VERTEX vertices[] =
    //{
    //    { 0.0f, 0.5f, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f} },
    //    { 0.45f, -0.5, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f} },
    //    { -0.45f, -0.5f, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f} }
    //};

    VERTEX vertices[] =
    {
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },

        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },

        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },

        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },

        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },

        { XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
    };

    //Create the vertex buffer
    D3D11_BUFFER_DESC vBufferDesc = {};
    vBufferDesc.Usage = D3D11_USAGE_DYNAMIC;                 //Write access by CPU and GPU
    vBufferDesc.ByteWidth = sizeof(vertices);                //Size is the VERTEX struct * number of vertices
    vBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;        //Use as a vertex buffer
    vBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;     //Allow CPU to write in buffer

    hr = device->CreateBuffer(&vBufferDesc, NULL, &pVBuffer);
    assert(SUCCEEDED(hr));

    //Copy the vertices into the buffer
    D3D11_MAPPED_SUBRESOURCE vMappedResource = {};
    deviceContext->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &vMappedResource);    //map the buffer
    memcpy(vMappedResource.pData, vertices, sizeof(vertices));                        //copy the data
    deviceContext->Unmap(pVBuffer, NULL);

    short indices[] =
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };
    D3D11_BUFFER_DESC iBufferDesc = {};
    iBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    iBufferDesc.ByteWidth = sizeof(indices);
    iBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    iBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    //Create index buffer
    hr = device->CreateBuffer(&iBufferDesc, NULL, &pIBuffer);
    assert(SUCCEEDED(hr));

    //Copy the indices into the buffer
    D3D11_MAPPED_SUBRESOURCE iMappedResource = {};
    deviceContext->Map(pIBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &iMappedResource);    //Map the buffer
    memcpy(iMappedResource.pData, indices, sizeof(indices));                                //Copy the data
    deviceContext->Unmap(pIBuffer, NULL);

    int width = 0;
    int height = 0;
    bool result = LoadTarga("assets/stone.tga", height, width);
    assert(result == true);

    CD3D11_TEXTURE2D_DESC textureDesc(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1);

    D3D11_SUBRESOURCE_DATA initialData = {};
    initialData.SysMemPitch = 4 * width;
    initialData.pSysMem = targaData;

    hr = device->CreateTexture2D(&textureDesc, &initialData, &pTexture);
    assert(SUCCEEDED(hr));

    hr = device->CreateShaderResourceView(pTexture, NULL, &pShaderView);
    assert(SUCCEEDED(hr));

    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = device->CreateSamplerState(&samplerDesc, &pSamplerState);
    assert(SUCCEEDED(hr));
}


//Loads and prepares the shaders
void InitPipeline()
{
    HRESULT hr = S_OK;

    //Load and compile the two shaders
    ID3D10Blob *VS = nullptr;
    ID3D10Blob *PS = nullptr;
    ID3DBlob *pErrorBlob = nullptr;
    hr = D3DCompileFromFile(L"source/shader.hlsl", nullptr, nullptr, "VShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &VS, &pErrorBlob);
    assert(SUCCEEDED(hr));

    if (pErrorBlob)
    {
        OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
        pErrorBlob->Release();
        pErrorBlob->Release();
    }

    hr = D3DCompileFromFile(L"source/shader.hlsl", nullptr, nullptr, "PShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &PS, &pErrorBlob);
    assert(SUCCEEDED(hr));

    if (pErrorBlob)
    {
        OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
        pErrorBlob->Release();
        pErrorBlob->Release();
    }

    //Encapsulate both shaders into the shader objects
    hr = device->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
    assert(SUCCEEDED(hr));

    hr = device->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);
    assert(SUCCEEDED(hr));


    //Initialize the world matrix
    worldMatrix = XMMatrixIdentity();

    //Create the constant buffer
    D3D11_BUFFER_DESC cBufferDesc = {};
    cBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    cBufferDesc.ByteWidth = sizeof(ConstantBuffer);
    cBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cBufferDesc.CPUAccessFlags = 0;
    hr = device->CreateBuffer(&cBufferDesc, nullptr, &pConstantBuffer);
    assert(SUCCEEDED(hr));

    //Initialize the view matrix
    XMVECTOR eye = XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f);
    XMVECTOR at = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    viewMatrix = XMMatrixLookAtLH(eye, at, up);

    //Initialize the projection matrix
    projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV2, winWidth / (float)winHeight, 0.01f, 100.0f);

    //Set the shader objects
    deviceContext->VSSetShader(pVS, 0, 0);
    deviceContext->VSSetConstantBuffers(0, 1, &pConstantBuffer);
    deviceContext->PSSetConstantBuffers(0, 1, &pConstantBuffer);
    deviceContext->PSSetShader(pPS, 0, 0);
    deviceContext->PSSetShaderResources(0, 1, &pShaderView);

    //Create the input layout object
    D3D11_INPUT_ELEMENT_DESC elementDesc[] = 
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = device->CreateInputLayout(elementDesc, _countof(elementDesc), VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
    assert(SUCCEEDED(hr));

    deviceContext->IASetInputLayout(pLayout);
}


bool LoadTarga(char* filename, int& height, int& width)
{
    FILE* filePtr = nullptr;
    unsigned int count = 0;
    TargaHeader targaFileHeader = {};
    unsigned char* targaImage = nullptr;


    // Open the targa file for reading in binary.
    int error = fopen_s(&filePtr, filename, "rb");
    if (error != 0)
    {
        return false;
    }

    // Read in the file header.
    count = (unsigned int)fread(&targaFileHeader, sizeof(TargaHeader), 1, filePtr);
    if (count != 1)
    {
        return false;
    }

    // Get the important information from the header.
    height = (int)targaFileHeader.height;
    width = (int)targaFileHeader.width;
    int bpp = (int)targaFileHeader.bpp;

    // Check that it is 32 bit and not 24 bit.
    if (bpp != 32)
    {
        return false;
    }

    // Calculate the size of the 32 bit image data.
    int imageSize = width * height * 4;

    // Allocate memory for the targa image data.
    targaImage = new unsigned char[imageSize];
    if (!targaImage)
    {
        return false;
    }

    // Read in the targa image data.
    count = (unsigned int)fread(targaImage, 1, imageSize, filePtr);
    if (count != imageSize)
    {
        return false;
    }

    // Close the file.
    error = fclose(filePtr);
    if (error != 0)
    {
        return false;
    }

    // Allocate memory for the targa destination data.
    targaData = new unsigned char[imageSize];

    // Initialize the index into the targa destination data array.
    int index = 0;

    // Initialize the index into the targa image data.
    int k = (width * height * 4) - (width * 4);

    // Now copy the targa image data into the targa destination array in the correct order since the targa format is stored upside down.
    for (int j = 0; j<height; j++)
    {
        for (int i = 0; i<width; i++)
        {
            targaData[index + 0] = targaImage[k + 2];  // Red.
            targaData[index + 1] = targaImage[k + 1];  // Green.
            targaData[index + 2] = targaImage[k + 0];  // Blue
            targaData[index + 3] = targaImage[k + 3];  // Alpha

                                                         // Increment the indexes into the targa data.
            k += 4;
            index += 4;
        }

        // Set the targa image data index back to the preceding row at the beginning of the column since its reading it in upside down.
        k -= (width * 8);
    }

    // Release the targa image data now that it was copied into the destination array.
    delete[] targaImage;
    targaImage = 0;

    return true;
}