#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Geometry.h"
#include "LightHelper.h"


class StimulatingVertices;// 声明类， 避免头文件与类之间的相互依赖

class GameApp : public D3DApp
{
public:

	struct VSConstantBuffer
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX proj;
		DirectX::XMMATRIX worldInvTranspose;
		
	};

	struct PSConstantBuffer
	{
		DirectionalLight dirLight;
		PointLight pointLight0;
		PointLight pointLight1;
		PointLight pointLight2;
		SpotLight spotLight;
		Material material;
		//DirectX::XMFLOAT3 g_SphereCenter;
		//float g_SphereRadius;
		DirectX::XMFLOAT4 eyePos;
	};



public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();


private:
	bool InitEffect();
	bool InitResource();
    void processKeyboardInput(float dt);
    void SetStreamOutputSplitedSphere(ID3D11DeviceContext * deviceContext, ID3D11Buffer * vertexBufferIn, ID3D11Buffer * vertexBufferOut);
    void ResetSplitedSphere();
	void SetRenderSplitedSphere(ID3D11DeviceContext * deviceContext);

	template<class VertexType>
	bool ResetMesh(const Geometry::MeshData<VertexType>& meshData);
	bool ResetMeshWithStimulatingVertices();


private:
	ComPtr<ID3D11InputLayout> m_pVertexLayout;	    // 顶点输入布局
	ComPtr<ID3D11InputLayout> m_pVertexLayoutTex;	//用于纹理的顶点输入布局
	ComPtr<ID3D11Buffer> m_pVertexBuffer;			// 顶点缓冲区
	ComPtr<ID3D11Buffer> m_pVertexBuffers[9];			// 顶点缓冲区
	ComPtr<ID3D11Buffer> m_pIndexBuffer;			// 索引缓冲区
	ComPtr<ID3D11Buffer> m_pConstantBuffers[2];	    // 常量缓冲区
	UINT m_IndexCount;							    // 绘制物体的索引数组大小

	ComPtr<ID3D11ShaderResourceView> m_pWoodCrate;			    // 木盒纹理
	ComPtr<ID3D11SamplerState> m_pSamplerState;				    // 采样器状态

	ComPtr<ID3D11VertexShader> m_pVertexShader;	    // 顶点着色器
	ComPtr<ID3D11GeometryShader> m_pGeometryShader;	// 几何着色器
	ComPtr<ID3D11GeometryShader> m_pGeometryShader_line;	// 几何着色器
	ComPtr<ID3D11PixelShader> m_pPixelShader;		// 像素着色器
	ComPtr<ID3D11PixelShader> m_pPixelShaderCloth;		// 像素着色器
	ComPtr<ID3D11VertexShader> m_pVertexShaderTex;	// 用于纹理的顶点着色器
	ComPtr<ID3D11PixelShader> m_pPixelShaderTex;	// 用于纹理的像素着色器
	ComPtr<ID3D11VertexShader> m_pSphereSOVS;	    // SO VS顶点着色器
	ComPtr<ID3D11GeometryShader> m_pSphereSOGS;	// 几何着色器

	VSConstantBuffer m_VSConstantBuffer;			// 用于修改用于VS的GPU常量缓冲区的变量
	PSConstantBuffer m_PSConstantBuffer;			// 用于修改用于PS的GPU常量缓冲区的变量

	DirectionalLight m_DirLight;					// 默认环境光
	PointLight m_PointLight[3];						// 默认点光
	SpotLight m_SpotLight;						    // 默认汇聚光

	ComPtr<ID3D11RasterizerState> m_pRSWireframe;	// 光栅化状态: 线框模式
	bool m_IsWireframeMode;							// 当前是否为线框模式
	
	StimulatingVertices* cloth;
	float angle;
	DirectX::XMFLOAT3 viewPos;
	DirectX::XMFLOAT3 viewRot;
	bool isLazyMouse = false;
	float mirrorPosX;
	UINT m_InitVertexCounts;
	UINT m_CurrIndex;
};


#endif