#include "Light.hlsli"

[maxvertexcount(9)]
void GS(triangle VertexOut input[3], inout TriangleStream<VertexOut> output)
{
	float mirrorPosX = 60.0f;// 以X=mirrorPosX处为中心，将场景制作一个镜像
	float doubleMirrorPosX = 2 * mirrorPosX;

	int i;
	//original
	for (i = 0; i < 3; i++) {
		output.Append(input[i]);
	}
	output.RestartStrip();

	//clone
	for (i = 2; i >= 0; i--) {// 倒序处理三角面的三个顶点，保证镜像前后绘制的三角形顶点顺序一致，否则字符笔画前后叠压关系和打光会出错
		VertexOut vOut;

		//PosW
		vOut.PosW = input[i].PosW;
		vOut.PosW.x = doubleMirrorPosX - input[i].PosW.x;

		//PosH
		matrix viewProj = mul(g_View, g_Proj);
		vOut.PosH = mul(float4(vOut.PosW, 1.0f), viewProj);

		//NormalW
		vOut.NormalW = input[i].NormalW;
		vOut.NormalW.x = -vOut.NormalW.x;

		//Color
		vOut.Color = float4(0.5f, 0.5f, 0.5f, 1.0f);// 为生成的森林镜像修改颜色

		output.Append(vOut);
	}
}