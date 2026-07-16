#pragma once
#include <d3d11.h>

using namespace DirectX;


struct VERTEX_3D
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT4 Diffuse;
	XMFLOAT2 TexCoord;
};



struct MATERIAL
{
	XMFLOAT4	Ambient;
	XMFLOAT4	Diffuse;
	XMFLOAT4	Specular;
	XMFLOAT4	Emission;
	float		Shininess;
	BOOL		TextureEnable;
	float		Dummy[2];
};



struct LIGHT	//Common.hlslと同じ内容
{
	BOOL        Enable;
	BOOL        CastShadow; // 追加: 影を落とすかどうかのフラグ
	BOOL        Dummy[2];   // Dummy[3] から減らす
	XMFLOAT4	Direction;
	XMFLOAT4	Diffuse;
	XMFLOAT4	Ambient;

	XMFLOAT4	Position;
	XMFLOAT4	PointLightParam;	//照らす範囲
	XMFLOAT4	SpotLightParam;		//照らす角度
};



class Renderer
{
private:

	static D3D_FEATURE_LEVEL       m_FeatureLevel;

	static ID3D11Device*           m_Device;
	static ID3D11DeviceContext*    m_DeviceContext;
	static IDXGISwapChain*         m_SwapChain;
	static ID3D11RenderTargetView* m_RenderTargetView;
	static ID3D11DepthStencilView* m_DepthStencilView;

	static ID3D11Buffer*			m_WorldBuffer;
	static ID3D11Buffer*			m_ViewBuffer;
	static ID3D11Buffer*			m_ProjectionBuffer;
	static ID3D11Buffer*			m_MaterialBuffer;
	static ID3D11Buffer*			m_LightBuffer;
	static ID3D11Buffer*			m_CameraBuffer;


	static ID3D11DepthStencilState* m_DepthStateEnable;
	static ID3D11DepthStencilState* m_DepthStateDisable;

	static ID3D11BlendState*		m_BlendState;
	static ID3D11BlendState*		m_BlendStateATC;
	static ID3D11BlendState*		m_BlendStateAdditive;

	//以下shadowマップ用
	static ID3D11Texture2D* g_ShadowTexture;	//shadowマップの本体(Depth値)
	static ID3D11DepthStencilView* g_ShadowDSV ;	//ShadowTextureへDepthを書き込むためのView(書き込み専用)
	static ID3D11ShaderResourceView* g_ShadowSRV ;	//ShadowTextureをShaderから読むためのView(読み込み専用)
	static XMMATRIX g_LightViewMatrix;	//ライト視点カメラ
	static XMMATRIX g_LightProjectionMatrix;	//ライト用Projection
	static ID3D11Buffer* g_LightViewProjectionBuffer;	//ライト空間変換をするため(上二つの値を使ってる)



public:
	static void Init();
	static void Uninit();
	static void Begin();
	static void End();

	static void SetDepthEnable(bool Enable);
	static void SetATCEnable(bool Enable);
	static void SetAdditiveBlend(bool Enable); // true: 加算合成（光るエフェクト用）、false: 通常のアルファブレンドに戻す
	static void SetWorldViewProjection2D();
	static void SetWorldMatrix(XMMATRIX WorldMatrix);
	static void SetViewMatrix(XMMATRIX ViewMatrix);
	static void SetProjectionMatrix(XMMATRIX ProjectionMatrix);
	static void SetMaterial(MATERIAL Material);
	static void SetLight(LIGHT Light);
	static void SetCameraPosition(const XMFLOAT4& pos);

	static ID3D11Device* GetDevice( void ){ return m_Device; }
	static ID3D11DeviceContext* GetDeviceContext( void ){ return m_DeviceContext; }

	// Bloom等のMRT(複数レンダーターゲット)描画で使う。メインRTV/DSVをそのまま公開する
	static ID3D11RenderTargetView* GetMainRenderTargetView( void ){ return m_RenderTargetView; }
	static ID3D11DepthStencilView* GetDepthStencilView( void ){ return m_DepthStencilView; }
	static void CreateVertexShader(ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName);
	static void CreatePixelShader(ID3D11PixelShader** PixelShader, const char* FileName);

	// コンピュートシェーダー（GPUパーティクル等のGPGPU処理用）
	static void CreateComputeShader(ID3D11ComputeShader** ComputeShader, const char* FileName);

	// 現在のバックバッファの内容を dest へコピーする（Mosaic/Blur/Distortion等の画面キャプチャ用）
	static void CopyBackBufferTo(ID3D11Texture2D* dest);

	// オフスクリーンレンダーターゲットへの切り替え/復帰（Blur等のマルチパス処理用）
	static void SetRenderTarget(ID3D11RenderTargetView* rtv);
	static void RestoreMainRenderTarget();

	//shadowマップ用
	static void CreateShadowMap(void);
	static ID3D11ShaderResourceView* GetShadowMap(void) { return g_ShadowSRV; }
	static void BeginShadowPass();
	static void EndShadowPass();
	static void SetLightViewMatrix(XMMATRIX matrix);
	static void SetLightProjectionMatrix(XMMATRIX matrix);
	static void SetLightViewProjectionMatrix(XMMATRIX matrix);

	//ハードウェアインスタンシング
	static void CreateVertexShaderInstancing(ID3D11VertexShader** VertexShader,
		ID3D11InputLayout** VertexLayout, const char* FileName);
};
