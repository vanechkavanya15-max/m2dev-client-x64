#include "StdAfx.h"
#include "Eterbase/Debug.h"
#include "Thing.h"
#include "ThingInstance.h"
#include "EterModelLib/GltfModel.h"

CGraphicThing::CGraphicThing(const char* c_szFileName) : CResource(c_szFileName)
{
	Initialize();	
}

CGraphicThing::~CGraphicThing()
{
	//OnClear();
	Clear();
}

bool CGraphicThing::IsGltf() const
{
	return (m_pGltfModel != NULL);
}

EterModelLib::CGltfModel * CGraphicThing::GetGltfModelPointer()
{
	return m_pGltfModel;
}

bool CGraphicThing::CreateFromGltfModel(EterModelLib::CGltfModel * pGltfModel)
{
	Clear();
	if (!pGltfModel)
		return false;

	m_pGltfModel = pGltfModel;
	int motionCount = (int)m_pGltfModel->GetAnimationCount();
	if (motionCount > 0)
	{
		m_motions = new CGrannyMotion[motionCount];
		for (int m = 0; m < motionCount; ++m)
		{
			m_motions[m].BindGltfAnimation(m_pGltfModel->GetAnimation(m));
		}
	}
	me_state = STATE_EXIST;
	return true;
}

bool CGraphicThing::CreateFromGltfModelData(const GltfModelData & data)
{
	Clear();
	m_pGltfModel = new EterModelLib::CGltfModel();
	m_pGltfModel->GetModelData() = data;
	int motionCount = (int)m_pGltfModel->GetAnimationCount();
	if (motionCount > 0)
	{
		m_motions = new CGrannyMotion[motionCount];
		for (int m = 0; m < motionCount; ++m)
		{
			m_motions[m].BindGltfAnimation(m_pGltfModel->GetAnimation(m));
		}
	}
	me_state = STATE_EXIST;
	return true;
}

bool CGraphicThing::LoadFromMemory(int iSize, const void* c_pvBuf)
{
	Clear();
	if (OnLoad(iSize, c_pvBuf))
	{
		me_state = STATE_EXIST;
		return true;
	}
	me_state = STATE_ERROR;
	return false;
}

bool CGraphicThing::LoadFromFile(const char* c_szFileName)
{
	FILE* fp = fopen(c_szFileName, "rb");
	if (!fp)
		return false;

	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (size <= 0)
	{
		fclose(fp);
		return false;
	}

	std::vector<unsigned char> buf(size);
	size_t readBytes = fread(buf.data(), 1, size, fp);
	fclose(fp);

	if (readBytes != (size_t)size)
		return false;

	SetFileName(c_szFileName);
	return LoadFromMemory((int)size, buf.data());
}

bool CGraphicThing::RegisterMotion(const GltfMotionData& motion)
{
	if (!m_pGltfModel)
		return false;

	m_pGltfModel->GetModelData().motions.push_back(motion);
	int motionCount = (int)m_pGltfModel->GetAnimationCount();

	if (m_motions)
	{
		delete [] m_motions;
		m_motions = NULL;
	}

	m_motions = new CGrannyMotion[motionCount];
	for (int m = 0; m < motionCount; ++m)
	{
		m_motions[m].BindGltfAnimation(m_pGltfModel->GetAnimation(m));
	}
	return true;
}

void CGraphicThing::Initialize()
{
	m_pgrnFile = NULL;
	m_pgrnFileInfo = NULL;
	m_pgrnAni = NULL;

	m_models = NULL;
	m_motions = NULL;
	m_pGltfModel = NULL;
}

void CGraphicThing::OnClear()
{
	if (m_pGltfModel)
	{
		delete m_pGltfModel;
		m_pGltfModel = NULL;
	}

	if (m_motions)
		delete [] m_motions;

	if (m_models)
		delete [] m_models;

	if (m_pgrnFile)
		GrannyFreeFile(m_pgrnFile);

	Initialize();
}

CGraphicThing::TType CGraphicThing::Type()
{
	static TType s_type = StringToType("CGraphicThing");
	return s_type;
}

bool CGraphicThing::OnIsEmpty() const
{
	return (m_pgrnFile == NULL && m_pGltfModel == NULL);
}

bool CGraphicThing::OnIsType(TType type)
{
	if (CGraphicThing::Type() == type)
		return true;

	return CResource::OnIsType(type);
}

bool CGraphicThing::CreateDeviceObjects()
{
	if (!m_pgrnFileInfo)
		return true;
	
	for (int m = 0; m < m_pgrnFileInfo->ModelCount; ++m)
	{
		CGrannyModel & rModel = m_models[m];
		rModel.CreateDeviceObjects();
	}

	return true;
}

void CGraphicThing::DestroyDeviceObjects()
{
	if (!m_pgrnFileInfo)
		return;

	for (int m = 0; m < m_pgrnFileInfo->ModelCount; ++m)
	{
		CGrannyModel & rModel = m_models[m];
		rModel.DestroyDeviceObjects();
	}
}

bool CGraphicThing::CheckModelIndex(int iModel) const
{
	if (m_pGltfModel)
	{
		return (iModel == 0 && (m_pGltfModel->GetSubmeshCount() > 0 || m_pGltfModel->GetBoneCount() > 0));
	}

	if (!m_pgrnFileInfo)
	{
		Tracef("m_pgrnFileInfo == NULL: %s\n", GetFileName());
		return false;
	}

	assert(m_pgrnFileInfo != NULL);

	if (iModel < 0)
		return false;

	if (iModel >= m_pgrnFileInfo->ModelCount)
		return false;

	return true;
}

