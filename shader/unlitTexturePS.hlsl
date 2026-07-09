
#include "common.hlsl"


Texture2D		g_Texture : register(t0);
SamplerState	g_SamplerState : register(s0);


void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{

	if (Material.TextureEnable)
	{
		outDiffuse = g_Texture.Sample(g_SamplerState, In.TexCoord);
		outDiffuse *= In.Diffuse;
	}
	else
	{
		outDiffuse = In.Diffuse;
	}
	
	    // --- ほぼ完全に透明なピクセルだけ描画を破棄する ---
    // （0.5だとパーティクルのフェードアウトが半分で急に消えてしまうため、
    //   ブレンドで自然に消えるギリギリまでしきい値を下げている）
    if (outDiffuse.a < 0.01f)
    {
        discard; // ピクセルを破棄（Zバッファにも書き込まれなくなる）
    }
    // --------------------------------------------------------------------
}
