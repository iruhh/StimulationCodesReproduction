#include "LightHelper.hlsli"

cbuffer VSConstantBuffer : register(b0)
{
    matrix g_World; 
    matrix g_View;  
    matrix g_Proj;  
    matrix g_WorldInvTranspose;
}

cbuffer PSConstantBuffer : register(b1)
{
    DirectionalLight g_DirLight;
    PointLight g_PointLight0;
    PointLight g_PointLight1;//还好记得上课提到的"C++和hlsl这里的结构体得对应起来,因为这里的传递不带语义"
    PointLight g_PointLight2;
    SpotLight g_SpotLight;
    Material g_Material;
    //float3 g_SphereCenter;
    //float g_SphereRadius;
    float3 g_EyePosW;
    float g_Pad;
}



struct VertexIn
{
	float3 PosL : POSITION;
    float3 NormalL : NORMAL;//相对KeyAndBoard添加的，另外变量首字母原先没大写，现在大写了。
	float4 Color : COLOR;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
    float3 PosW : POSITION; //在世界中的位置//相对KeyAndBoard添加的
    float3 NormalW : NORMAL; //法向量在世界中的方向//相对KeyAndBoard添加的
	float4 Color : COLOR;
};


