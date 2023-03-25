#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
#include "StimulatingVertices.h"
#include "RenderStates.h"
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance), 
	//初始化列表
	m_IndexCount(),
	m_VSConstantBuffer(),
	m_PSConstantBuffer(),
	m_DirLight(),
	m_PointLight(),
	m_SpotLight(),

	cloth(nullptr),
	m_CurrIndex(0),
	m_InitVertexCounts(24),
	angle(0),
	viewPos(0.0f, 0.0f, -3.0f), //初始相机位置
	viewRot(0.0f, 0.0f, 0.0f),
	mirrorPosX(60.0f)
{
}

GameApp::~GameApp()
{
	//delete name;// 释放内存
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	// 务必先初始化所有渲染状态，以供下面的特效使用
	RenderStates::InitAll(m_pd3dDevice.Get());
	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	if (!InitEffect())
		return false;

	if (!InitResource())
		return false;

	// 初始化鼠标，键盘不需要
	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);

	return true;
}

void GameApp::OnResize()
{
	D3DApp::OnResize();
}

void GameApp::UpdateScene(float dt)
{
	processKeyboardInput(dt);

	angle += 1.5f * dt;

	cloth->step();
	
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


	// 绘制分形法生成球体 BEGIN
	UINT offset = 0;
	UINT stride = sizeof(VertexPosNormalColor);
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffers[m_CurrIndex].GetAddressOf(), &stride, &offset);
	SetRenderSplitedSphere(m_pd3dImmediateContext.Get());
	// 除了索引为0的缓冲区缺少内部图元数目记录，其余都可以使用DrawAuto方法
	if (m_CurrIndex == 0)
	{
		m_pd3dImmediateContext->Draw(m_InitVertexCounts, 0);
	}
	else
	{
		m_pd3dImmediateContext->DrawAuto();
	}
	// 绘制分形法生成球体 END


	//// 绘制Gemotry.h自带的球体
	//auto meshData = Geometry::CreateBox<VertexPosNormalColor>(100.0f, 5.0f, 100.0f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	//ResetMesh(meshData);//更新顶点缓冲区m_pIndexBuffer和索引缓冲区
	//m_pd3dImmediateContext->DrawIndexed(m_IndexCount, 0, 0);


	// 绘制布料
    cloth->update_stimulatingvertices_POS();

	m_pd3dImmediateContext->PSSetShader(m_pPixelShaderCloth.Get(), nullptr, 0);
	m_pd3dImmediateContext->RSSetState(RenderStates::RSNoCull.Get());
	ResetMeshWithStimulatingVertices();
	m_pd3dImmediateContext->DrawIndexed(cloth->GetIndexCount(), 0, 0);


	HR(m_pSwapChain->Present(0, 0));
}

