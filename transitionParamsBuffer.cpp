#include "transitionParamsBuffer.h"
#include "renderer.h"

void TransitionParamsBuffer::Init()
{
    D3D11_BUFFER_DESC bd{};
    bd.Usage          = D3D11_USAGE_DEFAULT;
    bd.ByteWidth      = sizeof(TransitionParamsCB);
    bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;

    Renderer::GetDevice()->CreateBuffer(&bd, nullptr, &m_Buffer);
}

void TransitionParamsBuffer::Uninit()
{
    if (m_Buffer) { m_Buffer->Release(); m_Buffer = nullptr; }
}

void TransitionParamsBuffer::Update(const TransitionParamsCB& data) const
{
    Renderer::GetDeviceContext()->UpdateSubresource(m_Buffer, 0, nullptr, &data, 0, 0);
}

void TransitionParamsBuffer::Bind() const
{
    Renderer::GetDeviceContext()->PSSetConstantBuffers(8, 1, &m_Buffer);
}
