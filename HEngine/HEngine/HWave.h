#pragma once
#include"EngineInterface.h"
#include"HManagerController.h"
struct WaveVertex
{
    XMFLOAT3 pos;
    XMFLOAT3 normal;
    XMFLOAT2 uv;
};

struct WaveConstant
{
    XMFLOAT4X4 worldTM;
    XMFLOAT4X4 worldInvTransTM;
    XMFLOAT4X4 textureTM;
};

class HWaveEffect;

class Waves : public HWaveData
{
public:
    Waves(int m, int n, float dx, float dt, float speed, float damping);
    Waves(const Waves& rhs) = delete;
    Waves& operator=(const Waves& rhs) = delete;
    ~Waves();

    int RowCount()const;
    int ColumnCount()const;
    int VertexCount()const;
    int TriangleCount()const;
    float Width()const;
    float Depth()const;

    // Returns the solution at the ith grid point.
    const DirectX::XMFLOAT3& Position(int i)const { return mCurrSolution[i]; }

    // Returns the solution normal at the ith grid point.
    const DirectX::XMFLOAT3& Normal(int i)const { return mNormals[i]; }

    // Returns the unit tangent vector at the ith grid point in the local x-axis direction.
    const DirectX::XMFLOAT3& TangentX(int i)const { return mTangentX[i]; }

    void Update(float dt);
    void Disturb(int i, int j, float magnitude);

    void Delete()override;

private:
    int mNumRows = 0;
    int mNumCols = 0;

    int mVertexCount = 0;
    int mTriangleCount = 0;
    int mIndexCount = 0;

    // Simulation constants we can precompute.
    float mK1 = 0.0f;
    float mK2 = 0.0f;
    float mK3 = 0.0f;

    float mTimeStep = 0.0f;
    float mSpatialStep = 0.0f;

    std::vector<DirectX::XMFLOAT3> mPrevSolution;
    std::vector<DirectX::XMFLOAT3> mCurrSolution;
    std::vector<DirectX::XMFLOAT3> mNormals;
    std::vector<DirectX::XMFLOAT3> mTangentX;




    // Additional variable
public:
    HManagerController_map<HWaveEffect, std::unordered_map<void*, std::unique_ptr<Waves>>> managerController;
private:
    float m_tBaseDisturb = 0.f;
    float m_tBaseUpdate = 0.f;

    Microsoft::WRL::ComPtr<ID3D12Resource> dynamicVtxPerFrame[2];
    BYTE* mappedVertex[2];
    //BYTE* mappedVertex;

    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
    SharedGraphicsResource constantBuffer;

    // Additional functions
    void CreateDynamicVertexBufferNIndexBuffer(unsigned int vertexCnt);
public:
    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetConstantBufferGpuAddress() { return constantBuffer.GpuAddress(); }
    UINT GetIndexCnt() { return mIndexCount; }
};