// common.hlsl を使わず cbuffer を直接定義する（include による構造体定義の混入を避ける）
cbuffer WorldBuffer      : register(b0) { matrix World; }
cbuffer ViewBuffer       : register(b1) { matrix View; }
cbuffer ProjectionBuffer : register(b2) { matrix Projection; }

void main(in float3 inPos : POSITION, out float4 outPos : SV_Position)
{
    float4 worldPos = mul(float4(inPos, 1.0f), World);
    float4 viewPos  = mul(worldPos, View);
    outPos          = mul(viewPos, Projection);
}
