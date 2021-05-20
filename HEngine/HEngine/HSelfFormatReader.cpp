#include"HSelfFormatReader.h"
#include<fstream>

HSelfFormatReader::~HSelfFormatReader()
{
	DeleteCurruntReadHModel();
	DeleteCurruntReadHAnim();
}

HModelFormat* HSelfFormatReader::ReadHModelFile(const char* file)
{
	std::string fileName = file;

	if (fileName.substr(fileName.length() - 7, 7) != ".hmodel")
	{
		return nullptr;
	}

	m_curruntHModel = new HModelFormat;

	std::ifstream readFile(fileName.c_str(), std::ostream::binary);

	readFile.read(reinterpret_cast<char*>(&m_curruntHModel->header), sizeof(HModel_Header));
	readFile.read(reinterpret_cast<char*>(&m_curruntHModel->commonValue), sizeof(HModel_CommonValue));
	
	if (m_curruntHModel->header.meshCount != 0)
	{
		m_curruntHModel->pMesh = new HModel_Mesh[m_curruntHModel->header.meshCount];

		for (int i = 0; i < m_curruntHModel->header.meshCount; i++)
		{
			readFile.read(reinterpret_cast<char*>(&m_curruntHModel->pMesh[i].name), sizeof(char) * 256);
			readFile.read(reinterpret_cast<char*>(&m_curruntHModel->pMesh[i].bIsDepthZero), sizeof(bool));
			readFile.read(reinterpret_cast<char*>(&m_curruntHModel->pMesh[i].index), sizeof(unsigned int));
			readFile.read(reinterpret_cast<char*>(&m_curruntHModel->pMesh[i].childCount), sizeof(unsigned int));

			if (m_curruntHModel->pMesh[i].childCount != 0)
			{
				m_curruntHModel->pMesh[i].pChildIndex = new unsigned int[m_curruntHModel->pMesh[i].childCount];

				for (int j = 0; j < m_curruntHModel->pMesh[i].childCount; j++)
				{
					readFile.read(reinterpret_cast<char*>(&m_curruntHModel->pMesh[i].pChildIndex[j]), sizeof(unsigned int));
				}
			}
		}
	}

	if (m_curruntHModel->header.boneCount != 0)
	{
		m_curruntHModel->pBone = new HModel_Bone[m_curruntHModel->header.boneCount];

		for (int i = 0; i < m_curruntHModel->header.boneCount; i++)
		{
			readFile.read(reinterpret_cast<char*>(&m_curruntHModel->pBone[i].name), sizeof(char) * 256);
			readFile.read(reinterpret_cast<char*>(&m_curruntHModel->pBone[i].bIsDepthZero), sizeof(bool));
			readFile.read(reinterpret_cast<char*>(&m_curruntHModel->pBone[i].index), sizeof(unsigned int));
			readFile.read(reinterpret_cast<char*>(&m_curruntHModel->pBone[i].childCount), sizeof(unsigned int));

			if (m_curruntHModel->pBone[i].childCount != 0)
			{
				m_curruntHModel->pBone[i].pChildIndex = new unsigned int[m_curruntHModel->pBone[i].childCount];

				for (int j = 0; j < m_curruntHModel->pBone[i].childCount; j++)
				{
					readFile.read(reinterpret_cast<char*>(&m_curruntHModel->pBone[i].pChildIndex[j]), sizeof(unsigned int));
				}
			}

			readFile.read(reinterpret_cast<char*>(&m_curruntHModel->pBone[i].invGlobalTTM), sizeof(XMFLOAT4X4));
		}
	}

	if (m_curruntHModel->header.totalVertexCount != 0)
	{
		m_curruntHModel->pVertices = new HModel_Vertex[m_curruntHModel->header.totalVertexCount];
		readFile.read(reinterpret_cast<char*>(m_curruntHModel->pVertices),
			sizeof(HModel_Vertex) * m_curruntHModel->header.totalVertexCount);
	}

	if (m_curruntHModel->header.totalIndexCount != 0)
	{
		m_curruntHModel->pIndices = new std::uint32_t[m_curruntHModel->header.totalIndexCount];
		readFile.read(reinterpret_cast<char*>(m_curruntHModel->pIndices),
			sizeof(std::uint32_t) * m_curruntHModel->header.totalIndexCount);
	}
	
	readFile.close();

	return m_curruntHModel;
}

HAnimFormat* HSelfFormatReader::ReadHAnimFile(const char* file)
{
	std::string fileName = file;

	if (fileName.substr(fileName.length() - 6, 6) != ".hanim")
	{
		return nullptr;
	}

	m_curruntHAnim = new HAnimFormat;

	std::ifstream readFile(fileName.c_str(), std::ostream::binary);

	readFile.read(reinterpret_cast<char*>(&m_curruntHAnim->header), sizeof(HAnim_Header));
	readFile.read(reinterpret_cast<char*>(&m_curruntHAnim->axisSystemModify), sizeof(XMFLOAT4X4));
	
	if (m_curruntHAnim->header.boneCount != 0)
	{
		m_curruntHAnim->pBone = new HAnim_Bone[m_curruntHAnim->header.boneCount];

		for (int i = 0; i < m_curruntHAnim->header.boneCount; i++)
		{
			readFile.read(reinterpret_cast<char*>(m_curruntHAnim->pBone[i].name), sizeof(char) * 256);
			readFile.read(reinterpret_cast<char*>(&m_curruntHAnim->pBone[i].index), sizeof(unsigned int));
			readFile.read(reinterpret_cast<char*>(&m_curruntHAnim->pBone[i].keyCount), sizeof(unsigned int));

			if (m_curruntHAnim->pBone[i].keyCount != 0)
			{
				m_curruntHAnim->pBone[i].pKey = new HAnim_Bone_KeyFrame[m_curruntHAnim->pBone[i].keyCount];
				readFile.read(reinterpret_cast<char*>(m_curruntHAnim->pBone[i].pKey),
					sizeof(HAnim_Bone_KeyFrame) * m_curruntHAnim->pBone[i].keyCount);
			}
		}
	}

	readFile.close();

	return m_curruntHAnim;
}

void HSelfFormatReader::DeleteCurruntReadHModel()
{
	if (m_curruntHModel != nullptr)
	{
		if (m_curruntHModel->pMesh != nullptr)
		{
			if (m_curruntHModel->pMesh->pChildIndex != nullptr)
			{
				delete[] m_curruntHModel->pMesh->pChildIndex;
			}
			delete[] m_curruntHModel->pMesh;
		}
		if (m_curruntHModel->pBone != nullptr)
		{
			if (m_curruntHModel->pBone->pChildIndex != nullptr)
			{
				delete[] m_curruntHModel->pBone->pChildIndex;
			}
			delete[] m_curruntHModel->pBone;
		}
		if (m_curruntHModel->pVertices != nullptr)
			delete[] m_curruntHModel->pVertices;
		if (m_curruntHModel->pIndices != nullptr)
			delete[] m_curruntHModel->pIndices;
	}

	delete m_curruntHModel;

	m_curruntHModel = nullptr;
}

void HSelfFormatReader::DeleteCurruntReadHAnim()
{
	if (m_curruntHAnim != nullptr)
	{
		if (m_curruntHAnim->pBone != nullptr)
		{
			if (m_curruntHAnim->pBone->pKey != nullptr)
			{
				delete[] m_curruntHAnim->pBone->pKey;
			}
			delete[] m_curruntHAnim->pBone;
		}
	}

	delete m_curruntHAnim;

	m_curruntHAnim = nullptr;
}
