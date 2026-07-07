#include "main.h"
#include "grass.h"
#include "manager.h"
#include "renderer.h"
#include "camera.h"
#include "DirectXTex.h"
#include "GameConfig.h"

ID3D11InputLayout*  Grass::m_VertexLayout = nullptr;
ID3D11VertexShader* Grass::m_VertexShader = nullptr;
ID3D11PixelShader*  Grass::m_PixelShader  = nullptr;
bool                Grass::m_IsLoaded     = false;

void Grass::Init()
{
    m_Layer = 2;

    VERTEX_3D vertex[4];
    vertex[0] = { XMFLOAT3(-2.0f, 4.0f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT4(1,1,1,1), XMFLOAT2(0,0) };
    vertex[1] = { XMFLOAT3( 2.0f, 4.0f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT4(1,1,1,1), XMFLOAT2(1,0) };
    vertex[2] = { XMFLOAT3(-2.0f, 0.0f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT4(1,1,1,1), XMFLOAT2(0,1) };
    vertex[3] = { XMFLOAT3( 2.0f, 0.0f, 0.0f), XMFLOAT3(0,0,-1), XMFLOAT4(1,1,1,1), XMFLOAT2(1,1) };

    D3D11_BUFFER_DESC bd{};
    bd.Usage          = D3D11_USAGE_DEFAULT;
    bd.ByteWidth      = sizeof(VERTEX_3D) * 4;
    bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA sd{};
    sd.pSysMem = vertex;
    Renderer::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);

    if (!m_IsLoaded)
    {
        Renderer::CreateVertexShader(&m_VertexShader, &m_VertexLayout,
            "shader\\TreeLeafVS.cso");
        Renderer::CreatePixelShader(&m_PixelShader,
            "shader\\TreeLeafPS.cso");
        m_IsLoaded = true;
    }

    TexMetadata  metadata;
    ScratchImage image;
    LoadFromWICFile(L"asset\\texture\\grass.png", WIC_FLAGS_NONE, &metadata, image);
    CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(),
        image.GetImageCount(), metadata, &m_Texture);
}

void Grass::Uninit()
{
    if (m_VertexBuffer) { m_VertexBuffer->Release(); m_VertexBuffer = nullptr; }
    if (m_Texture)      { m_Texture->Release();      m_Texture      = nullptr; }

    GameObject::Uninit();
}

void Grass::Update(float dt)
{
    GameObject::Update(dt);
}

void Grass::Draw()
{
    Camera* camera = Manager::GetCamera();
    if (!camera) return;

    Renderer::GetDeviceContext()->IASetInputLayout(m_VertexLayout);
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader,  nullptr, 0);

    // ビルボード：カメラのビュー行列の逆行列で向きを合わせる
    XMMATRIX invView = XMMatrixInverse(nullptr, camera->GetViewMatrix());
    invView.r[3].m128_f32[0] = 0.0f;
    invView.r[3].m128_f32[1] = 0.0f;
    invView.r[3].m128_f32[2] = 0.0f;

    XMMATRIX world = XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z)
                   * invView
                   * XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
    Renderer::SetWorldMatrix(world);

    MATERIAL material{};
    material.Diffuse       = { 1.0f, 1.0f, 1.0f, 1.0f };
    material.TextureEnable = true;
    Renderer::SetMaterial(material);

    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &m_Texture);

    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_VertexBuffer, &stride, &offset);
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    Renderer::GetDeviceContext()->Draw(4, 0);
}

void Grass::ReleaseShaders()
{
    if (m_VertexShader) { m_VertexShader->Release(); m_VertexShader = nullptr; }
    if (m_PixelShader)  { m_PixelShader ->Release(); m_PixelShader  = nullptr; }
    if (m_VertexLayout) { m_VertexLayout ->Release(); m_VertexLayout = nullptr; }
    m_IsLoaded = false;
}
