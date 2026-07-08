
#include "main.h"
#include "renderer.h"
#include <io.h>

D3D_FEATURE_LEVEL       Renderer::m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;

ID3D11Device*           Renderer::m_Device{};
ID3D11DeviceContext*    Renderer::m_DeviceContext{};
IDXGISwapChain*         Renderer::m_SwapChain{};
ID3D11RenderTargetView* Renderer::m_RenderTargetView{};
ID3D11DepthStencilView* Renderer::m_DepthStencilView{};

ID3D11Buffer*			Renderer::m_WorldBuffer{};
ID3D11Buffer*			Renderer::m_ViewBuffer{};
ID3D11Buffer*			Renderer::m_ProjectionBuffer{};
ID3D11Buffer*			Renderer::m_MaterialBuffer{};
ID3D11Buffer*			Renderer::m_LightBuffer{};
ID3D11Buffer*			Renderer::m_CameraBuffer{};


ID3D11DepthStencilState* Renderer::m_DepthStateEnable{};
ID3D11DepthStencilState* Renderer::m_DepthStateDisable{};


ID3D11BlendState*		Renderer::m_BlendState{};
ID3D11BlendState*		Renderer::m_BlendStateATC{};
ID3D11BlendState*		Renderer::m_BlendStateAdditive{};

//以下shadowマップ用 コメントはヘッダー参照
ID3D11Texture2D*			Renderer::g_ShadowTexture{};	
ID3D11DepthStencilView*		Renderer::g_ShadowDSV{};	
ID3D11ShaderResourceView*	Renderer::g_ShadowSRV{};	
XMMATRIX					Renderer::g_LightViewMatrix{};
XMMATRIX					Renderer::g_LightProjectionMatrix{};
ID3D11Buffer*				Renderer::g_LightViewProjectionBuffer{};


