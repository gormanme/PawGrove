#include <windows.h>
#include <d3d11.h>
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <stdio.h>
#include <vector>

#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>

#include "Camera.h"

using namespace DirectX;

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "d3dcompiler.lib")

struct Object {
    ID3D11Buffer *pVBuffer = nullptr;                //Pointer to vertex buffer
    ID3D11Buffer *pIBuffer = nullptr;                //Pointer to index buffer
    ID3D11Texture2D *pTexture = nullptr;
    ID3D11ShaderResourceView *pShaderView = nullptr;
    int vertex_count = 0;
    int index_count = 0;
    int vertex_size = 0;
    int index_size = 0;
};

//Global declarations
IDXGISwapChain *swapChain = nullptr;             //Pointer to swap chain interface
ID3D11Device *device = nullptr;                  //Pointer to Direct3D device interface
ID3D11DeviceContext *deviceContext = nullptr;    //Pointer to Direct3D device context
ID3D11RenderTargetView *backBuffer = nullptr;    //Pointer to the back buffer
ID3D11InputLayout *pLayout = nullptr;            //Pointer to the input layout
ID3D11VertexShader *pVS = nullptr;               //Pointer to vertex shader
ID3D11PixelShader *pPS = nullptr;                //Pointer to pixel shader
ID3D11Buffer *pConstantBuffer = nullptr;         //Pointer to constant buffer
ID3D11SamplerState *pSamplerState = nullptr;
XMMATRIX worldMatrix = {};
XMMATRIX viewMatrix = {};
XMMATRIX projectionMatrix = {};
unsigned char* targaData = nullptr;
Camera camera;
Object cube;
Object ground;
Object panda;
ID3D11Texture2D *depthStencilBuffer = nullptr;
ID3D11DepthStencilView *depthStencilView = nullptr;
ID3D11DepthStencilState *depthStencilState = nullptr;

IDXGIDebug* debug = nullptr;