bool GameApp::InitEffect()
{
	ComPtr<ID3DBlob> blob;

	// 创建顶点着色器
	HR(CreateShaderFromFile(L"HLSL\\Light_VS.cso", L"HLSL\\Light_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader.GetAddressOf()));
	// 创建并绑定顶点布局
	HR(m_pd3dDevice->CreateInputLayout(VertexPosNormalColor::inputLayout, ARRAYSIZE(VertexPosNormalColor::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout.GetAddressOf()));

	// 创建几何着色器
	HR(CreateShaderFromFile(L"HLSL\\Light_GS.cso", L"HLSL\\Light_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pGeometryShader.GetAddressOf()));

	// 创建几何着色器
	HR(CreateShaderFromFile(L"HLSL\\LinerGS.cso", L"HLSL\\LinerGS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pGeometryShader_line.GetAddressOf()));

	// 创建像素着色器
	HR(CreateShaderFromFile(L"HLSL\\Light_PS.cso", L"HLSL\\Light_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShader.GetAddressOf()));

	// 创建新的三个shader
	// 创建顶点着色器(TeX)
	HR(CreateShaderFromFile(L"HLSL\\Basic_VS_3D.cso", L"HLSL\\Basic_VS_3D.hlsl", "VS_3D", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShaderTex.GetAddressOf()));
	// 创建顶点布局(TeX)
	HR(m_pd3dDevice->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayoutTex.GetAddressOf()));
	// 创建像素着色器(3D)
	HR(CreateShaderFromFile(L"HLSL\\Basic_PS_3D.cso", L"HLSL\\Basic_PS_3D.hlsl", "PS_3D", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShaderTex.GetAddressOf()));

	// ******************
	// 流输出分形球体
	//
	HR(CreateShaderFromFile(L"HLSL\\SphereSO_VS.cso", L"HLSL\\SphereSO_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pSphereSOVS.GetAddressOf()));
	//// 创建顶点输入布局
	//HR(m_pd3dDevice->CreateInputLayout(VertexPosNormalColor::inputLayout, ARRAYSIZE(VertexPosNormalColor::inputLayout), blob->GetBufferPointer(),
	//	blob->GetBufferSize(), m_pVertexPosNormalColorLayout.GetAddressOf()));
	const D3D11_SO_DECLARATION_ENTRY posNormalColorLayout[3] = {
		{ 0, "POSITION", 0, 0, 3, 0 },
		{ 0, "NORMAL", 0, 0, 3, 0 },
		{ 0, "COLOR", 0, 0, 4, 0 }
	};
	UINT stridePosNormalColor = sizeof(VertexPosNormalColor);

	HR(CreateShaderFromFile(L"HLSL\\SphereSO_GS.cso", L"HLSL\\SphereSO_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(), posNormalColorLayout, ARRAYSIZE(posNormalColorLayout),
		&stridePosNormalColor, 1, D3D11_SO_NO_RASTERIZED_STREAM, nullptr, m_pSphereSOGS.GetAddressOf()));

	// 创建Cloth用的像素着色器
	HR(CreateShaderFromFile(L"HLSL\\Cloth_PS.cso", L"HLSL\\Cloth_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShaderCloth.GetAddressOf()));

	return true;
}

bool GameApp::InitResource()
{
	// ******************
	// 初始化网格模型
	// 设置顶点缓冲区、索引缓冲区
	// IASetVertexBuffers、IASetIndexBuffers
	// 
	// 
						////auto meshData = Geometry::CreateBox<VertexPosNormalColor>(100.0f, 5.0f, 100.0f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
						//auto meshData = Geometry::CreateBox<VertexPosNormalColor>();
						////Geometry::MeshData<VertexPosNormalTex, WORD> meshData = Geometry::CreateBox<VertexPosNormalTex>(0.5f, 50.0f, 50.0f, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
						//ResetMesh(meshData);//更新顶点缓冲区m_pIndexBuffer和索引缓冲区
	cloth = new StimulatingVertices(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);//这句从ResetMeshWithNameVertices()中移出来了，免得每次ResetMeshWithNameVertices();都new一个，直接爆内存了。
	ResetMeshWithStimulatingVertices();

	ResetSplitedSphere();//更新顶点缓冲区m_pIndexBuffers（和上面的不一样）
	
	// ******************
	// 设置常量缓冲区描述
	//
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.ByteWidth = sizeof(VSConstantBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;//不再是D3D11_BIND_INDEX_BUFFER和D3D11_BIND_VERTEX_BUFFER
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	// 新建用于VS和PS的常量缓冲区
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[0].GetAddressOf()));
	cbd.ByteWidth = sizeof(PSConstantBuffer);
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffers[1].GetAddressOf()));

	// ******************
	// 初始化纹理和采样器状态
	//

	// 初始化木箱纹理
	/*HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\WoodCrate.dds", nullptr, m_pWoodCrate.GetAddressOf()));*/
    HR(CreateDDSTextureFromFile(m_pd3dDevice.Get(), L"Texture\\Smile.dds", nullptr, m_pWoodCrate.GetAddressOf()));
	// 初始化采样器状态
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR(m_pd3dDevice->CreateSamplerState(&sampDesc, m_pSamplerState.GetAddressOf()));


	// ******************
	// 初始化默认光照
	// 三种光源用不同颜色分开，便于观察
	// 方向光
	m_DirLight.ambient = XMFLOAT4(0.2f, 0.2f,0.2f, 1.0f);
	m_DirLight.diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_DirLight.specular = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
	m_DirLight.direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);

	// 点光
	for (int i = 0; i <= 2; i++) {
		m_PointLight[i].position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		m_PointLight[i].ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);//为了有参照物方便看出萤火虫的移动
		m_PointLight[i].diffuse = XMFLOAT4(0.9375f, 0.6289f, 0.53516f, 1.0f);
		m_PointLight[i].specular = XMFLOAT4(0.2f, 0.61f, 0.90625f, 1.0f);
		m_PointLight[i].att = XMFLOAT3(0.0f, 0.1f, 0.0f);
		m_PointLight[i].range = 30.0f;//为了有参照物方便看出萤火虫的移动
	}
	
	// 聚光灯
	m_SpotLight.position = XMFLOAT3(0.0f, 0.0f, -5.0f);
	m_SpotLight.direction = XMFLOAT3(0.2f, -1.0f, 0.6f);
	m_SpotLight.ambient = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	m_SpotLight.diffuse = XMFLOAT4(0.9375f, 0.6289f, 0.53516f, 1.0f);
	m_SpotLight.specular = XMFLOAT4(0.3f, 0.61f, 0.90625f, 1.0f);
	m_SpotLight.att = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_SpotLight.spot = 15.0f;
	m_SpotLight.range = 10000.0f;
	// 初始化用于VS的常量缓冲区的值
	m_VSConstantBuffer.world = XMMatrixIdentity();			
	m_VSConstantBuffer.view = XMMatrixTranspose(XMMatrixLookAtLH(
		XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), //m_PSConstantBuffer.eyePos = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
		XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
	));
	m_VSConstantBuffer.proj = XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV2, AspectRatio(), 1.0f, 1000.0f));
	m_VSConstantBuffer.worldInvTranspose = XMMatrixIdentity();
	

	// ******************
	// 初始化PS的常量缓冲区
	// 
	
	// 初始化用于PS的常量缓冲区的值
	m_PSConstantBuffer.material.ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_PSConstantBuffer.material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_PSConstantBuffer.material.specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 5.0f);
	// 使用默认平行光
	m_PSConstantBuffer.dirLight = m_DirLight;
	// 注意不要忘记设置此处的观察位置，否则高亮部分会有问题
	m_PSConstantBuffer.eyePos = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
	// 设置默认g_SphereCenter和g_SphereRadius
	//m_PSConstantBuffer.g_SphereCenter = XMFLOAT3(0.0f, 0.0f, 0.0f);
	//m_PSConstantBuffer.g_SphereRadius = 3.0f;

	// 更新PS常量缓冲区资源
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(PSConstantBuffer), &m_VSConstantBuffer, sizeof(PSConstantBuffer));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);


	// ******************
	// 初始化光栅化状态
	//
	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));
	rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = false;
	rasterizerDesc.DepthClipEnable = true;
	HR(m_pd3dDevice->CreateRasterizerState(&rasterizerDesc, m_pRSWireframe.GetAddressOf()));


	// ******************
	// 给渲染管线各个阶段绑定好所需资源
	//

	// 设置图元类型，设定输入布局
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout.Get());
	// 将着色器绑定到渲染管线
	m_pd3dImmediateContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	// VS常量缓冲区对应HLSL寄存于b0的常量缓冲区
	m_pd3dImmediateContext->VSSetConstantBuffers(0, 1, m_pConstantBuffers[0].GetAddressOf());
	// GS常量缓冲区对应HLSL寄存于b0的常量缓冲区
	//m_pd3dImmediateContext->GSSetShader(m_pGeometryShader_line.Get(), nullptr, 0); //：将几何着色器绑定到渲染管线
	//m_pd3dImmediateContext->GSSetConstantBuffers(0, 1, m_pConstantBuffers[0].GetAddressOf());//：GS需要访问和VS一样的常量缓冲区
	// PS常量缓冲区对应HLSL寄存于b1的常量缓冲区
	m_pd3dImmediateContext->PSSetConstantBuffers(1, 1, m_pConstantBuffers[1].GetAddressOf());
	m_pd3dImmediateContext->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());// 像素着色阶段设置好采样器
	m_pd3dImmediateContext->PSSetShaderResources(0, 1, m_pWoodCrate.GetAddressOf());
	m_pd3dImmediateContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	

	// ******************
	// 设置调试对象名
	//
	D3D11SetDebugObjectName(m_pVertexLayout.Get(), "VertexPosNormalTexLayout");
	D3D11SetDebugObjectName(m_pVertexLayoutTex.Get(), "VertexPosNormalTexLayout");
	D3D11SetDebugObjectName(m_pConstantBuffers[0].Get(), "VSConstantBuffer");
	D3D11SetDebugObjectName(m_pConstantBuffers[1].Get(), "PSConstantBuffer");
	D3D11SetDebugObjectName(m_pVertexShader.Get(), "Light_VS");
	D3D11SetDebugObjectName(m_pGeometryShader.Get(), "Light_GS");
	D3D11SetDebugObjectName(m_pGeometryShader_line.Get(), "LinerGS");
	D3D11SetDebugObjectName(m_pPixelShader.Get(), "Light_PS");
	D3D11SetDebugObjectName(m_pVertexShaderTex.Get(), "Basic_VS_Tex");
	D3D11SetDebugObjectName(m_pPixelShaderTex.Get(), "Basic_PS_Tex");

	return true;
}

