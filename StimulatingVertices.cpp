//Stimulate.cpp
#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
#include "StimulatingVertices.h"

#include <cstdio>
#include <vector>
#include <cmath>
#include <random>
#include <cstdlib>

using namespace DirectX;

struct Offset {
    int i, j;
};

//const double spring_Y = 3e4; // bug1
const double spring_Y = 1.5e4 / 1;
const double dashpot_damping = 1e4 / 4;
const double drag_damping = 1 / 10;
const double ball_center[3] = {0, 0, 0};
const double ball_radius = 0.3;

const int N = 128;
const double quad_size = 1.0 / N;
//const double quad_size = 1.0;
const double dt = 4e-2 / N;
const double gravity[3] = {0, -9.8, 0};
const int num_triangles = (N - 1) * (N - 1) * 2;

std::vector<Offset> spring_offsets;

//for generating random double
double lower_bound = -N / 10 * quad_size;
double upper_bound = N / 10 * quad_size;
 
std::uniform_real_distribution<double> unif(lower_bound, upper_bound);
std::default_random_engine re;
double random_double = unif(re);



StimulatingVertices::StimulatingVertices(D3D11_PRIMITIVE_TOPOLOGY type) 
{
    substeps = (UINT)(1.0 / 60 / dt);
    topology = type;
    initialize_spring_offsets();//bug
    random_double = unif(re);

    //更新顶点信息
    verticesCount = N * N;

    stimulatingVertices = new VertexPosNormalColor[verticesCount];
    initialize_mass_points();
    update_stimulatingvertices_POS();

    ////更新索引信息
    indexCount = 3 * num_triangles;
    stimulatingIndices = new WORD[indexCount];
    initialize_mesh_indices();

    //为了简单，不初始化Noraml。但为了方便观察，初始化一下Color
    //for i, j in ti.ndrange(n, n):
        //if (i // 4 + j // 4) % 2 == 0:
        //    colors[i * n + j] = (0., 0.5, 1)
        //else:
        //    colors[i * n + j] = (1, 0.5, 0.)
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            stimulatingVertices[i * N + j].color = (i / 4 + j / 4) % 2 == 0 ? XMFLOAT4(0.0f, 0.5f, 1.0f, 1.0f) : XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f);
        }
    }
}

StimulatingVertices::~StimulatingVertices()
{
	delete stimulatingVertices;
}

VertexPosNormalColor* StimulatingVertices::GetStimulatingVertices() {
    return stimulatingVertices;
}

WORD* StimulatingVertices::GetStimulatingIndices()
{
	return stimulatingIndices;
}

D3D11_PRIMITIVE_TOPOLOGY StimulatingVertices::GetTopology()
{
	return topology;
}

UINT StimulatingVertices::GetVerticesCount()
{
	return verticesCount;
}

UINT StimulatingVertices::GetIndexCount()
{
	return indexCount;
}
















UINT StimulatingVertices::GetSubSteps() {
    return substeps;
}

void StimulatingVertices::initialize_mass_points() {
    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++) {
            pos[i][j][0] = i * quad_size - 0.5 * N * quad_size + random_double;
            pos[i][j][1] = 0.6;
            pos[i][j][2] = j * quad_size - 0.5 * N * quad_size + random_double;
        }
}

void StimulatingVertices::initialize_spring_offsets() {
    spring_offsets.clear();
    for(int i = -2; i <= 2; i++) {
        for(int j = -2; j <= 2; j++) {
            if(i == 0 && j == 0) continue;
            if(abs(i) + abs(j) > 2) continue;
            Offset tmp = {i,j};//?
            spring_offsets.push_back(tmp);
        }
    }
}

void StimulatingVertices::initialize_mesh_indices() {
    for (int i = 0; i < N-1; i++) {
        for (int j = 0; j < N-1; j++) {
            int quad_id = i * (N-1) + j;
            // 1st triangle of the square
            stimulatingIndices[quad_id * 6 + 0] = i * N + j;
            stimulatingIndices[quad_id * 6 + 1] = (i + 1) * N + j;
            stimulatingIndices[quad_id * 6 + 2] = i * N + (j + 1);
            // 2nd triangle of the square
            stimulatingIndices[quad_id * 6 + 3] = (i + 1) * N + j + 1;
            stimulatingIndices[quad_id * 6 + 4] = i * N + (j + 1);
            stimulatingIndices[quad_id * 6 + 5] = (i + 1) * N + j;
        }
    }

}

double StimulatingVertices::length(double deltaPos[], int n) {
    double res = 0.0;
    for(int k = 0; k < n; k++) {
        res += deltaPos[k] * deltaPos[k];
    }
    res = sqrt(res);
    return res;
}

double StimulatingVertices::dot(double a[], double b[], int n) {
    double res = 0;
    for(int k = 0; k < n; k++) res += a[k] * b[k];
    return res;
}

//update pos[][] and v[][]
void StimulatingVertices::substep() {

    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            for(int k = 0; k < 3; k++) v[i][j][k] += gravity[k] * dt;
        }
    }

    for(int i = 0; i < N; i++) {
        for(int j = 0; j < N; j++) {
            double force[3] = {0.0, 0.0, 0.0};
            for(auto& offset : spring_offsets) {
                int ii = i + offset.i;
                int jj = j + offset.j;
                if(!(ii >= 0 && ii < N && jj >= 0 && jj <N)) continue;
                double deltaPos[3], deltaVel[3];
                for(int k = 0; k < 3; k++) {
                    deltaPos[k] = pos[i][j][k] - pos[ii][jj][k];
                    deltaVel[k] = v[i][j][k] - v[ii][jj][k];
                }

                double current_dist = length(deltaPos, 3);
                double d[3]; for(int k = 0; k < 3; k++) d[k] = deltaPos[k] / current_dist;
                double original_dist = quad_size * sqrt(pow((double)offset.i, 2.0) + pow((double)offset.j, 2.0));

                double deltaVel_dot__d = dot(deltaVel, d, 3);
                for(int k = 0; k < 3; k++) {
                    force[k] += -spring_Y * d[k] * (current_dist / original_dist - 1);
                    //?
                    force[k] += -deltaVel_dot__d * d[k] * dashpot_damping * quad_size;//?
                }

            }

            for(int k = 0; k < 3; k++) v[i][j][k] += force[k] * dt;
        }
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {


            for(int k = 0; k < 3; k++) {
                //?
                v[i][j][k] *= exp(-drag_damping * dt);
            }

            double offset_to_ball_center[3] = {};
            for(int k = 0; k < 3; k++)
                offset_to_ball_center[k] = pos[i][j][k] - ball_center[k];

            double d_to_ball_center = length(offset_to_ball_center, 3);
            if(d_to_ball_center <= ball_radius * 1.05) {
                double normal[3] = {};
                for(int k = 0; k < 3; k++) normal[k] = offset_to_ball_center[k] / d_to_ball_center;

                double v_dot_normal = dot(v[i][j], normal, 3);
                if(v_dot_normal < 0)
                    for(int k = 0; k < 3; k++) v[i][j][k] -= v_dot_normal * normal[k];
            }

            for(int k = 0; k < 3; k++) pos[i][j][k] += dt * v[i][j][k];
        }
    }
}

void StimulatingVertices::update_stimulatingvertices_POS() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            stimulatingVertices[i * N + j].pos.x = pos[i][j][0];
            stimulatingVertices[i * N + j].pos.y = pos[i][j][1];
            stimulatingVertices[i * N + j].pos.z = pos[i][j][2];
        }
    }
}

void StimulatingVertices::step() {
    for (UINT i = 0; i < 1; i++) {
		substep();
	}
}