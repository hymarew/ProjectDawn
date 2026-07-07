#include "enemyManager.h"
#include "enemyPool.h"
#include "renderer.h"
#include "modelRenderer.h"
#include "main.h"

void EnemyManager::Init(int maxEnemies)
{
    // インスタンスバッファを作成する。
    // DYNAMIC + CPU_ACCESS_WRITE にすることで毎フレームCPU側から行列を書き込める
    D3D11_BUFFER_DESC bd = {};
    bd.Usage          = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth      = sizeof(InstanceData) * maxEnemies;
    bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Renderer::GetDevice()->CreateBuffer(&bd, nullptr, &m_InstanceBuffer);

    // インスタンシング専用シェーダーを読み込む。
    // 頂点シェーダーはスロット0(モデル頂点)とスロット1(インスタンス行列)の2入力を持つ
    Renderer::CreateVertexShaderInstancing(&m_VertexShader, &m_InputLayout, "shader\\InstancingVS.cso");
    Renderer::CreatePixelShader(&m_PixelShader, "shader\\ScorpionPS.cso");
}

void EnemyManager::Uninit()
{
    if (m_InstanceBuffer) { m_InstanceBuffer->Release(); m_InstanceBuffer = nullptr; }
    if (m_VertexShader)   { m_VertexShader->Release();   m_VertexShader   = nullptr; }
    if (m_PixelShader)    { m_PixelShader->Release();    m_PixelShader    = nullptr; }
    if (m_InputLayout)    { m_InputLayout->Release();    m_InputLayout    = nullptr; }
}

void EnemyManager::Draw(EnemyPool& pool)
{
    // --- 1. アクティブな敵のワールド行列をCPU側の配列にかき集める ---
    std::vector<InstanceData> instanceArray;
    instanceArray.reserve(pool.GetPool().size());

    for (auto& enemy : pool.GetPool())
    {
        if (!enemy.m_IsActive) continue;

        XMMATRIX scale = XMMatrixScaling(enemy.GetScale().x, enemy.GetScale().y, enemy.GetScale().z);
        XMMATRIX rot   = XMMatrixRotationRollPitchYaw(enemy.GetRotation().x, enemy.GetRotation().y, enemy.GetRotation().z);
        XMMATRIX trans = XMMatrixTranslation(enemy.GetPosition().x, enemy.GetPosition().y, enemy.GetPosition().z);

        InstanceData data;
        XMStoreFloat4x4(&data.worldMatrix, scale * rot * trans);
        instanceArray.push_back(data);
    }

    // 描画対象が0体なら何もしない
    if (instanceArray.empty()) return;

    // --- 2. かき集めた行列をGPUのインスタンスバッファに転送する ---
    // Map/Unmapで一時的にバッファを開き、memcpyで一括コピーする
    D3D11_MAPPED_SUBRESOURCE mapped;
    Renderer::GetDeviceContext()->Map(m_InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, instanceArray.data(), sizeof(InstanceData) * instanceArray.size());
    Renderer::GetDeviceContext()->Unmap(m_InstanceBuffer, 0);

    // --- 3. シェーダーとバッファをセットして一括描画する ---
    LIGHT light = {};
    XMVECTOR dir = XMVector3Normalize(XMVectorSet(0.0f, -1.0f, 1.0f, 0.0f));
    XMStoreFloat4(&light.Direction, dir);
    light.Enable    = TRUE;
    light.CastShadow = TRUE;
    light.Diffuse   = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    light.Ambient   = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
    Renderer::SetLight(light);

    Renderer::GetDeviceContext()->IASetInputLayout(m_InputLayout);
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader, nullptr, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader, nullptr, 0);

    // 全員同じモデルなので、先頭スロットの敵からモデルデータを借りる
    ModelRenderer* mr    = pool.GetPool()[0].GetComponent<ModelRenderer>();
    MODEL*         model = mr->GetModel();

    // スロット0: モデルの頂点バッファ、スロット1: インスタンスの行列バッファ
    UINT strides[2] = { sizeof(VERTEX_3D), sizeof(InstanceData) };
    UINT offsets[2] = { 0, 0 };
    ID3D11Buffer* vbs[2] = { model->VertexBuffer, m_InstanceBuffer };

    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 2, vbs, strides, offsets);
    Renderer::GetDeviceContext()->IASetIndexBuffer(model->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // モデルのサブセット（マテリアル単位）ごとにDrawIndexedInstancedを呼ぶ
    // 1回の呼び出しでinstanceArray.size()体分を一括描画できる
    for (unsigned int i = 0; i < model->SubsetNum; i++)
    {
        Renderer::SetMaterial(model->SubsetArray[i].Material.Material);

        if (model->SubsetArray[i].Material.Texture)
            Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &model->SubsetArray[i].Material.Texture);

        Renderer::GetDeviceContext()->DrawIndexedInstanced(
            model->SubsetArray[i].IndexNum,   // 1体あたりのインデックス数
            (UINT)instanceArray.size(),        // 描画する体数
            model->SubsetArray[i].StartIndex,
            0, 0);
    }
}

void EnemyManager::DrawShadow(EnemyPool& pool)
{
    // 影はインスタンシング未対応のため、アクティブな敵を1体ずつ描画する
    for (auto& enemy : pool.GetPool())
    {
        if (!enemy.m_IsActive) continue;
        enemy.DrawShadow();
    }
}