template<class VertexType>
bool GameApp::ResetMesh(const Geometry::MeshData<VertexType>& meshData)
{
	// 释放旧资源
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();


	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = (UINT)meshData.vertexVec.size() * sizeof(VertexType);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = meshData.vertexVec.data();
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.GetAddressOf()));

	// 输入装配阶段的顶点缓冲区设置
	UINT stride = sizeof(VertexType);			// 跨越字节数
	UINT offset = 0;							// 起始偏移量

	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);



	// 设置索引缓冲区描述
	m_IndexCount = (UINT)meshData.indexVec.size();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(WORD) * m_IndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// 新建索引缓冲区
	InitData.pSysMem = meshData.indexVec.data();
	HR(m_pd3dDevice->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf()));
	// 输入装配阶段的索引缓冲区设置
	m_pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);



	// 设置调试对象名
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), "VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), "IndexBuffer");

	return true;
}

void GameApp::processKeyboardInput(float dt) {
	// 获取鼠标状态
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	// 获取键盘状态
	Keyboard::State keyState = m_pKeyboard->GetState();
	Keyboard::State lastKeyState = m_KeyboardTracker.GetLastState();
	// 更新鼠标按钮状态跟踪器，仅当鼠标按住的情况下才进行移动
	m_MouseTracker.Update(mouseState);
	m_KeyboardTracker.Update(keyState);

	//为点光源增加动画，使其能在字符森林里中运动，类似萤火虫
	//萤火虫效果BEGIN
	m_PointLight[0].position.z = 15.0f * sinf(2.0f * angle);
	m_PointLight[0].position.x = 12.5f * cosf(2.0f * angle) + 30.0f;
	//m_PointLight[1].position.z = m_PointLight[0].position.z + 10.0f * sinf(-2.0f * angle);
	//m_PointLight[1].position.x = m_PointLight[0].position.x + 5.0f * sinf(-2.0f * angle);
	m_PointLight[1].position.z = 15.0f * sinf(-2.0f * angle);
	m_PointLight[1].position.x = 12.5f * cosf(-2.0f * angle) + 30.0f;
	m_PointLight[2].position.z = 10.0f * sinf(-2.0f * angle);
	m_PointLight[2].position.x = 5.0f * cosf(-2.0f * angle);

	m_PSConstantBuffer.pointLight0.position = m_PointLight[0].position;
	m_PSConstantBuffer.pointLight1.position = m_PointLight[1].position;
	m_PSConstantBuffer.pointLight2.position = m_PointLight[2].position;
	//萤火虫效果END

	//为了旋转长方体而更新m_VSConstantBuffer.world和m_VSConstantBuffer.worldInvTranspose
	static float phi = 0.0f, theta = 0.0f;
	phi += 0.0001f, theta += 0.00015f;
	XMMATRIX W = XMMatrixRotationX(phi) * XMMatrixRotationY(theta);
	//m_VSConstantBuffer.world = XMMatrixTranspose(W);
	//m_VSConstantBuffer.worldInvTranspose = XMMatrixInverse(nullptr, W);	// 两次转置可以抵消?

	// 键盘切换灯光类型
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1))//用“1”键作为方向光开关
	{
		m_PSConstantBuffer.dirLight = m_DirLight;
		m_PSConstantBuffer.pointLight0 = PointLight();
		m_PSConstantBuffer.pointLight1 = PointLight();
		m_PSConstantBuffer.pointLight2 = PointLight();
		m_PSConstantBuffer.spotLight = SpotLight();
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2))//用“2”键作为点光源开关
	{
		m_PSConstantBuffer.dirLight = DirectionalLight();
		m_PSConstantBuffer.pointLight0 = m_PointLight[0];
		m_PSConstantBuffer.pointLight1 = m_PointLight[1];
		m_PSConstantBuffer.pointLight2 = m_PointLight[2];
		m_PSConstantBuffer.spotLight = SpotLight();
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D3))//用“3”键作为投射光开关
	{
		m_PSConstantBuffer.dirLight = DirectionalLight();
		m_PSConstantBuffer.pointLight0 = PointLight();
		m_PSConstantBuffer.pointLight1 = PointLight();
		m_PSConstantBuffer.pointLight2 = PointLight();
		m_PSConstantBuffer.spotLight = m_SpotLight;
	}

	if (m_KeyboardTracker.IsKeyPressed(Keyboard::R)) {
		cloth = new StimulatingVertices(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);//这句从ResetMeshWithNameVertices()中移出来了，免得每次ResetMeshWithNameVertices();都new一个，直接爆内存了。
	}

	// 键盘切换模型类型
	//if (m_KeyboardTracker.IsKeyPressed(Keyboard::Q))
	//{
	//	auto meshData = Geometry::CreateBox<VertexPosNormalColor>();
	//	ResetMesh(meshData);
	//}
	//else if (m_KeyboardTracker.IsKeyPressed(Keyboard::W))
	//{
	//	auto meshData = Geometry::CreateSphere<VertexPosNormalColor>();
	//	ResetMesh(meshData);
	//}
	//else if (m_KeyboardTracker.IsKeyPressed(Keyboard::E))
	//{
	//	auto meshData = Geometry::CreateCylinder<VertexPosNormalColor>();
	//	ResetMesh(meshData);
	//}
	//else if (m_KeyboardTracker.IsKeyPressed(Keyboard::R))
	//{
	//	auto meshData = Geometry::CreateCone<VertexPosNormalColor>();
	//	ResetMesh(meshData);
	//}
	// 键盘切换光栅化状态
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D4))
	{
		m_IsWireframeMode = !m_IsWireframeMode;
		m_pd3dImmediateContext->RSSetState(m_IsWireframeMode ? m_pRSWireframe.Get() : nullptr);
	}

	//操纵相机运动
	XMFLOAT3 pos = { 0, 0, 0 };
	float moveSpeed = 5.0f;
	if (keyState.IsKeyDown(Keyboard::LeftShift)) moveSpeed *= 2.0f;
	if (keyState.IsKeyDown(Keyboard::W)) pos.z += moveSpeed * dt;
	if (keyState.IsKeyDown(Keyboard::S)) pos.z -= moveSpeed * dt;
	pos.z += (mouseState.scrollWheelValue - lastMouseState.scrollWheelValue) / 60 * 1.0f;//使用滚轮“放大”/“缩小”相机视野。虽然只有一行代码但不太好想阿。
	if (keyState.IsKeyDown(Keyboard::A)) pos.x -= moveSpeed * dt;
	if (keyState.IsKeyDown(Keyboard::D)) pos.x += moveSpeed * dt;
	if (keyState.IsKeyDown(Keyboard::Z)) pos.y += moveSpeed * dt;//像unity一样可以垂直起降(不过忘了unity用的是哪两个键控制了，随便选了Z和C)
	if (keyState.IsKeyDown(Keyboard::C)) pos.y -= moveSpeed * dt;
	if (keyState.IsKeyDown(Keyboard::Q)) viewRot.z += 0.5f * dt;
	if (keyState.IsKeyDown(Keyboard::E)) viewRot.z -= 0.5f * dt;

	//懒得一直按鼠标移动视角可以按Tab键切换到这个模式。
	if (keyState.IsKeyDown(Keyboard::Tab) && lastKeyState.IsKeyUp(Keyboard::Tab)) {
		isLazyMouse = !isLazyMouse;
	}
	if (isLazyMouse || mouseState.leftButton == true && m_MouseTracker.leftButton == m_MouseTracker.HELD) // 这两者似乎只有在鼠标按下的那一帧存在区别(此时左为true，右为false)?
	{
		viewRot.y += (mouseState.x - lastMouseState.x) * 0.8f * dt;
		viewRot.x += (mouseState.y - lastMouseState.y) * 0.8f * dt;
	}

	if (viewRot.x > 1.5f) viewRot.x = 1.5f;
	else if (viewRot.x < -1.5f) viewRot.x = -1.5f;
	if (viewRot.z > 0.7f) viewRot.z = 0.7f;
	else if (viewRot.z < -0.7f) viewRot.z = -0.7f;

	XMStoreFloat3(&pos, XMVector3Transform(XMLoadFloat3(&pos),  XMMatrixRotationX(viewRot.x) * XMMatrixRotationZ(viewRot.z) * XMMatrixRotationY(viewRot.y)));
	viewPos.x += pos.x;
	viewPos.y += pos.y;
	viewPos.z += pos.z;

	//投射光源与相机绑定，作为飞行相机的探照灯，照亮其前方的一个区域
	//探照灯功能BEGIN
	m_SpotLight.position = viewPos;
	m_PSConstantBuffer.spotLight.position = m_SpotLight.position;
    m_PSConstantBuffer.eyePos.x = viewPos.x;
	m_PSConstantBuffer.eyePos.y = viewPos.y;
	m_PSConstantBuffer.eyePos.z = viewPos.z;
    DirectX::XMFLOAT3 initialLookTo = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
	XMStoreFloat3(&m_SpotLight.direction, XMVector3Transform(XMLoadFloat3(&initialLookTo),  XMMatrixRotationX(viewRot.x) * XMMatrixRotationZ(viewRot.z) * XMMatrixRotationY(viewRot.y)));
	m_PSConstantBuffer.spotLight.direction = m_SpotLight.direction;
	//探照灯功能END
	m_VSConstantBuffer.view = XMMatrixTranspose(XMMatrixTranslation(-viewPos.x, -viewPos.y, -viewPos.z) * XMMatrixRotationY(-viewRot.y) * XMMatrixRotationZ(-viewRot.z) * XMMatrixRotationX(-viewRot.x));



	// 更新常量缓冲区
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[0].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(VSConstantBuffer), &m_VSConstantBuffer, sizeof(VSConstantBuffer));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[0].Get(), 0);

	HR(m_pd3dImmediateContext->Map(m_pConstantBuffers[1].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
	memcpy_s(mappedData.pData, sizeof(PSConstantBuffer), &m_PSConstantBuffer, sizeof(PSConstantBuffer));
	m_pd3dImmediateContext->Unmap(m_pConstantBuffers[1].Get(), 0);


	//
	UINT offset = 0;
	UINT stride = sizeof(VertexPosNormalColor);
	for (int i = 0; i < 7; ++i)
	{
		if (m_KeyboardTracker.IsKeyPressed((Keyboard::Keys)((int)Keyboard::D5 + i)))
		{
			m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffers[i].GetAddressOf(), &stride, &offset);
			m_CurrIndex = i;
		}
	}
}