void Renderer::Init()
{
	HRESULT hr = S_OK;




	// デバイス、スワップチェーン作成
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = SCREEN_WIDTH;
	swapChainDesc.BufferDesc.Height = SCREEN_HEIGHT;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = GetWindow();
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;

	hr = D3D11CreateDeviceAndSwapChain( NULL,
										D3D_DRIVER_TYPE_HARDWARE,
										NULL,
										0,
										NULL,
										0,
										D3D11_SDK_VERSION,
										&swapChainDesc,
										&m_SwapChain,
										&m_Device,
										&m_FeatureLevel,
										&m_DeviceContext );






	// レンダーターゲットビュー作成
	ID3D11Texture2D* renderTarget{};
	m_SwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&renderTarget );
	m_Device->CreateRenderTargetView( renderTarget, NULL, &m_RenderTargetView );
	renderTarget->Release();


	// デプスステンシルバッファ作成
	ID3D11Texture2D* depthStencile{};
	D3D11_TEXTURE2D_DESC textureDesc{};
	textureDesc.Width = swapChainDesc.BufferDesc.Width;
	textureDesc.Height = swapChainDesc.BufferDesc.Height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_D16_UNORM;
	textureDesc.SampleDesc = swapChainDesc.SampleDesc;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	m_Device->CreateTexture2D(&textureDesc, NULL, &depthStencile);

	// デプスステンシルビュー作成
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = textureDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Flags = 0;
	m_Device->CreateDepthStencilView(depthStencile, &depthStencilViewDesc, &m_DepthStencilView);
	depthStencile->Release();


	m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);

	CreateShadowMap();


	// ビューポート設定
	D3D11_VIEWPORT viewport;
	viewport.Width = (FLOAT)SCREEN_WIDTH;
	viewport.Height = (FLOAT)SCREEN_HEIGHT;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	m_DeviceContext->RSSetViewports( 1, &viewport );



	// ラスタライザステート設定
	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID; 
	rasterizerDesc.CullMode = D3D11_CULL_BACK; 
	rasterizerDesc.DepthClipEnable = TRUE; 
	rasterizerDesc.MultisampleEnable = FALSE; 

	ID3D11RasterizerState *rs;
	m_Device->CreateRasterizerState( &rasterizerDesc, &rs );

	m_DeviceContext->RSSetState( rs );




	// ブレンドステート設定
	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	m_Device->CreateBlendState( &blendDesc, &m_BlendState );

	blendDesc.AlphaToCoverageEnable = TRUE;
	m_Device->CreateBlendState( &blendDesc, &m_BlendStateATC );

	// 加算合成ブレンドステート（爆炎・火花・爆風リングなど「光る」エフェクト用）
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.RenderTarget[0].SrcBlend  = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	m_Device->CreateBlendState( &blendDesc, &m_BlendStateAdditive );

	float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	m_DeviceContext->OMSetBlendState(m_BlendState, blendFactor, 0xffffffff );





	// デプスステンシルステート設定
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	depthStencilDesc.StencilEnable = FALSE;

	m_Device->CreateDepthStencilState( &depthStencilDesc, &m_DepthStateEnable );//深度有効ステート

	//depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ZERO;
	m_Device->CreateDepthStencilState( &depthStencilDesc, &m_DepthStateDisable );//深度無効ステート

	m_DeviceContext->OMSetDepthStencilState( m_DepthStateEnable, NULL );




	// サンプラーステート設定
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxAnisotropy = 4;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState* samplerState{};
	m_Device->CreateSamplerState( &samplerDesc, &samplerState );

	// 既存のコード：スロット 0 にセット
	m_DeviceContext->PSSetSamplers(0, 1, &samplerState);

	// ★追加：スロット 1 にも同じサンプラーをセット
	m_DeviceContext->PSSetSamplers(1, 1, &samplerState);



	// 定数バッファ生成
	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.ByteWidth = sizeof(XMFLOAT4X4);
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = sizeof(float);

	m_Device->CreateBuffer( &bufferDesc, NULL, &m_WorldBuffer );
	m_DeviceContext->VSSetConstantBuffers( 0, 1, &m_WorldBuffer);

	m_Device->CreateBuffer( &bufferDesc, NULL, &m_ViewBuffer );
	m_DeviceContext->VSSetConstantBuffers( 1, 1, &m_ViewBuffer );

	m_Device->CreateBuffer( &bufferDesc, NULL, &m_ProjectionBuffer );
	m_DeviceContext->VSSetConstantBuffers( 2, 1, &m_ProjectionBuffer );

	//shadowマップ用
	{
		m_Device->CreateBuffer(&bufferDesc, NULL, &g_LightViewProjectionBuffer);

		m_DeviceContext->VSSetConstantBuffers(6, 1, &g_LightViewProjectionBuffer);
		m_DeviceContext->PSSetConstantBuffers(1, 1, &g_LightViewProjectionBuffer);

	}

	//material用の定数バッファ
	bufferDesc.ByteWidth = sizeof(MATERIAL);
	m_Device->CreateBuffer( &bufferDesc, NULL, &m_MaterialBuffer );
	m_DeviceContext->VSSetConstantBuffers( 3, 1, &m_MaterialBuffer );
	m_DeviceContext->PSSetConstantBuffers( 3, 1, &m_MaterialBuffer );

	//ライト用の定数バッファ
	bufferDesc.ByteWidth = sizeof(LIGHT);
	m_Device->CreateBuffer( &bufferDesc, NULL, &m_LightBuffer );
	m_DeviceContext->VSSetConstantBuffers( 4, 1, &m_LightBuffer );
	m_DeviceContext->PSSetConstantBuffers( 4, 1, &m_LightBuffer );

	// カメラ位置用の定数バッファ (b5)
	bufferDesc.ByteWidth = sizeof(XMFLOAT4);
	m_Device->CreateBuffer( &bufferDesc, NULL, &m_CameraBuffer );
	m_DeviceContext->VSSetConstantBuffers( 5, 1, &m_CameraBuffer );
	m_DeviceContext->PSSetConstantBuffers( 5, 1, &m_CameraBuffer );





	// ライト初期化
	LIGHT light{};
	light.Enable = true;
	light.Direction = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	light.Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	light.Diffuse = XMFLOAT4(1.5f, 1.5f, 1.5f, 1.0f);
	SetLight(light);



	// マテリアル初期化
	MATERIAL material{};
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	SetMaterial(material);

}



