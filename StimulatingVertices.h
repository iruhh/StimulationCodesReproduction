#pragma once

#include "GameApp.h"
#include "Vertex.h"

class StimulatingVertices
{
public:
	StimulatingVertices(D3D11_PRIMITIVE_TOPOLOGY type);
	~StimulatingVertices();

	// 获取顶点
	VertexPosNormalColor* GetStimulatingVertices();
	// 获取索引
	WORD* GetStimulatingIndices();
	// 获取绘制图元类型
	D3D11_PRIMITIVE_TOPOLOGY GetTopology();
	// 获取顶点个数 
	UINT GetVerticesCount();
	// 获取索引个数
	UINT GetIndexCount();
    UINT GetSubSteps();

	
	void step();
	void update_stimulatingvertices_POS();

private:
	VertexPosNormalColor* stimulatingVertices; // 顶点
	WORD* stimulatingIndices; // 索引
	D3D11_PRIMITIVE_TOPOLOGY topology;// 图元类型 
	UINT verticesCount; // 顶点个数
	UINT indexCount; // 索引个数

	UINT substeps;
    double pos[128][128][3];
	double v[128][128][3];

	double length(double deltaPos[], int n);
	double dot(double a[], double b[], int n);
	void initialize_mass_points();
	void initialize_spring_offsets();
	void initialize_mesh_indices();
	void substep();
};