void GameApp::ResetSplitedSphere()
{
    // ******************
    // 分形球体
    //

    VertexPosNormalColor basePoint[] = {
        { XMFLOAT3(0.0f, 2.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(2.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, 0.0f, 2.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-2.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, 0.0f, -2.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.0f, -2.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
    };
	for (auto& point : basePoint) {
		point.pos.x *= 0.5 * 0.3;
		point.pos.y *= 0.5 * 0.3;
		point.pos.z *= 0.5 * 0.3;
	}
    int indices[] = { 0, 2, 1, 0, 3, 2, 0, 4, 3, 0, 1, 4, 1, 2, 5, 2, 3, 5, 3, 4, 5, 4, 1, 5 };

    std::vector<VertexPosNormalColor> vertices;
    for (int pos : indices)
    {
        vertices.push_back(basePoint[pos]);
    }


    // 设置顶点缓冲区描述
    D3D11_BUFFER_DESC vbd;
    ZeroMemory(&vbd, sizeof(vbd));
    vbd.Usage = D3D11_USAGE_DEFAULT;    // 这里需要允许流输出阶段通过GPU写入
    vbd.ByteWidth = (UINT)(vertices.size() * sizeof(VertexPosNormalColor));
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;    // 需要额外添加流输出标签
    vbd.CPUAccessFlags = 0;
    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices.data();
    HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffers[0].ReleaseAndGetAddressOf()));

//#if defined(DEBUG) | defined(_DEBUG)
//    ComPtr<IDXGraphicsAnalysis> graphicsAnalysis;
//    HR(DXGIGetDebugInterface1(0, __uuidof(graphicsAnalysis.Get()), reinterpret_cast<void**>(graphicsAnalysis.GetAddressOf())));
//    graphicsAnalysis->BeginCapture();
//#endif

    // 顶点数
    m_InitVertexCounts = 24;
    // 初始化所有顶点缓冲区
    for (int i = 1; i < 7; ++i)
    {
        vbd.ByteWidth *= 4;
        HR(m_pd3dDevice->CreateBuffer(&vbd, nullptr, m_pVertexBuffers[i].ReleaseAndGetAddressOf()));
        SetStreamOutputSplitedSphere(m_pd3dImmediateContext.Get(), m_pVertexBuffers[i - 1].Get(), m_pVertexBuffers[i].Get());
        // 第一次绘制需要调用一般绘制指令，之后就可以使用DrawAuto了
        if (i == 1)
        {
            m_pd3dImmediateContext->Draw(m_InitVertexCounts, 0);
        }
        else
        {
            m_pd3dImmediateContext->DrawAuto();
        }
    }

	UINT offset = 0;
	UINT stride = sizeof(VertexPosNormalColor);
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffers[0].GetAddressOf(), &stride, &offset);
}