bool CGraphicThing::CheckMotionIndex(int iMotion) const
{
	if (m_pGltfModel)
	{
		if (iMotion < 0 || iMotion >= (int)m_pGltfModel->GetAnimationCount())
			return false;
		return true;
	}

	if (!m_pgrnFileInfo)
		return false;

	assert(m_pgrnFileInfo != NULL);

	if (iMotion < 0)
		return false;
	
	if (iMotion >= m_pgrnFileInfo->AnimationCount)
		return false;

	return true;
}

CGrannyModel * CGraphicThing::GetModelPointer(int iModel)
{	
	if (m_pGltfModel)
		return NULL;

	assert(CheckModelIndex(iModel));
	assert(m_models != NULL);
	return m_models + iModel;
}

CGrannyMotion * CGraphicThing::GetMotionPointer(int iMotion)
{
	assert(CheckMotionIndex(iMotion));

	if (m_pGltfModel)
	{
		if (iMotion >= (int)m_pGltfModel->GetAnimationCount())
			return NULL;

		assert(m_motions != NULL);
		return (m_motions + iMotion);
	}

	if (iMotion >= m_pgrnFileInfo->AnimationCount)
		return NULL;

	assert(m_motions != NULL);
	return (m_motions + iMotion);
}

int CGraphicThing::GetModelCount() const
{
	if (m_pGltfModel)
		return (m_pGltfModel->GetSubmeshCount() > 0 || m_pGltfModel->GetBoneCount() > 0) ? 1 : 0;

	if (!m_pgrnFileInfo)
		return 0;

	return (m_pgrnFileInfo->ModelCount);
}

int CGraphicThing::GetMotionCount() const
{
	if (m_pGltfModel)
		return (int)m_pGltfModel->GetAnimationCount();

	if (!m_pgrnFileInfo)
		return 0;

	return (m_pgrnFileInfo->AnimationCount);
}

bool CGraphicThing::OnLoad(int iSize, const void * c_pvBuf)
{
	if (!c_pvBuf || iSize <= 0)
		return false;

	// Detekcja plikow glTF 2.0 (binarny .glb z magic 'glTF' / 0x46546C67 lub JSON '{"asset"')
	const unsigned char* pBytes = (const unsigned char*)c_pvBuf;
	bool isGltf = false;
	if (iSize >= 4 && pBytes[0] == 'g' && pBytes[1] == 'l' && pBytes[2] == 'T' && pBytes[3] == 'F')
		isGltf = true;
	else if (iSize > 12 && (pBytes[0] == '{' || strstr((const char*)c_pvBuf, "\"asset\"") != NULL))
		isGltf = true;

	if (isGltf)
	{
		m_pGltfModel = new EterModelLib::CGltfModel();
		if (!m_pGltfModel->LoadFromMemory(c_pvBuf, (size_t)iSize))
		{
			delete m_pGltfModel;
			m_pGltfModel = NULL;
			return false;
		}

		int motionCount = (int)m_pGltfModel->GetAnimationCount();
		if (motionCount > 0)
		{
			m_motions = new CGrannyMotion[motionCount];
			for (int m = 0; m < motionCount; ++m)
			{
				m_motions[m].BindGltfAnimation(m_pGltfModel->GetAnimation(m));
			}
		}
		return true;
	}

	m_pgrnFile = GrannyReadEntireFileFromMemory(iSize, (void *) c_pvBuf);

	if (!m_pgrnFile)
		return false;

    m_pgrnFileInfo = GrannyGetFileInfo(m_pgrnFile);

	if (!m_pgrnFileInfo)
		return false;

	LoadModels();
	LoadMotions();
	return true;
}

// SUPPORT_LOCAL_TEXTURE
static std::string gs_modelLocalPath;

const std::string& GetModelLocalPath()
{
	return gs_modelLocalPath;
}
// END_OF_SUPPORT_LOCAL_TEXTURE

bool CGraphicThing::LoadModels()
{
	assert(m_pgrnFile != NULL);
	assert(m_models == NULL);
	
	if (m_pgrnFileInfo->ModelCount <= 0)
		return false;	

	// SUPPORT_LOCAL_TEXTURE
	const std::string& fileName = GetFileNameString();

	//char localPath[256] = "";
	if (fileName.length() > 2 && fileName[1] != ':')
	{				
		int sepPos = fileName.rfind('\\');
		gs_modelLocalPath.assign(fileName, 0, sepPos+1);
	}
	// END_OF_SUPPORT_LOCAL_TEXTURE

	int modelCount = m_pgrnFileInfo->ModelCount;

	m_models = new CGrannyModel[modelCount];

	for (int m = 0; m < modelCount; ++m)
	{
		CGrannyModel & rModel = m_models[m];
		granny_model * pgrnModel = m_pgrnFileInfo->Models[m];

		if (!rModel.CreateFromGrannyModelPointer(pgrnModel))
			return false;
	}

	GrannyFreeFileSection(m_pgrnFile, GrannyStandardRigidVertexSection);
	GrannyFreeFileSection(m_pgrnFile, GrannyStandardRigidIndexSection);
	GrannyFreeFileSection(m_pgrnFile, GrannyStandardDeformableIndexSection);
	GrannyFreeFileSection(m_pgrnFile, GrannyStandardTextureSection);
	return true;
}

bool CGraphicThing::LoadMotions()
{
	assert(m_pgrnFile != NULL);
	assert(m_motions == NULL);

	if (m_pgrnFileInfo->AnimationCount <= 0)
		return false;
	
	int motionCount = m_pgrnFileInfo->AnimationCount;

	m_motions = new CGrannyMotion[motionCount];
	
	for (int m = 0; m < motionCount; ++m)
		if (!m_motions[m].BindGrannyAnimation(m_pgrnFileInfo->Animations[m]))
			return false;

	return true;
}
