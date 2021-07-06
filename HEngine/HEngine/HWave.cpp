#include "pch.h"
#include "HWave.h"
#include <ppl.h>
#include <algorithm>
#include <vector>
#include <cassert>
#include "DeviceResources.h"
#include<ctime>
#include"HEngine.h"

using namespace DirectX;

Waves::Waves(int m, int n, float dx, float dt, float speed, float damping)
{
	mNumRows = m;
	mNumCols = n;

	mVertexCount = m * n;
	mTriangleCount = (m - 1) * (n - 1) * 2;
	mIndexCount = mTriangleCount * 3;

	mTimeStep = dt;
	mSpatialStep = dx;

	float d = damping * dt + 2.0f;
	float e = (speed * speed) * (dt * dt) / (dx * dx);
	mK1 = (damping * dt - 2.0f) / d;
	mK2 = (4.0f - 8.0f * e) / d;
	mK3 = (2.0f * e) / d;

	mPrevSolution.resize(m * n);
	mCurrSolution.resize(m * n);
	mNormals.resize(m * n);
	mTangentX.resize(m * n);

	// Generate grid vertices in system memory.

	float halfWidth = (n - 1) * dx * 0.5f;
	float halfDepth = (m - 1) * dx * 0.5f;
	for (int i = 0; i < m; ++i)
	{
		float z = halfDepth - i * dx;
		for (int j = 0; j < n; ++j)
		{
			float x = -halfWidth + j * dx;

			mPrevSolution[i * n + j] = XMFLOAT3(x, 0.0f, z);
			mCurrSolution[i * n + j] = XMFLOAT3(x, 0.0f, z);
			mNormals[i * n + j] = XMFLOAT3(0.0f, 1.0f, 0.0f);
			mTangentX[i * n + j] = XMFLOAT3(1.0f, 0.0f, 0.0f);
		}
	}

	CreateDynamicVertexBufferNIndexBuffer(mVertexCount);
}

Waves::~Waves()
{
}

int Waves::RowCount()const
{
	return mNumRows;
}

int Waves::ColumnCount()const
{
	return mNumCols;
}

int Waves::VertexCount()const
{
	return mVertexCount;
}

int Waves::TriangleCount()const
{
	return mTriangleCount;
}

float Waves::Width()const
{
	return mNumCols * mSpatialStep;
}

float Waves::Depth()const
{
	return mNumRows * mSpatialStep;
}

void Waves::Update(float dt)
{
	DX::DeviceResources* pDeviceResource =
		DX::DeviceResources::GetInstance();
	unsigned int frameIndex = pDeviceResource->GetCurrentFrameIndex();
	
	// Accumulate time.
	m_tBaseDisturb += dt;
	m_tBaseUpdate += dt;

	if (m_tBaseDisturb >= 0.25f)
	{
		
		unsigned int randomNum0 = std::rand();
		unsigned int randomNum1 = std::rand();

		float randomNumf = std::rand() / (float)RAND_MAX;

		// return a + rand() % ((b - a) + 1);
		unsigned int rowIndex = 4 + randomNum0 % (mNumRows - 5 - 4 + 1);
		unsigned int columnIndex = 4 + randomNum1 % (mNumCols - 5 - 4 + 1);
		randomNumf = 0.2f + randomNumf * (0.5f - 0.2f);

		Disturb(rowIndex, columnIndex, randomNumf);

		m_tBaseDisturb = 0.f;
	}

	// Only update the simulation at the specified time step.
	if (m_tBaseUpdate >= mTimeStep)
	{
		// Only update interior points; we use zero boundary conditions.
		concurrency::parallel_for(1, mNumRows - 1, [this](int i)
		//for (int i = 1; i < mNumRows - 1; ++i)
		{
			for (int j = 1; j < mNumCols - 1; ++j)
			{
				// After this update we will be discarding the old previous
				// buffer, so overwrite that buffer with the new update.
				// Note how we can do this inplace (read/write to same element) 
				// because we won't need prev_ij again and the assignment happens last.

				// Note j indexes x and i indexes z: h(x_j, z_i, t_k)
				// Moreover, our +z axis goes "down"; this is just to 
				// keep consistent with our row indices going down.

				mPrevSolution[i * mNumCols + j].y =
					mK1 * mPrevSolution[i * mNumCols + j].y +
					mK2 * mCurrSolution[i * mNumCols + j].y +
					mK3 * (mCurrSolution[(i + 1) * mNumCols + j].y +
						mCurrSolution[(i - 1) * mNumCols + j].y +
						mCurrSolution[i * mNumCols + j + 1].y +
						mCurrSolution[i * mNumCols + j - 1].y);
			}
		});

		// We just overwrote the previous buffer with the new data, so
		// this data needs to become the current solution and the old
		// current solution becomes the new previous solution.
		std::swap(mPrevSolution, mCurrSolution);

		m_tBaseUpdate = 0.0f; // reset time

		//
		// Compute normals using finite difference scheme.
		//
		concurrency::parallel_for(1, mNumRows - 1, [this](int i)
			//for(int i = 1; i < mNumRows - 1; ++i)
			{
				for (int j = 1; j < mNumCols - 1; ++j)
				{
					float l = mCurrSolution[i * mNumCols + j - 1].y;
					float r = mCurrSolution[i * mNumCols + j + 1].y;
					float t = mCurrSolution[(i - 1) * mNumCols + j].y;
					float b = mCurrSolution[(i + 1) * mNumCols + j].y;
					mNormals[i * mNumCols + j].x = -r + l;
					mNormals[i * mNumCols + j].y = 2.0f * mSpatialStep;
					mNormals[i * mNumCols + j].z = b - t;

					XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&mNormals[i * mNumCols + j]));
					XMStoreFloat3(&mNormals[i * mNumCols + j], n);

					mTangentX[i * mNumCols + j] = XMFLOAT3(2.0f * mSpatialStep, r - l, 0.0f);
					XMVECTOR T = XMVector3Normalize(XMLoadFloat3(&mTangentX[i * mNumCols + j]));
					XMStoreFloat3(&mTangentX[i * mNumCols + j], T);
				}
			});
	}

	for (int i = 0; i < mVertexCount; i++)
	{
		WaveVertex v;
		v.pos = Position(i);
		v.normal = Normal(i);
		v.uv.x = 0.5f + v.pos.x / Width();
		v.uv.y = 0.5f - v.pos.z / Depth();


	

		memcpy(&(mappedVertex[frameIndex][i * sizeof(WaveVertex)]),
			&v, sizeof(WaveVertex));
		//memcpy(&(mappedVertex[i * sizeof(WaveVertex)]),
		//	&v, sizeof(WaveVertex));
	}

	WaveConstant waveConstant = {};
	waveConstant.worldTM = worldTM.Transpose();
	Matrix matrix;
	matrix = worldTM.Invert();
	matrix = matrix.Transpose();
	waveConstant.worldInvTransTM = matrix.Transpose();
	waveConstant.textureTM = Matrix().Transpose();

	constantBuffer.Reset(GraphicsMemory::Get().AllocateConstant(waveConstant));
}