const int winWidth = 800;
const int winHeight = 600;

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
void CreateDepthBuffer();           //Creates depth buffer
void InitD3D(HWND hWnd);            //Sets up and initializes Direct3D
void RenderFrame();                 //Renders a single frame
void CleanD3D();                    //Closes Direct3D and releases memory
void InitGraphics();                //Creates the shape to render
void InitPipeline();                //Loads and prepares the shaders
bool LoadTarga(const char* filename, int& height, int& width);
Object SetupObject(VERTEX* vertices, UINT vertices_size, short *indices, UINT indices_size, const char *targa);
Object LoadModel(const char* filename);


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
    //Handle for the window
    HWND hWnd = nullptr;

    //Holds information for the window class
    WNDCLASSEX wc = {};

    //Windows event messages struct
    MSG msg = {};

    //GetTickCount() returns milliseconds, when we want seconds
    LARGE_INTEGER frequency = {};
    LARGE_INTEGER prevTime = {};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&prevTime);

    //Fill in the window class with needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"WindowClass1";

    //Register the window class
    RegisterClassEx(&wc);

    //Calculate the size of the client area
    RECT wr = { 0, 0, winWidth, winHeight }; //Set the size, but not the position
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE); //Adjust the size

    //Create the window and use the result as the handle
    hWnd = CreateWindowEx(0,
        L"WindowClass1",           // name of the window class
        L"PawGrove",               // title of the window
        WS_OVERLAPPEDWINDOW,       // window style
        300,                       // x-position of the window
        300,                       // y-position of the window
        wr.right - wr.left,        // width of the window
        wr.bottom - wr.top,        // height of the window
        nullptr,                      // we have no parent window, nullptr
        nullptr,                      // we aren't using menus, nullptr
        hInstance,                 // application handle
        nullptr);                     // used with multiple windows, nullptr

    //Display the window on the screen
    ShowWindow(hWnd, nShowCmd);

    //Sets up and initialize Direct3D
    InitD3D(hWnd);


    //Initialize game stuff (camera)
    camera.SetLens(XM_PI/3, winWidth / static_cast<float>(winHeight), 0.01f, 100.0f);
    XMVECTOR eye = XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f);
    XMVECTOR at = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    camera.LookAt(eye, at, up);

    //Main loop:
    while (true)
    {
        //Check to see if any messages are waiting in the queue
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
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
        
        //Already have previous time
        //Get current time
        //Subtract current time from previous time
        LARGE_INTEGER currTime = {};
        QueryPerformanceCounter(&currTime);
        float deltaTime = (currTime.QuadPart - prevTime.QuadPart) / static_cast<float>(frequency.QuadPart);
        prevTime = currTime;

        //Update the frame with camera controls
        // Camera controls
        if (GetAsyncKeyState('W') & 0x8000)
            camera.Walk(10.0f*deltaTime);

        if (GetAsyncKeyState('S') & 0x8000)
            camera.Walk(-10.0f*deltaTime);

        if (GetAsyncKeyState('A') & 0x8000)
            camera.Strafe(-10.0f*deltaTime);

        if (GetAsyncKeyState('D') & 0x8000)
            camera.Strafe(10.0f*deltaTime);

        camera.UpdateViewMatrix();

        //Render the frame:
        RenderFrame();

    }

    //Clean up DirectX
    CleanD3D();

    //Return this part of the WM_QUIT message to Windows
    return static_cast<int>(msg.wParam);
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
    swapChainDesc.BufferCount = 3;                                  //One back buffer
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;   //Use 32-bit color
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;    //How swap chain is to be used
    swapChainDesc.OutputWindow = hWnd;                              //The window to be used
    swapChainDesc.SampleDesc.Count = 1;                             //How many multisamples
    swapChainDesc.Windowed = true;                                  //Windowed/full-screen mode

    //Create a device, device context, and swap chain using the information in the swapChainDesc struct
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION,
        &swapChainDesc, &swapChain, &device, nullptr, &deviceContext);

    //Get the address of the back buffer
    auto lib = LoadLibraryW(L"dxgidebug.dll");
    if (lib) {
        auto pDXGIGetDebugInterface = reinterpret_cast<decltype(&DXGIGetDebugInterface)>(GetProcAddress(lib, "DXGIGetDebugInterface"));
        pDXGIGetDebugInterface(IID_PPV_ARGS(&debug));
    }
    
    ID3D11Texture2D *pBackBuffer = {};
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    //Use the back buffer address to create the render target
    hr = device->CreateRenderTargetView(pBackBuffer, nullptr, &backBuffer);
    assert(SUCCEEDED(hr));

    pBackBuffer->Release();

    //Create the depth buffer
    CreateDepthBuffer();

    //Set the render target as the back buffer
    deviceContext->OMSetRenderTargets(1, &backBuffer, depthStencilView);

    //Set the viewport
    D3D11_VIEWPORT viewPort = {};

    viewPort.TopLeftX = 0;
    viewPort.TopLeftY = 0;
    viewPort.Width = (float)winWidth;
    viewPort.Height = (float)winHeight;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;

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
    XMFLOAT4 LightDir = { 0.0f, -3.0f, 1.0f, 1.0f };
    XMStoreFloat4(&LightDir, XMVector4Normalize(XMLoadFloat4(&LightDir)));
    XMFLOAT4 LightColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    //Clear the back buffer to a deep blue
    deviceContext->ClearRenderTargetView(backBuffer, color);
    deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    //deviceContext->OMSetDepthStencilState(depthStencilState, 0);

    //Select which vertex buffer to display
    UINT stride = sizeof(VERTEX);
    UINT offset = 0;

    //Select which primitive type we are using
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //Update variables for shader
    ConstantBuffer cb = {};
    cb.mWorld = XMMatrixTranspose(worldMatrix);
    cb.mView = XMMatrixTranspose(camera.View());
    cb.mProjection = XMMatrixTranspose(camera.Proj());
    cb.vLightDir = LightDir;
    cb.vLightColor = LightColor;
    deviceContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);

    //Render the light
    XMMATRIX light = XMMatrixTranslationFromVector(5.0f * XMLoadFloat4(&LightDir));
    const XMMATRIX lightScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
    light = lightScale * light;

    deviceContext->PSSetSamplers(0, 1, &pSamplerState);


    //Update world variable to reflect current light
    //cb.mWorld = XMMatrixTranspose(light);
    //deviceContext->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);


    //Draw the vertex buffer to the back buffer
    //Ground:
    //deviceContext->IASetVertexBuffers(0, 1, &ground.pVBuffer, &stride, &offset);
    //deviceContext->IASetIndexBuffer(ground.pIBuffer, DXGI_FORMAT_R16_UINT, 0);
    //deviceContext->PSSetShaderResources(0, 1, &ground.pShaderView);
    //deviceContext->DrawIndexed(ground.index_count, 0, 0);

    //Panda:
    deviceContext->IASetVertexBuffers(0, 1, &panda.pVBuffer, &stride, &offset);
    deviceContext->IASetIndexBuffer(panda.pIBuffer, DXGI_FORMAT_R16_UINT, 0);
    deviceContext->PSSetShaderResources(0, 1, &panda.pShaderView);
    deviceContext->DrawIndexed(panda.index_count, 0, 0);

    //Cube:
    /*deviceContext->IASetVertexBuffers(0, 1, &cube.pVBuffer, &stride, &offset);
    deviceContext->IASetIndexBuffer(cube.pIBuffer, DXGI_FORMAT_R16_UINT, 0);
    deviceContext->PSSetShaderResources(0, 1, &cube.pShaderView);
    deviceContext->DrawIndexed(cube.index_count, 0, 0);*/



    //Switch the back buffer and the front buffer to present to screen
    swapChain->Present(1, 0);
}