void Renderer::Uninit()
{

	m_WorldBuffer->Release();
	m_ViewBuffer->Release();
	m_ProjectionBuffer->Release();
	m_LightBuffer->Release();
	m_MaterialBuffer->Release();
	m_CameraBuffer->Release();


	m_DeviceContext->ClearState();
	m_RenderTargetView->Release();
	m_SwapChain->Release();
	m_DeviceContext->Release();
	m_Device->Release();

}




void Renderer::Begin()
{
	float clearColor[4] = { 0.627f,0.847f,0.937f,1.0f };
	m_DeviceContext->ClearRenderTargetView( m_RenderTargetView, clearColor );
	m_DeviceContext->ClearDepthStencilView( m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}



void Renderer::End()
{
	m_SwapChain->Present( 1, 0 );
}

void Renderer::CopyBackBufferTo(ID3D11Texture2D* dest)
{
	ID3D11Resource* backBuffer = nullptr;
	m_RenderTargetView->GetResource(&backBuffer);
	m_DeviceContext->CopyResource(dest, backBuffer);
	backBuffer->Release();
}

void Renderer::SetRenderTarget(ID3D11RenderTargetView* rtv)
{
	// ポストプロセスのオフスクリーンパスなので深度は使わない
	m_DeviceContext->OMSetRenderTargets(1, &rtv, nullptr);
}

void Renderer::RestoreMainRenderTarget()
{
	m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);
}




void Renderer::SetDepthEnable( bool Enable )
{
	if( Enable )
		m_DeviceContext->OMSetDepthStencilState( m_DepthStateEnable, NULL );
	else
		m_DeviceContext->OMSetDepthStencilState( m_DepthStateDisable, NULL );

}



void Renderer::SetATCEnable( bool Enable )
{
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (Enable)
		m_DeviceContext->OMSetBlendState(m_BlendStateATC, blendFactor, 0xffffffff);
	else
		m_DeviceContext->OMSetBlendState(m_BlendState, blendFactor, 0xffffffff);

}

void Renderer::SetAdditiveBlend(bool Enable)
{
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (Enable)
		m_DeviceContext->OMSetBlendState(m_BlendStateAdditive, blendFactor, 0xffffffff);
	else
		m_DeviceContext->OMSetBlendState(m_BlendState, blendFactor, 0xffffffff);
}

void Renderer::SetWorldViewProjection2D()
{
	SetWorldMatrix(XMMatrixIdentity());
	SetViewMatrix(XMMatrixIdentity());

	XMMATRIX projection;
	projection = XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f);
	SetProjectionMatrix(projection);
}


void Renderer::SetWorldMatrix(XMMATRIX WorldMatrix)
{
	XMFLOAT4X4 worldf;
	XMStoreFloat4x4(&worldf, XMMatrixTranspose(WorldMatrix));
	m_DeviceContext->UpdateSubresource(m_WorldBuffer, 0, NULL, &worldf, 0, 0);
}

void Renderer::SetViewMatrix(XMMATRIX ViewMatrix)
{
	XMFLOAT4X4 viewf;
	XMStoreFloat4x4(&viewf, XMMatrixTranspose(ViewMatrix));
	m_DeviceContext->UpdateSubresource(m_ViewBuffer, 0, NULL, &viewf, 0, 0);
}

void Renderer::SetProjectionMatrix(XMMATRIX ProjectionMatrix)
{
	XMFLOAT4X4 projectionf;
	XMStoreFloat4x4(&projectionf, XMMatrixTranspose(ProjectionMatrix));
	m_DeviceContext->UpdateSubresource(m_ProjectionBuffer, 0, NULL, &projectionf, 0, 0);

}



void Renderer::SetMaterial( MATERIAL Material )
{
	m_DeviceContext->UpdateSubresource( m_MaterialBuffer, 0, NULL, &Material, 0, 0 );
}

void Renderer::SetLight( LIGHT Light )
{
	m_DeviceContext->UpdateSubresource(m_LightBuffer, 0, NULL, &Light, 0, 0);
}

void Renderer::SetCameraPosition(const XMFLOAT4& pos)
{
	m_DeviceContext->UpdateSubresource(m_CameraBuffer, 0, NULL, &pos, 0, 0);
}