void Waves::Disturb(int i, int j, float magnitude)
{
	// Don't disturb boundaries.
	assert(i > 1 && i < mNumRows - 2);
	assert(j > 1 && j < mNumCols - 2);

	float halfMag = 0.5f * magnitude;

	// Disturb the ijth vertex height and its neighbors.
	mCurrSolution[i * mNumCols + j].y += magnitude;
	mCurrSolution[i * mNumCols + j + 1].y += halfMag;
	mCurrSolution[i * mNumCols + j - 1].y += halfMag;
	mCurrSolution[(i + 1) * mNumCols + j].y += halfMag;
	mCurrSolution[(i - 1) * mNumCols + j].y += halfMag;
}

void Waves::Delete()
{
	managerController.DeleteMe();
}

void Waves::CreateDynamicVertexBufferNIndexBuffer(unsigned int vertexCnt)
{
	DX::DeviceResources* pDeviceResource =
		DX::DeviceResources::GetInstance();

	ID3D12Device* device = pDeviceResource->GetD3DDevice();

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto buffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(WaveVertex) * vertexCnt);
	//2 프레임의 버텍스 자원 생성
	device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ| D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		nullptr,
		IID_PPV_ARGS(&dynamicVtxPerFrame[0]));

	dynamicVtxPerFrame[0]->Map(0, nullptr,
		reinterpret_cast<void**>(&mappedVertex[0]));
	
	
	device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&dynamicVtxPerFrame[1]));

	dynamicVtxPerFrame[1]->Map(0, nullptr,
		reinterpret_cast<void**>(&mappedVertex[1]));
		

	//인덱스 버퍼 생성
	std::vector<uint32_t> indices(mIndexCount);

	int k = 0;
	int m = mNumRows;
	int n = mNumCols;

	for (int i = 0; i < m - 1; ++i)
	{
		for (int j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;

			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}

	
	pDeviceResource->WaitForGpu();

	ResourceUploadBatch batch(device);
	batch.Begin();

	CreateStaticBuffer(device, batch, indices,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
		| D3D12_RESOURCE_STATE_INDEX_BUFFER,
		indexBuffer.ReleaseAndGetAddressOf());

	auto uploadResourcesFinished = batch.End(pDeviceResource->GetCommandQueue());
	uploadResourcesFinished.wait();
}

D3D12_VERTEX_BUFFER_VIEW Waves::GetVertexBufferView() const
{
	DX::DeviceResources* pDeviceResource = DX::DeviceResources::GetInstance();
	unsigned int frameIndex = pDeviceResource->GetCurrentFrameIndex();

	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = dynamicVtxPerFrame[frameIndex]->GetGPUVirtualAddress();
	vbv.StrideInBytes = sizeof(WaveVertex);
	vbv.SizeInBytes = sizeof(WaveVertex) * mVertexCount;

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW Waves::GetIndexBufferView() const
{
	DX::DeviceResources* pDeviceResource = DX::DeviceResources::GetInstance();
	unsigned int frameIndex = pDeviceResource->GetCurrentFrameIndex();

	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R32_UINT;
	ibv.SizeInBytes = sizeof(std::uint32_t) * mTriangleCount * 3;


	return ibv;
}