//Cleans up Direct3D
void CleanD3D()
{
    //Close and release all existing COM objects
    pLayout->Release();
    pVS->Release();
    pPS->Release();
    cube.pVBuffer->Release();
    ground.pVBuffer->Release();
    cube.pIBuffer->Release();
    ground.pIBuffer->Release();
    pConstantBuffer->Release();
    cube.pTexture->Release();
    ground.pTexture->Release();
    cube.pShaderView->Release();
    ground.pShaderView->Release();
    depthStencilBuffer->Release();
    depthStencilView->Release();
    depthStencilState->Release();
    pSamplerState->Release();
    swapChain->Release();
    backBuffer->Release();
    device->Release();
    deviceContext->Release();

    debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    debug->Release();

    delete[] targaData;
    targaData = 0;
}

Object SetupObject(VERTEX* vertices, UINT vertices_size, short *indices, UINT indices_size, const char *targa) 
{
    HRESULT hr = S_OK;
    Object object;

    //Create the vertex buffer
    D3D11_BUFFER_DESC vBufferDesc = {};
    vBufferDesc.Usage = D3D11_USAGE_DYNAMIC;                 //Write access by CPU and GPU
    vBufferDesc.ByteWidth = vertices_size;                //Size is the VERTEX struct * number of vertices
    vBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;        //Use as a vertex buffer
    vBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;     //Allow CPU to write in buffer

    hr = device->CreateBuffer(&vBufferDesc, nullptr, &object.pVBuffer);
    assert(SUCCEEDED(hr));

    //Copy the vertices into the buffer
    D3D11_MAPPED_SUBRESOURCE vMappedResource = {};
    deviceContext->Map(object.pVBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vMappedResource);    //map the buffer
    memcpy(vMappedResource.pData, vertices, vertices_size);                        //copy the data
    deviceContext->Unmap(object.pVBuffer, 0);

    D3D11_BUFFER_DESC iBufferDesc = {};
    iBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    iBufferDesc.ByteWidth = indices_size;
    iBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    iBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    //Create index buffer
    hr = device->CreateBuffer(&iBufferDesc, nullptr, &object.pIBuffer);
    assert(SUCCEEDED(hr));

    //Copy the indices into the buffer
    D3D11_MAPPED_SUBRESOURCE iMappedResource = {};
    deviceContext->Map(object.pIBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &iMappedResource);    //Map the buffer
    memcpy(iMappedResource.pData, indices, indices_size);                                //Copy the data
    deviceContext->Unmap(object.pIBuffer, 0);

    int width = 0;
    int height = 0;
    //bool result = LoadTarga("assets/stone.tga", height, width);
    const bool result = LoadTarga(targa, height, width);
    assert(result == true);

    CD3D11_TEXTURE2D_DESC textureDesc(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1);

    D3D11_SUBRESOURCE_DATA initialData = {};
    initialData.SysMemPitch = 4 * width;
    initialData.pSysMem = targaData;

    hr = device->CreateTexture2D(&textureDesc, &initialData, &object.pTexture);
    assert(SUCCEEDED(hr));

    hr = device->CreateShaderResourceView(object.pTexture, nullptr, &object.pShaderView);
    assert(SUCCEEDED(hr));

    object.vertex_count = vertices_size / sizeof(vertices[0]);
    object.vertex_size = sizeof(vertices[0]);
    object.index_count = indices_size / sizeof(indices[0]);
    object.index_size = sizeof(indices[0]);

    return object;
}