void Renderer::CreateVertexShader( ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName )
{

	FILE* file;
	long int fsize;

	file = fopen(FileName, "rb");
	assert(file);

	fsize = _filelength(_fileno(file));
	unsigned char* buffer = new unsigned char[fsize];
	fread(buffer, fsize, 1, file);
	fclose(file);

	m_Device->CreateVertexShader(buffer, fsize, NULL, VertexShader);


	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 6, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 10, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = ARRAYSIZE(layout);

	m_Device->CreateInputLayout(layout,
		numElements,
		buffer,
		fsize,
		VertexLayout);

	delete[] buffer;
}



void Renderer::CreatePixelShader( ID3D11PixelShader** PixelShader, const char* FileName )
{
	FILE* file;
	long int fsize;

	file = fopen(FileName, "rb");
	assert(file);

	fsize = _filelength(_fileno(file));
	unsigned char* buffer = new unsigned char[fsize];
	fread(buffer, fsize, 1, file);
	fclose(file);

	m_Device->CreatePixelShader(buffer, fsize, NULL, PixelShader);

	delete[] buffer;
}


// ============================================================
// ShadowMap用 DepthTexture 作成
// ============================================================
//
// ShadowMapでは、
// 「ライト視点から見た深度情報」を
// Textureへ保存する必要がある。
//
// このTextureは：
//
// 1. ShadowPass
//    → DepthBufferとして書き込み
//
// 2. MainPass
//    → Textureとして読み込み
//
// の両方に使用される。
//
// そのため：
//
// - DSV (DepthStencilView)
// - SRV (ShaderResourceView)
//
// の2種類のViewを作成する。
// ============================================================
void Renderer::CreateShadowMap(void)
{
	// ============================================================
	// ShadowMap本体Texture生成
	// ============================================================
	{
		D3D11_TEXTURE2D_DESC shadowDesc = {};

		// ShadowMap解像度
		// 解像度が高いほど影が綺麗になる。
		// 1024 : 軽量だがジャギーが目立つ
		// 2048 : 一般的
		// 4096 : 高品質だが重い
		//
		shadowDesc.Width = 2048;
		shadowDesc.Height = 2048;

		// mipmapは不要
		// ShadowMapは通常Mipを使わない。
		shadowDesc.MipLevels = 1;

		// 通常の2DTextureなので1枚
		// CascadedShadowやTextureArrayの場合は増える
		shadowDesc.ArraySize = 1;

		// ========================================================
		// typeless format が超重要
		// ========================================================
		// 同じTextureを：
		// - DSVとして使う- SRVとして使う
		// 必要があるため、typeless formatを使う。
		// もし D32_FLOAT 固定にすると、 SRV作成時に失敗する。
		shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;

		// MSAAなし
		// ShadowMapでは通常1でOK
		shadowDesc.SampleDesc.Count = 1;

		// GPU専用メモリ
		// CPUから触らないので DEFAULT
		shadowDesc.Usage = D3D11_USAGE_DEFAULT;

		// ========================================================
		// DSV + SRV 両方必要
		// ========================================================
		// DSV:ShadowPassでDepthを書き込む
		// SRV:MainPassでShaderから読む
		shadowDesc.BindFlags =
			D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

		// 実際にTexture生成
		// g_ShadowTexture がShadowMap本体になる
		m_Device->CreateTexture2D(&shadowDesc,
			nullptr, &g_ShadowTexture);
	}
	// ============================================================
	// DSV (DepthStencilView) 作成
	// ============================================================
	// ShadowPass時に
	// 「このTextureへDepthを書き込む」ためのView。
	// DSVが無いと、
	// ShadowMapへ深度を書き込めない。
	// ============================================================
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSVDesc = {};

		// ========================================================
		// DSVとして見る時のFormat
		// ========================================================
		// typeless texture を「DepthBuffer」として扱うため、
		// D32_FLOATへ変換する。
		shadowDSVDesc.Format = DXGI_FORMAT_D32_FLOAT;

		// 通常の2DTexture
		shadowDSVDesc.ViewDimension =
			D3D11_DSV_DIMENSION_TEXTURE2D;

		// DSV生成
		// ShadowPassでは
		// OMSetRenderTargets(0,nullptr,g_ShadowDSV);
		// のように使用する。
		m_Device->CreateDepthStencilView(g_ShadowTexture,
			&shadowDSVDesc, &g_ShadowDSV);
	}
	// ============================================================
	// SRV (ShaderResourceView) 作成
	// ============================================================
	// MainPass時にPixelShaderからShadowMapを読むためのView。
	// SRVが無いとShadowMap.Sample(...)が使えない。
	// ============================================================
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC shadowSRVDesc = {};

		// ========================================================
		// SRVとして見る時のFormat
		// ========================================================
		// typeless texture を「float texture」として扱う。
		// Shader側では ShadowMap.Sample(...)によってDepth値を取得する。
		shadowSRVDesc.Format = DXGI_FORMAT_R32_FLOAT;

		// 通常の2DTexture
		shadowSRVDesc.ViewDimension =
			D3D11_SRV_DIMENSION_TEXTURE2D;

		// mipmap数
		// 今回はMip無しなので1
		shadowSRVDesc.Texture2D.MipLevels = 1;

		// SRV生成
		// MainPassではPSSetShaderResources(...)でShaderへ渡す。
		m_Device->CreateShaderResourceView(g_ShadowTexture,
			&shadowSRVDesc, &g_ShadowSRV);
	}

}

