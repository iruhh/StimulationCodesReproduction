#include "Light.hlsli"

[maxvertexcount(12)]
void GS(triangle VertexIn input[3], inout TriangleStream<VertexIn> output)
{
    float3 g_SphereCenter = float3(0.0f, 0.0f, 0.0f);
    float g_SphereRadius = 0.3f;
    
    
    //
    // 将一个三角形分裂成四个三角形，但同时顶点v3, v4, v5也需要在球面上
    //       v1
    //       /\
    //      /  \
    //   v3/____\v4
    //    /\xxxx/\
    //   /  \xx/  \
    //  /____\/____\
    // v0    v5    v2
	
    VertexIn vertexes[6];

    matrix viewProj = mul(g_View, g_Proj);

    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        vertexes[i] = input[i];
        vertexes[i + 3].Color = lerp(input[i].Color, input[(i + 1) % 3].Color, 0.5f);
        vertexes[i + 3].NormalL = normalize(input[i].NormalL + input[(i + 1) % 3].NormalL);
        vertexes[i + 3].PosL = g_SphereCenter + g_SphereRadius * vertexes[i + 3].NormalL;
        //vertexes[i + 3].PosL = g_SphereCenter.xyz;
        //vertexes[i + 3].PosL = 1.79f;
    }
        
    output.Append(vertexes[0]);
    output.Append(vertexes[3]);
    output.Append(vertexes[5]);
    output.RestartStrip();

    output.Append(vertexes[3]);
    output.Append(vertexes[4]);
    output.Append(vertexes[5]);
    output.RestartStrip();

    output.Append(vertexes[5]);
    output.Append(vertexes[4]);
    output.Append(vertexes[2]);
    output.RestartStrip();

    output.Append(vertexes[3]);
    output.Append(vertexes[1]);
    output.Append(vertexes[4]);
}