void GameApp::SetStreamOutputSplitedSphere(ID3D11DeviceContext * deviceContext, ID3D11Buffer * vertexBufferIn, ID3D11Buffer * vertexBufferOut)
{
	// 先恢复流输出默认设置，防止顶点缓冲区同时绑定在输入和输出阶段
	UINT stride = sizeof(VertexPosNormalColor);
	UINT offset = 0;
	ID3D11Buffer * nullBuffer = nullptr;
	deviceContext->SOSetTargets(1, &nullBuffer, &offset);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(m_pVertexLayout.Get());

	deviceContext->IASetVertexBuffers(0, 1, &vertexBufferIn, &stride, &offset);

	deviceContext->VSSetShader(m_pSphereSOVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(m_pSphereSOGS.Get(), nullptr, 0);

	deviceContext->SOSetTargets(1, &vertexBufferOut, &offset);

	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(nullptr, nullptr, 0);

}

void GameApp::SetRenderSplitedSphere(ID3D11DeviceContext * deviceContext)
{
	// 先恢复流输出默认设置，防止顶点缓冲区同时绑定在输入和输出阶段
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	ID3D11Buffer* nullBuffer = nullptr;
	deviceContext->SOSetTargets(1, &nullBuffer, &offset);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(m_pVertexLayout.Get());
	deviceContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);

	deviceContext->GSSetShader(nullptr, nullptr, 0);

	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

}