void Renderer::BeginShadowPass()
{
	ID3D11ShaderResourceView* nullSRV = nullptr;

	m_DeviceContext->PSSetShaderResources(1,1,&nullSRV);
	// カラー出力なし
	m_DeviceContext->OMSetRenderTargets(0,nullptr,g_ShadowDSV);

	// ShadowMapクリア
	m_DeviceContext->ClearDepthStencilView(g_ShadowDSV,D3D11_CLEAR_DEPTH,1.0f,0);

	// ShadowMap用Viewport
	D3D11_VIEWPORT vp;

	vp.Width = 2048.0f;
	vp.Height = 2048.0f;

	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	vp.TopLeftX = 0;
	vp.TopLeftY = 0;

	m_DeviceContext->RSSetViewports(1, &vp);
}

void Renderer::EndShadowPass()
{
	// 通常RenderTargetへ戻す
	m_DeviceContext->OMSetRenderTargets(1,&m_RenderTargetView,m_DepthStencilView);

	// EndShadowPass() の中（RenderTargetを戻した後など）に追加
	m_DeviceContext->PSSetShaderResources(1, 1, &g_ShadowSRV);

	// 通常Viewportへ戻す
	D3D11_VIEWPORT vp;

	vp.Width = (FLOAT)SCREEN_WIDTH;
	vp.Height = (FLOAT)SCREEN_HEIGHT;

	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	vp.TopLeftX = 0;
	vp.TopLeftY = 0;

	m_DeviceContext->RSSetViewports(1, &vp);
}

void Renderer::SetLightViewMatrix(XMMATRIX matrix)
{
	g_LightViewMatrix = matrix;
}

void Renderer::SetLightProjectionMatrix(XMMATRIX matrix)
{
	g_LightProjectionMatrix = matrix;
}

void Renderer::SetLightViewProjectionMatrix(XMMATRIX matrix)
{
	matrix = XMMatrixTranspose(matrix);

	XMFLOAT4X4 mat;

	XMStoreFloat4x4(&mat, matrix);

	m_DeviceContext->UpdateSubresource(g_LightViewProjectionBuffer, 0, NULL, &mat, 0, 0);
}

void Renderer::CreateVertexShaderInstancing(ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName)
{
	FILE* file;
	long int fsize;

	file = fopen(FileName, "rb");
	assert(file);

	fsize = _filelength(_fileno(file));
	unsigned char* buffer = new unsigned char[fsize];
	fread(buffer, fsize, 1, file);
	fclose(file);

	m_Device->CreateVertexShader(buffer, fsize, NULL, VertexShader);

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		// スロット0：通常の頂点データ（1頂点ごとに変わる）
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },

		// ★スロット1：インスタンスデータ（1匹ごとに変わる）
		// 4x4行列 = float4(16バイト) × 4本
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,  0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};
	UINT numElements = ARRAYSIZE(layout);

	m_Device->CreateInputLayout(layout, numElements, buffer, fsize, VertexLayout);

	delete[] buffer;
}