Object LoadModel(const char * filename)
{
    
    // Create importer
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, true);
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE,
        aiPrimitiveType_LINE |
        aiPrimitiveType_POINT);
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
        aiComponent_COLORS |
        aiComponent_LIGHTS |
        aiComponent_CAMERAS |
        aiComponent_BONEWEIGHTS);
    // Load scene
    auto const* scene = importer.ReadFile(filename,
        aiProcess_ConvertToLeftHanded |
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_TransformUVCoords |
        aiProcess_GenUVCoords |
        aiProcess_ValidateDataStructure |
        aiProcess_GenSmoothNormals |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_OptimizeMeshes |
        aiProcess_FindDegenerates |
        aiProcess_FindInvalidData |
        aiProcess_FindInstances |
        aiProcess_SortByPType);
    
    assert(scene != NULL);
    assert(scene->mNumMeshes > 0);

    //Find number of vertices and indices
    uint32_t vertex_count = 0;
    uint32_t index_count = 0;

    for (uint32_t m = 0; m < scene->mNumMeshes; m++) {
        auto const* const mesh = scene->mMeshes[m];
        vertex_count += mesh->mNumVertices;
        index_count += mesh->mNumFaces * 3;
    }

    
    size_t const vertex_size = sizeof(VERTEX);
    size_t const index_size = sizeof(uint32_t);
    // For each model
    //  for each mesh
    //      get verices
    //      get indices

    std::vector<VERTEX> vertices = {};
    std::vector<short> indices = {};

    for (uint32_t m = 0; m < scene->mNumMeshes; m++) {
        auto const* curr_mesh = scene->mMeshes[m];

        size_t vertex_start_offset = vertices.size();

        for (uint32_t i = 0; i < curr_mesh->mNumVertices; i++) {
            aiVector3D const& pos = curr_mesh->mVertices[i];
            aiVector3D const& tex = curr_mesh->HasTextureCoords(0) ? curr_mesh->mTextureCoords[0][i] : aiVector3D(0.0f);
            aiVector3D const& normal = curr_mesh->mNormals[i];

            VERTEX vertex = {
                { pos.x, pos.y, pos.z },
                { normal.x, normal.y, normal.z },
                { tex.x, tex.y }
            };

            vertices.push_back(vertex);
        }

        for (uint32_t i = 0; i < curr_mesh->mNumFaces; i++) {
            aiFace const& face = curr_mesh->mFaces[i];
            //for (int j = 2; j >=0; --j) {
            for (int j = 0; j < 3; j++) {
                indices.push_back((uint16_t)face.mIndices[j] + (uint16_t)vertex_start_offset);
            }
        }
    }



    return SetupObject(vertices.data(), static_cast<UINT>(vertices.size() * sizeof(vertices[0])), indices.data(), static_cast<UINT>(indices.size() * sizeof(indices[0])), "assets/pandaren_model/pandaren_Body.tga");
}


