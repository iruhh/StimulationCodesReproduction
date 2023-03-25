#include "Light.hlsli"

// 像素着色器
float4 PS(VertexOut pIn) : SV_Target
{
    // 标准化法向量
    pIn.NormalW = normalize(pIn.NormalW);

    // 顶点指向眼睛的向量
    float3 toEyeW = normalize(g_EyePosW - pIn.PosW);

    // 初始化为0 
    float4 ambient, diffuse, spec;
    float4 A, D, S;
    ambient = diffuse = spec = A = D = S = float4(0.0f, 0.0f, 0.0f, 0.0f);

    ComputeDirectionalLight(g_Material, g_DirLight, pIn.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;

    ComputePointLight(g_Material, g_PointLight0, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;
    
    ComputePointLight(g_Material, g_PointLight1, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;
    
    ComputePointLight(g_Material, g_PointLight2, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;

    ComputeSpotLight(g_Material, g_SpotLight, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;

    float4 litColor = pIn.Color * (ambient + diffuse) + spec; //相对KeyAndBoard，对pIn.Color做了些处理才输出。
	
    litColor.a = g_Material.Diffuse.a * pIn.Color.a;
	
    //float4 litColor = { 0.7f, 0.7f, 1.0f, 1.0f };
    //float4 litColor = pIn.Color;
    return litColor;
}