bool GameApp::ResetMeshWithStimulatingVertices()
{
	// 释放旧资源
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();

	// 创建对象，绘制类型为0、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、、
		//移到initResource()中了
	// 设置三角形顶点
	VertexPosNormalColor* vertices = cloth->GetStimulatingVertices(); // 通过对象获取顶点、、、、、、、、、、、、、、、、、、、、、

	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = (sizeof * vertices) * cloth->GetVerticesCount(); // 计算位宽、、、、、、、、、、、、、、、、、、、、、、、、、
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.GetAddressOf()));

	// 输入装配阶段的顶点缓冲区设置
	UINT stride = sizeof(VertexPosNormalColor);	// 跨越字节数
	UINT offset = 0;							// 起始偏移量

	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);



	// 设置索引缓冲区描述
	WORD* indices = cloth->GetStimulatingIndices(); // 通过对象获取索引、、、、、、、、、、、、、、、、、、、、、
	m_IndexCount = cloth->GetIndexCount();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = (sizeof * indices) * cloth->GetIndexCount(); // 计算位宽、、、、、、、、、、、、、、、、、、、、、、、、、
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// 新建索引缓冲区
	InitData.pSysMem = indices;
	HR(m_pd3dDevice->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf()));
	// 输入装配阶段的索引缓冲区设置
	m_pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);



	// 设置调试对象名
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), "VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), "IndexBuffer");

	return true;
}