void CreateDepthBuffer()
{
    HRESULT hr = S_OK;

    D3D11_TEXTURE2D_DESC depthStencilDesc = {};

    depthStencilDesc.Width = winWidth;
    depthStencilDesc.Height = winHeight;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    //No MSAA
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;

    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;

    hr = device->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilBuffer);
    assert(SUCCEEDED(hr));

    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
    depthStencilViewDesc.Format = depthStencilDesc.Format;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    hr = device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &depthStencilView);
    assert(SUCCEEDED(hr));

}



//Creates the shape to render
void InitGraphics()
{
    //Create a triangle using the VERTEX struct
    //VERTEX vertices[] =
    //{
    //    { 0.0f, 0.5f, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f} },
    //    { 0.45f, -0.5, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f} },
    //    { -0.45f, -0.5f, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f} }
    //};

    VERTEX cubeVertices[] =
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

    VERTEX groundVertices[]=
    {
        { XMFLOAT3(-5.0f, -1.0f, -5.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(5.0f, -1.0f, -5.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(-5.0f, -1.0f, 5.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(5.0f, -1.0f, 5.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
    };

    short cubeIndices[] =
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

    short groundIndices[] =
    {
        1,0,2,
        1,2,3
    };

    cube = SetupObject(cubeVertices, sizeof(cubeVertices), cubeIndices, sizeof(cubeIndices), "assets/stone.tga");
    ground = SetupObject(groundVertices, sizeof(groundVertices), groundIndices, sizeof(groundIndices), "assets/stone.tga");
    panda = LoadModel("assets/pandaren_model/pandaren.obj");
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


    if (pErrorBlob)
    {
        OutputDebugStringA(static_cast<const char*>(pErrorBlob->GetBufferPointer()));
        pErrorBlob->Release();
    }
    assert(SUCCEEDED(hr));

    hr = D3DCompileFromFile(L"source/shader.hlsl", nullptr, nullptr, "PShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &PS, &pErrorBlob);

    if (pErrorBlob)
    {
        OutputDebugStringA(static_cast<const char*>(pErrorBlob->GetBufferPointer()));
        pErrorBlob->Release();
    }
    assert(SUCCEEDED(hr));

    //Encapsulate both shaders into the shader objects
    hr = device->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), nullptr, &pVS);
    assert(SUCCEEDED(hr));

    hr = device->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), nullptr, &pPS);
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
    projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV2, winWidth / static_cast<float>(winHeight), 0.01f, 100.0f);


    //Set the shader objects
    deviceContext->VSSetShader(pVS, 0, 0);
    deviceContext->VSSetConstantBuffers(0, 1, &pConstantBuffer);
    deviceContext->PSSetConstantBuffers(0, 1, &pConstantBuffer);
    deviceContext->PSSetShader(pPS, 0, 0);
    deviceContext->PSSetShaderResources(0, 1, &cube.pShaderView);
    deviceContext->PSSetShaderResources(1, 1, &cube.pShaderView);

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

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = FALSE;

    hr = device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState);
    assert(SUCCEEDED(hr));

}


bool LoadTarga(const char* filename, int& height, int& width)
{
    FILE* filePtr = nullptr;
    size_t count = 0;
    TargaHeader targaFileHeader = {};
    unsigned char* targaImage = nullptr;


    // Open the targa file for reading in binary.
    int error = fopen_s(&filePtr, filename, "rb");
    if (error != 0 || filePtr == nullptr)
    {
        return false;
    }

    // Read in the file header.
    count = fread(&targaFileHeader, sizeof(TargaHeader), 1, filePtr);
    if (count != 1)
    {
        return false;
    }

    // Get the important information from the header.
    height = (int)targaFileHeader.height;
    width = (int)targaFileHeader.width;
    const int bpp = (int)targaFileHeader.bpp;

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
    count = fread(targaImage, 1, imageSize, filePtr);
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