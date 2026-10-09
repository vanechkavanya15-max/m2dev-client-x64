#include "StdAfx.h"
#include "LODController.h"
#include "EterModelLib/GltfModel.h"
#include "EterModelLib/GltfModelInstance.h"
#include "Eterlib/ResourceManager.h"
#include "Eterlib/GrpImage.h"
#include "Eterlib/StateManager.h"

static float LODHEIGHT_ACTOR		=	500.0f;
static float LODDISTANCE_ACTOR		=	5000.0f;
static float LODDISTANCE_BUILDING	=	25000.0f;

static const float c_fNearLodScale = 3.0f;
static const float c_fFarLodScale = 25.0f;
static const float LOD_APPLY_MAX = 2000.0f;
static const float LOD_APPLY_MIN = 500.0f;

bool ms_isMinLODModeEnable=false;

enum
{
	SHARED_VB_500	= 0,
	SHARED_VB_1000	= 1,
	SHARED_VB_1500	= 2,
	SHARED_VB_2000	= 3,
	SHARED_VB_2500	= 4,
	SHARED_VB_3000	= 5,
	SHARED_VB_3500	= 6,
	SHARED_VB_4000	= 7,
	SHARED_VB_NUM	= 9,
};

static std::vector<CGraphicVertexBuffer*> gs_vbs[SHARED_VB_NUM];

static CGraphicVertexBuffer gs_emptyVB;
static LPDIRECT3DVERTEXDECLARATION9 s_pGltfVertexDecl = NULL;

#include <time.h>

static CGraphicVertexBuffer* __AllocDeformVertexBuffer(unsigned deformableVertexCount)
{
	if (deformableVertexCount == 0)
		return &gs_emptyVB;

	unsigned capacity	= (((deformableVertexCount-1) / 500) + 1) * 500;
	unsigned index		= (deformableVertexCount-1) / 500;	
	if (index < SHARED_VB_NUM)
	{	
		std::vector<CGraphicVertexBuffer*>& vbs = gs_vbs[index];
		if (!vbs.empty())
		{
			//TraceError("REUSE %d(%d)", capacity, deformableVertexCount);

			CGraphicVertexBuffer* pkRetVB = vbs.back();
			vbs.pop_back();
			return pkRetVB;
		}
	}


	static time_t base = time(NULL);
	//TraceError("NEW %8d: %d(%d)", time(NULL) - base, capacity, deformableVertexCount);

	CGraphicVertexBuffer* pkNewVB = new CGraphicVertexBuffer;

	if (!pkNewVB->Create(
		capacity,
		D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1,
		D3DUSAGE_DYNAMIC,
		D3DPOOL_DEFAULT))
	{
		TraceError("NEW_ERROR %8d: %d(%d)", time(NULL) - base, capacity, deformableVertexCount);
	}

	return pkNewVB;
}

void __FreeDeformVertexBuffer(CGraphicVertexBuffer* pkDelVB)
{
	if (pkDelVB)
	{
		if (pkDelVB == &gs_emptyVB)
			return;

		unsigned index = (pkDelVB->GetVertexCount() - 1) / 500;
		if (index < SHARED_VB_NUM)
		{	
			gs_vbs[index].push_back(pkDelVB);			
		}
		else
		{
			pkDelVB->Destroy();
			delete pkDelVB;
		}
	}
}

void __ReserveSharedVertexBuffers(unsigned index, unsigned count)
{
	NANOBEGIN
	if (index >= SHARED_VB_NUM)
		return;

	unsigned capacity = (index + 1) * 500;

	for (unsigned i = 0; i != count; ++i)
	{
		CGraphicVertexBuffer* pkNewVB = new CGraphicVertexBuffer;
		pkNewVB->Create(
			capacity,
			D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1,
			D3DUSAGE_DYNAMIC,
			D3DPOOL_DEFAULT);	
		gs_vbs[index].push_back(pkNewVB);
	}
	NANOEND
}

void GrannyCreateSharedDeformBuffer()
{
	__ReserveSharedVertexBuffers(SHARED_VB_500, 40);
	__ReserveSharedVertexBuffers(SHARED_VB_1000, 20);
	__ReserveSharedVertexBuffers(SHARED_VB_1500, 20);
	__ReserveSharedVertexBuffers(SHARED_VB_2000, 40);
	__ReserveSharedVertexBuffers(SHARED_VB_3000, 20);
}

void GrannyDestroySharedDeformBuffer()
{
#ifdef _DEBUG
	TraceError("granny_shared_vbs:");
#endif
	for (int i = 0; i != SHARED_VB_NUM; ++i)
	{	
		std::vector<CGraphicVertexBuffer*>& vbs = gs_vbs[i];
#ifdef _DEBUG
		TraceError("\t%d: %d", i, vbs.size());
#endif

		std::vector<CGraphicVertexBuffer*>::iterator v;
		for (v = vbs.begin(); v != vbs.end(); ++v)
		{
			CGraphicVertexBuffer* pkEachVB = (*v);
			pkEachVB->Destroy();
			delete pkEachVB;
		}
		vbs.clear();
	}
	
}

void CGrannyLODController::SetMinLODMode(bool isEnable)
{
	ms_isMinLODModeEnable=isEnable;
}

void CGrannyLODController::SetMaterialImagePointer(const char* c_szImageName, CGraphicImage* pImage)
{
	std::deque<CGrannyModelInstance *>::iterator i;
	for (i=m_que_pkModelInst.begin(); i!=m_que_pkModelInst.end(); ++i)
	{
		CGrannyModelInstance* pkModelInst=(*i);
		pkModelInst->SetMaterialImagePointer(c_szImageName, pImage);
	}
}

void CGrannyLODController::SetMaterialData(const char* c_szImageName, const SMaterialData& c_rkMaterialData)
{
	std::deque<CGrannyModelInstance *>::iterator i;
	for (i=m_que_pkModelInst.begin(); i!=m_que_pkModelInst.end(); ++i)
	{
		CGrannyModelInstance* pkModelInst=(*i);
		pkModelInst->SetMaterialData(c_szImageName, c_rkMaterialData);
	}
}

void CGrannyLODController::SetSpecularInfo(const char* c_szMtrlName, BOOL bEnable, float fPower)
{
	std::deque<CGrannyModelInstance *>::iterator i;
	for (i=m_que_pkModelInst.begin(); i!=m_que_pkModelInst.end(); ++i)
	{
		CGrannyModelInstance* pkModelInst=(*i);
		pkModelInst->SetSpecularInfo(c_szMtrlName, bEnable, fPower);
	}
}

CGrannyLODController::CGrannyLODController() : 
	m_pCurrentModelInstance(NULL),
	m_bLODLevel(0),
	m_pAttachedParentModel(NULL),
	m_fLODDistance(0.0f),
	m_dwLODAniFPS(CGrannyModelInstance::ANIFPS_MAX),
	m_pkSharedDeformableVertexBuffer(NULL),
	m_pGltfModelInstance(NULL)
	/////////////////////////////////////////////////////
{
	D3DXMatrixIdentity(&m_matWorld);
}

CGrannyLODController::~CGrannyLODController()
{
	__FreeDeformVertexBuffer(m_pkSharedDeformableVertexBuffer);

	Clear();

	if (m_pGltfModelInstance)
	{
		delete m_pGltfModelInstance;
		m_pGltfModelInstance = NULL;
	}
}

void CGrannyLODController::Clear()
{
	if (m_pGltfModelInstance)
	{
		delete m_pGltfModelInstance;
		m_pGltfModelInstance = NULL;
	}

	if (m_pAttachedParentModel)
	{
		m_pAttachedParentModel->DetachModelInstance(this);
	}

	m_pCurrentModelInstance = NULL;
	m_pAttachedParentModel = NULL;

	std::for_each(m_que_pkModelInst.begin(), m_que_pkModelInst.end(), CGrannyModelInstance::Delete);
	m_que_pkModelInst.clear();

	std::vector<TAttachingModelData>::iterator itor = m_AttachedModelDataVector.begin();
	for (; m_AttachedModelDataVector.end() != itor; ++itor)
	{
		TAttachingModelData & rData = *itor;
		rData.pkLODController->m_pAttachedParentModel = NULL;
	}

	m_AttachedModelDataVector.clear();
}

void CGrannyLODController::AddModel(CGraphicThing * pThing, int iSrcModel, CGrannyLODController * pSkelLODController)
{
	if (!pThing)
		return;

	if (pThing->IsGltf())
	{
		if (m_pGltfModelInstance)
		{
			delete m_pGltfModelInstance;
			m_pGltfModelInstance = NULL;
		}

		m_pGltfModelInstance = new EterModelLib::CGltfModelInstance(pThing->GetGltfModelPointer());

		if (pSkelLODController != NULL && pSkelLODController->isGltfModelInstance())
		{
			m_pGltfModelInstance->SetLinkedModelInstance(pSkelLODController->GetGltfModelInstance());
		}

		SetLODLimits(0.0f, LODDISTANCE_ACTOR);
		return;
	}

	if (pSkelLODController && pSkelLODController->m_que_pkModelInst.empty())
	{
		assert(!"EMPTY SKELETON(CANNON LINK)");
		return;
	}

	assert(pThing->GetReferenceCount()>=1);

	pThing->AddReference();	
	
	if (pThing->GetModelCount() <= iSrcModel)
	{
		pThing->Release();
		return;
	}
	CGrannyModel * pModel = pThing->GetModelPointer(iSrcModel);
	if (!pModel)
	{
		pThing->Release();
		return;
	}
	
	CGrannyModelInstance * pModelInstance = CGrannyModelInstance::New();
	
	__ReserveSharedDeformableVertexBuffer(pModel->GetDeformVertexCount());

	if (pSkelLODController)
	{
		pModelInstance->SetLinkedModelPointer(pModel, m_pkSharedDeformableVertexBuffer, &pSkelLODController->m_pCurrentModelInstance);
	}
	else
	{		
		pModelInstance->SetLinkedModelPointer(pModel, m_pkSharedDeformableVertexBuffer, NULL);
	}

	// END_OF_WORK
	
	if (!m_pCurrentModelInstance)
	{
		m_pCurrentModelInstance = pModelInstance;
		pModelInstance->DeformNoSkin(&ms_matIdentity);

		D3DXVECTOR3 vtMin, vtMax;
		pModelInstance->GetBoundBox(&vtMin, &vtMax);

		float fSize = 0.0f;
		fSize = fMAX(fSize, fabs(vtMin.x - vtMax.x));
		fSize = fMAX(fSize, fabs(vtMin.y - vtMax.y));
		fSize = fMAX(fSize, fabs(vtMin.z - vtMax.z));		

		if (fSize<LODHEIGHT_ACTOR)			
			SetLODLimits(0.0f, LODDISTANCE_ACTOR);
		else
			// 
			SetLODLimits(0.0f, LODDISTANCE_BUILDING);
	}
	else
	{
		// FIXME : CModelInstance::m_pgrnWorldPose를 Update에서 사용하는데,
		//         Deform을 하지 않으면 NULL 입니다. 구조가 조금 바뀌어야 할지도.. - [levites]
 		pModelInstance->DeformNoSkin(&ms_matIdentity);
	}	

	pThing->Release();

	m_que_pkModelInst.push_front(pModelInstance);	
}



void CGrannyLODController::__ReserveSharedDeformableVertexBuffer(DWORD deformableVertexCount)
{
	if (m_pkSharedDeformableVertexBuffer && 
		m_pkSharedDeformableVertexBuffer->GetVertexCount() >= deformableVertexCount)
		return;	

	__FreeDeformVertexBuffer(m_pkSharedDeformableVertexBuffer);

	m_pkSharedDeformableVertexBuffer = __AllocDeformVertexBuffer(deformableVertexCount);
}

void CGrannyLODController::AttachModelInstance(CGrannyLODController * pSrcLODController, const char * c_szBoneName)
{
	if (!pSrcLODController)
		return;

	// 1. Doczepianie instancji Granny
	CGrannyModelInstance * pSrcInstance = pSrcLODController->GetModelInstance();
	CGrannyModelInstance * pDestInstance = GetModelInstance();
	if (pSrcInstance && pDestInstance)
	{
		pSrcInstance->SetParentModelInstance(pDestInstance, c_szBoneName);
	}

	// 2. Obsluga glTF (np. bron przymocowana do dloni Bip001 R Hand)
	if (pSrcLODController->m_pGltfModelInstance)
	{
		if (m_pGltfModelInstance)
		{
			pSrcLODController->m_pGltfModelInstance->SetParentModelInstance(m_pGltfModelInstance, c_szBoneName);
		}
		else if (pDestInstance)
		{
			int iBoneIndex = -1;
			if (pDestInstance->GetBoneIndexByName(c_szBoneName, &iBoneIndex))
			{
				const float * pBoneMat = pDestInstance->GetBoneMatrixPointer(iBoneIndex);
				if (pBoneMat)
					pSrcLODController->m_pGltfModelInstance->SetParentBoneMatrix((const EterModelLib::Matrix4x4*)pBoneMat);
			}
		}
	}

	// Link Parent Data
	pSrcLODController->m_pAttachedParentModel = this;

	// Link Child Data
	std::vector<TAttachingModelData>::iterator itor = m_AttachedModelDataVector.begin();
	for (; m_AttachedModelDataVector.end() != itor;)
	{
		TAttachingModelData & rData = *itor;
		if (pSrcLODController == rData.pkLODController)
		{
			itor = m_AttachedModelDataVector.erase(itor);
		}
		else
		{
			++itor;
		}
	}

	TAttachingModelData AttachingModelData;
	AttachingModelData.pkLODController = pSrcLODController;
	AttachingModelData.strBoneName = c_szBoneName;
	m_AttachedModelDataVector.push_back(AttachingModelData);
}

void CGrannyLODController::DetachModelInstance(CGrannyLODController * pSrcLODController)
{
	if (!pSrcLODController)
		return;

	if (pSrcLODController->m_pGltfModelInstance)
	{
		pSrcLODController->m_pGltfModelInstance->SetParentModelInstance(NULL, -1);
		pSrcLODController->m_pGltfModelInstance->SetParentBoneMatrix(NULL);
	}

	CGrannyModelInstance * pSrcInstance = pSrcLODController->GetModelInstance();
	CGrannyModelInstance * pDestInstance = GetModelInstance();
	if (pSrcInstance && pDestInstance)
	{
		pSrcInstance->SetParentModelInstance(NULL, 0);
	}

	// Unlink Child Data
	std::vector<TAttachingModelData>::iterator itor = m_AttachedModelDataVector.begin();
	for (; m_AttachedModelDataVector.end() != itor;)
	{
		TAttachingModelData & rData = *itor;
		if (pSrcLODController == rData.pkLODController)
		{
			itor = m_AttachedModelDataVector.erase(itor);
		}
		else
		{
			++itor;
		}
	}

	// Unlink Parent Data
	pSrcLODController->m_pAttachedParentModel = NULL;
}

void CGrannyLODController::SetLODLimits(float /*fNearLOD*/, float fFarLOD)
{
	m_fLODDistance = fFarLOD;
}

void CGrannyLODController::SetLODLevel(BYTE bLodLevel)
{
	assert(m_que_pkModelInst.size() > 0);
	
	if (m_que_pkModelInst.size() > 0)
		m_bLODLevel	= (BYTE) MIN(m_que_pkModelInst.size() - 1, bLodLevel);
}

void CGrannyLODController::CreateDeviceObjects()
{
	std::for_each(m_que_pkModelInst.begin(),
				  m_que_pkModelInst.end(),
				  CGrannyModelInstance::FCreateDeviceObjects());
}

void CGrannyLODController::DestroyDeviceObjects()
{
	if (s_pGltfVertexDecl)
	{
		s_pGltfVertexDecl->Release();
		s_pGltfVertexDecl = NULL;
	}

	std::for_each(m_que_pkModelInst.begin(),
				  m_que_pkModelInst.end(),
				  CGrannyModelInstance::FDestroyDeviceObjects());
}

void CGrannyLODController::RenderWithOneTexture()
{
	if (m_pGltfModelInstance)
	{
		RenderGltfModel();
		return;
	}

	assert(m_pCurrentModelInstance != NULL);

//#define CHECK_LOD
#ifdef CHECK_LOD
	if (m_que_pkModelInst.size() > 0 && m_pCurrentModelInstance == m_que_pkModelInst[0])
		m_pCurrentModelInstance->RenderWithoutTexture();

	if (m_que_pkModelInst.size() > 1 && m_pCurrentModelInstance == m_que_pkModelInst[1])
		m_pCurrentModelInstance->RenderWithOneTexture();

	if (m_que_pkModelInst.size() > 2 && m_pCurrentModelInstance == m_que_pkModelInst[2])
		m_pCurrentModelInstance->RenderWithOneTexture();

	if (m_que_pkModelInst.size() > 3 && m_pCurrentModelInstance == m_que_pkModelInst[3])
		m_pCurrentModelInstance->RenderWithOneTexture();

#else
	m_pCurrentModelInstance->RenderWithOneTexture();
#endif
}

void CGrannyLODController::BlendRenderWithOneTexture()
{
	if (m_pGltfModelInstance)
	{
		if (HaveBlendThing())
			RenderGltfModel();
		return;
	}

	assert(m_pCurrentModelInstance != NULL);
	m_pCurrentModelInstance->BlendRenderWithOneTexture();
}

void CGrannyLODController::RenderWithTwoTexture()
{
	if (m_pGltfModelInstance)
	{
		RenderGltfModel();
		return;
	}

	assert(m_pCurrentModelInstance != NULL);
	m_pCurrentModelInstance->RenderWithTwoTexture();
}

void CGrannyLODController::BlendRenderWithTwoTexture()
{
	if (m_pGltfModelInstance)
	{
		if (HaveBlendThing())
			RenderGltfModel();
		return;
	}

	assert(m_pCurrentModelInstance != NULL);
	m_pCurrentModelInstance->BlendRenderWithTwoTexture();
}

bool CGrannyLODController::HaveBlendThing()
{
	if (m_pGltfModelInstance)
		return m_pGltfModelInstance->HaveBlendThing();
	return (0 != GetModelInstance()) ? GetModelInstance()->HaveBlendThing() : false;
}

void CGrannyLODController::Update(float fElapsedTime, float fDistanceFromCenter, float fDistanceFromCamera)
{
	UpdateLODLevel(fDistanceFromCenter, fDistanceFromCamera);
	UpdateTime(fElapsedTime);
}

void CGrannyLODController::UpdateLODLevel(float fDistanceFromCenter, float fDistanceFromCamera)
{
	if (m_pGltfModelInstance)
		return;

	if (m_que_pkModelInst.size()<=1)
		return;
	
	assert(m_pCurrentModelInstance != NULL);

	
	if (fDistanceFromCenter > LOD_APPLY_MIN) // 중심 LOD 예외 취소
	{	
		// 카메라부터 멀어질 수록 fLODRate가 작아진다
		// 3개 LOD가 있을때.. 가장 먼게 0, 가까울 수록 숫자가 커진다

		// 100fps 50fps 33fps 25fps 20fps
		// 10ms 20ms 30ms 40ms 50ms
		float fLODFactor = fMINMAX(0.0f, (m_fLODDistance-fDistanceFromCamera), m_fLODDistance);
		
		if (m_fLODDistance>0.0f)
			m_dwLODAniFPS = (DWORD) ((CGrannyModelInstance::ANIFPS_MAX - CGrannyModelInstance::ANIFPS_MIN) * fLODFactor / m_fLODDistance + static_cast<float>(CGrannyModelInstance::ANIFPS_MIN));
		else
			m_dwLODAniFPS = CGrannyModelInstance::ANIFPS_MIN;
		
		assert(m_dwLODAniFPS > 0);
		m_dwLODAniFPS /= 10;
		m_dwLODAniFPS *= 10;

		float fLODStep = m_fLODDistance / m_que_pkModelInst.size();
		BYTE bLODLevel = BYTE(fLODFactor / fLODStep);

		if (m_fLODDistance <= 5000.0f)
		{
			if (fDistanceFromCamera < 500.0f)
			{
				bLODLevel = 0;
			}
			else if (fDistanceFromCamera < 1500.0f)
			{
				bLODLevel = 1;
			}
			else if (fDistanceFromCamera < 2500.0f)
			{
				bLODLevel = 2;
			}
			else
			{
				bLODLevel = 3;
			}

			bLODLevel = (BYTE) (m_que_pkModelInst.size() - std::min((size_t)bLODLevel, m_que_pkModelInst.size()) - 1);
		}
		
		if (ms_isMinLODModeEnable)
			bLODLevel=0;

		SetLODLevel(bLODLevel);

		if (m_pCurrentModelInstance != m_que_pkModelInst[m_bLODLevel])
		{
			SetCurrentModelInstance(m_que_pkModelInst[m_bLODLevel]);
		}
	}
	else
	{
		m_dwLODAniFPS=CGrannyModelInstance::ANIFPS_MAX;
		
		if (!m_que_pkModelInst.empty())
		{
			if (m_pCurrentModelInstance != m_que_pkModelInst.back())
			{
				SetCurrentModelInstance(m_que_pkModelInst.back());
			}
		}
	}
}

void CGrannyLODController::UpdateTime(float fElapsedTime)
{
	if (m_pGltfModelInstance)
	{
		m_pGltfModelInstance->Update(fElapsedTime);
		m_pGltfModelInstance->DeformVertices();
		RefreshAttachedModelInstance();
	}

	if (!m_pCurrentModelInstance)
		return;

	m_pCurrentModelInstance->Update(m_dwLODAniFPS);

	//DWORD t3=timeGetTime();
	m_pCurrentModelInstance->UpdateLocalTime(fElapsedTime);

	//DWORD t4=timeGetTime();

#ifdef __PERFORMANCE_CHECKER__
	{
		static FILE* fp=fopen("perf_lod_update.txt", "w");

		if (t4-t1>3)
		{
			fprintf(fp, "LOD.Total %d (Time %f)\n", t4-t1, timeGetTime()/1000.0f);
			fprintf(fp, "LOD.SMI %d\n", t2-t1);
			fprintf(fp, "LOD.UP %d\n", t3-t2);
			fprintf(fp, "LOD.UL %d\n", t4-t3);
			fprintf(fp, "-------------------------------- \n");
			fflush(fp);
		}			
		fflush(fp);
	}
#endif
}

void CGrannyLODController::SetCurrentModelInstance(CGrannyModelInstance * pgrnModelInstance)
{
	// Copy Motion
	pgrnModelInstance->CopyMotion(m_pCurrentModelInstance, true);
	m_pCurrentModelInstance = pgrnModelInstance;

	// Change children attaching link
	RefreshAttachedModelInstance();

	// Change parent attaching link
	if (m_pAttachedParentModel)
	{
		m_pAttachedParentModel->RefreshAttachedModelInstance();
	}
}

void CGrannyLODController::RefreshAttachedModelInstance()
{
	for (DWORD i = 0; i < m_AttachedModelDataVector.size(); ++i)
	{
		TAttachingModelData & rModelData = m_AttachedModelDataVector[i];
		CGrannyLODController * pkChildController = rModelData.pkLODController;
		if (!pkChildController)
			continue;

		// 1. Dziecko to model glTF (np. bron glTF)
		if (pkChildController->m_pGltfModelInstance)
		{
			if (m_pGltfModelInstance)
			{
				pkChildController->m_pGltfModelInstance->SetParentModelInstance(m_pGltfModelInstance, rModelData.strBoneName.c_str());
			}
			else if (m_pCurrentModelInstance)
			{
				int iBoneIndex = -1;
				if (m_pCurrentModelInstance->GetBoneIndexByName(rModelData.strBoneName.c_str(), &iBoneIndex))
				{
					const float * pBoneMat = m_pCurrentModelInstance->GetBoneMatrixPointer(iBoneIndex);
					if (pBoneMat)
						pkChildController->m_pGltfModelInstance->SetParentBoneMatrix((const EterModelLib::Matrix4x4*)pBoneMat);
				}
			}
		}

		// 2. Dziecko to model Granny
		CGrannyModelInstance * pSrcInstance = pkChildController->GetModelInstance();
		if (pSrcInstance)
		{
			if (m_pGltfModelInstance)
			{
				const auto * pBoneMat = m_pGltfModelInstance->GetBoneMatrixPointer(rModelData.strBoneName.c_str());
				if (pBoneMat)
					pSrcInstance->SetCustomParentBoneMatrix(&pBoneMat->m[0][0]);
			}
			else if (m_pCurrentModelInstance)
			{
				pSrcInstance->SetParentModelInstance(m_pCurrentModelInstance, rModelData.strBoneName.c_str());
			}
		}
	}
}

void CGrannyLODController::UpdateSkeleton(const D3DXMATRIX * c_pWorldMatrix, float fElapsedTime)
{
	if (c_pWorldMatrix)
		m_matWorld = *c_pWorldMatrix;

	if (m_pGltfModelInstance)
		m_pGltfModelInstance->Update(fElapsedTime);

	if (m_pCurrentModelInstance)
		m_pCurrentModelInstance->UpdateSkeleton(c_pWorldMatrix, fElapsedTime);

	RefreshAttachedModelInstance();
}

void CGrannyLODController::DeformAll(const D3DXMATRIX * c_pWorldMatrix)
{
	if (c_pWorldMatrix)
		m_matWorld = *c_pWorldMatrix;

	if (m_pGltfModelInstance)
		m_pGltfModelInstance->DeformVertices();

	std::deque<CGrannyModelInstance *>::iterator i;
	for (i=m_que_pkModelInst.begin(); i!=m_que_pkModelInst.end(); ++i)
	{
		CGrannyModelInstance* pkModelInst=(*i);
		pkModelInst->Deform(c_pWorldMatrix);		
	}	
}

void CGrannyLODController::DeformNoSkin(const D3DXMATRIX * c_pWorldMatrix)
{
	if (c_pWorldMatrix)
		m_matWorld = *c_pWorldMatrix;

	if (m_pGltfModelInstance)
		m_pGltfModelInstance->DeformVertices();

	if (m_pCurrentModelInstance)
		m_pCurrentModelInstance->DeformNoSkin(c_pWorldMatrix);
}

void CGrannyLODController::Deform(const D3DXMATRIX * c_pWorldMatrix)
{
	if (c_pWorldMatrix)
		m_matWorld = *c_pWorldMatrix;

	if (m_pGltfModelInstance)
		m_pGltfModelInstance->DeformVertices();

	if (m_pCurrentModelInstance)
		m_pCurrentModelInstance->Deform(c_pWorldMatrix);
}

void CGrannyLODController::RenderToShadowMap()
{
	if (m_pGltfModelInstance)
	{
		RenderGltfModel();
		return;
	}

	if (m_pCurrentModelInstance)
		m_pCurrentModelInstance->RenderWithoutTexture();
}

void CGrannyLODController::RenderShadow()
{
	if (m_pGltfModelInstance)
	{
		RenderGltfModel();
		return;
	}

	if (m_pCurrentModelInstance)
		m_pCurrentModelInstance->RenderWithOneTexture();
}

void CGrannyLODController::ReloadTexture()
{
	if (m_pCurrentModelInstance)
		m_pCurrentModelInstance->ReloadTexture();
}

void CGrannyLODController::GetBoundBox(D3DXVECTOR3 * vtMin, D3DXVECTOR3 * vtMax)
{
	if (m_pGltfModelInstance)
	{
		const std::vector<GltfVertex>& vertices = m_pGltfModelInstance->GetDeformedVertices();
		if (!vertices.empty())
		{
			vtMin->x = vtMax->x = vertices[0].position.x;
			vtMin->y = vtMax->y = vertices[0].position.y;
			vtMin->z = vtMax->z = vertices[0].position.z;
			for (size_t i = 1; i < vertices.size(); ++i)
			{
				const auto& pos = vertices[i].position;
				if (pos.x < vtMin->x) vtMin->x = pos.x;
				if (pos.x > vtMax->x) vtMax->x = pos.x;
				if (pos.y < vtMin->y) vtMin->y = pos.y;
				if (pos.y > vtMax->y) vtMax->y = pos.y;
				if (pos.z < vtMin->z) vtMin->z = pos.z;
				if (pos.z > vtMax->z) vtMax->z = pos.z;
			}
			return;
		}
	}

	if (m_pCurrentModelInstance)
		m_pCurrentModelInstance->GetBoundBox(vtMin, vtMax);
}

bool CGrannyLODController::Intersect(const D3DXMATRIX * c_pMatrix, float * u, float * v, float * t)
{
	if (m_pGltfModelInstance)
	{
		D3DXVECTOR3 vtMin, vtMax;
		GetBoundBox(&vtMin, &vtMax);
		return IntersectCube(c_pMatrix,
			vtMin.x, vtMin.y, vtMin.z,
			vtMax.x, vtMax.y, vtMax.z,
			ms_vtPickRayOrig,
			ms_vtPickRayDir,
			u, v, t);
	}

	if (!m_pCurrentModelInstance)
		return false;
	return m_pCurrentModelInstance->Intersect(c_pMatrix, u, v, t);
}

void CGrannyLODController::SetLocalTime(float fLocalTime)
{
	if (m_pGltfModelInstance)
		m_pGltfModelInstance->SetCurrentTime(fLocalTime);

	if (m_pCurrentModelInstance)
		m_pCurrentModelInstance->SetLocalTime(fLocalTime);
}

void CGrannyLODController::ResetLocalTime()
{
	if (m_pGltfModelInstance)
		m_pGltfModelInstance->SetCurrentTime(0.0f);

	if (!m_pCurrentModelInstance)
		return;

	m_pCurrentModelInstance->ResetLocalTime();
}

void CGrannyLODController::SetMotionPointer(const CGrannyMotion * c_pMotion, float fBlendTime, int iLoopCount, float speedRatio)
{
	if (m_pGltfModelInstance && c_pMotion)
	{
		const GltfMotionData* pGltfMotion = c_pMotion->GetGltfAnimationPointer();
		if (pGltfMotion != NULL)
		{
			m_pGltfModelInstance->SetPlaySpeed(speedRatio);
			m_pGltfModelInstance->PlayMotion(pGltfMotion, iLoopCount == 0);
		}
		else
		{
			m_pGltfModelInstance->SetPlaySpeed(speedRatio);
			m_pGltfModelInstance->PlayMotion(c_pMotion->GetName(), iLoopCount == 0);
		}
	}

	if (!m_pCurrentModelInstance)
		return;

	m_pCurrentModelInstance->SetMotionPointer(c_pMotion, fBlendTime, iLoopCount, speedRatio);
}

void CGrannyLODController::ChangeMotionPointer(const CGrannyMotion * c_pMotion, int iLoopCount, float speedRatio)
{
	if (m_pGltfModelInstance && c_pMotion)
	{
		const GltfMotionData* pGltfMotion = c_pMotion->GetGltfAnimationPointer();
		if (pGltfMotion != NULL)
		{
			m_pGltfModelInstance->SetPlaySpeed(speedRatio);
			m_pGltfModelInstance->PlayMotion(pGltfMotion, iLoopCount == 0);
		}
		else
		{
			m_pGltfModelInstance->SetPlaySpeed(speedRatio);
			m_pGltfModelInstance->PlayMotion(c_pMotion->GetName(), iLoopCount == 0);
		}
	}

	if (!m_pCurrentModelInstance)
		return;

	m_pCurrentModelInstance->ChangeMotionPointer(c_pMotion, iLoopCount, speedRatio);
}

void CGrannyLODController::SetMotionAtEnd()
{
	if (m_pGltfModelInstance)
		m_pGltfModelInstance->SetCurrentTime(m_pGltfModelInstance->GetDuration());

	if (m_pCurrentModelInstance)
		m_pCurrentModelInstance->SetMotionAtEnd();
}

BOOL CGrannyLODController::isModelInstance()
{
	if (m_pGltfModelInstance != NULL)
		return TRUE;

	if (!m_pCurrentModelInstance)
		return FALSE;

	return TRUE;
}

CGrannyModelInstance * CGrannyLODController::GetModelInstance()
{
	return m_pCurrentModelInstance;
}

bool CGrannyLODController::isGltfModelInstance() const
{
	return (m_pGltfModelInstance != NULL);
}

EterModelLib::CGltfModelInstance * CGrannyLODController::GetGltfModelInstance()
{
	return m_pGltfModelInstance;
}

const float * CGrannyLODController::GetBoneMatrixPointer(int iBone) const
{
	if (m_pGltfModelInstance)
	{
		const auto* pMat = m_pGltfModelInstance->GetBoneMatrixPointer(iBone);
		if (pMat)
		{
			return &pMat->m[0][0];
		}
		return NULL;
	}

	if (m_pCurrentModelInstance)
		return m_pCurrentModelInstance->GetBoneMatrixPointer(iBone);

	return NULL;
}

const float * CGrannyLODController::GetBoneMatrixPointer(const char* c_szBoneName) const
{
	if (m_pGltfModelInstance)
	{
		const auto* pMat = m_pGltfModelInstance->GetBoneMatrixPointer(c_szBoneName);
		if (pMat)
		{
			return &pMat->m[0][0];
		}
		return NULL;
	}

	if (m_pCurrentModelInstance)
	{
		int boneIdx = -1;
		if (m_pCurrentModelInstance->GetBoneIndexByName(c_szBoneName, &boneIdx))
			return m_pCurrentModelInstance->GetBoneMatrixPointer(boneIdx);
	}

	return NULL;
}

bool CGrannyLODController::GetBoneIndexByName(const char* c_szBoneName, int* pBoneIndex) const
{
	if (m_pGltfModelInstance && m_pGltfModelInstance->GetModel())
	{
		int boneIdx = m_pGltfModelInstance->GetModel()->FindBoneIndex(c_szBoneName);
		if (boneIdx >= 0)
		{
			if (pBoneIndex)
				*pBoneIndex = boneIdx;
			return true;
		}
		return false;
	}

	if (m_pCurrentModelInstance)
		return m_pCurrentModelInstance->GetBoneIndexByName(c_szBoneName, pBoneIndex);

	return false;
}

const std::vector<EterModelLib::GltfVertex>& CGrannyLODController::GetDeformedVertices() const
{
	static const std::vector<EterModelLib::GltfVertex> s_empty;
	if (m_pGltfModelInstance)
		return m_pGltfModelInstance->GetDeformedVertices();
	return s_empty;
}

DWORD CGrannyLODController::GetDeformableVertexCount() const
{
	if (m_pGltfModelInstance && m_pGltfModelInstance->GetModel())
		return (DWORD)m_pGltfModelInstance->GetModel()->GetVertexCount();
	if (m_pCurrentModelInstance)
		return m_pCurrentModelInstance->GetDeformableVertexCount();
	return 0;
}

DWORD CGrannyLODController::GetVertexCount() const
{
	if (m_pGltfModelInstance && m_pGltfModelInstance->GetModel())
		return (DWORD)m_pGltfModelInstance->GetModel()->GetVertexCount();
	if (m_pCurrentModelInstance)
		return m_pCurrentModelInstance->GetVertexCount();
	return 0;
}

void CGrannyLODController::RenderGltfModel()
{
	if (!m_pGltfModelInstance)
		return;

	const EterModelLib::CGltfModel* pModel = m_pGltfModelInstance->GetModel();
	if (!pModel || !pModel->IsLoaded())
		return;

	const std::vector<GltfVertex>& deformedVertices = m_pGltfModelInstance->GetDeformedVertices();
	if (deformedVertices.empty())
		return;

	const GltfModelData& modelData = pModel->GetModelData();
	if (modelData.indices.empty())
		return;

	if (!s_pGltfVertexDecl && ms_lpd3dDevice)
	{
		D3DVERTEXELEMENT9 elements[] =
		{
			{ 0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
			{ 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0 },
			{ 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
			{ 0, 32, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
			D3DDECL_END()
		};
		ms_lpd3dDevice->CreateVertexDeclaration(elements, &s_pGltfVertexDecl);
	}

	if (s_pGltfVertexDecl)
	{
		STATEMANAGER.SetVertexDeclaration(s_pGltfVertexDecl);
	}

	STATEMANAGER.SetTransform(D3DTS_WORLD, &m_matWorld);

	STATEMANAGER.SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	STATEMANAGER.SetRenderState(D3DRS_ALPHAREF, 0x80);
	STATEMANAGER.SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);

	// Okreslenie lokalnego katalogu dla tekstur z nazwy modelu
	std::string localPath;
	if (!pModel->GetName().empty())
	{
		const std::string& modelName = pModel->GetName();
		size_t sepPos = modelName.find_last_of("\\/");
		if (sepPos != std::string::npos)
			localPath = modelName.substr(0, sepPos + 1);
	}
	if (localPath.empty())
	{
		extern const std::string& GetModelLocalPath();
		localPath = GetModelLocalPath();
	}

	for (size_t s = 0; s < modelData.submeshes.size(); ++s)
	{
		const GltfSubmesh& submesh = modelData.submeshes[s];
		if (submesh.indexCount == 0)
			continue;

		UINT primitiveCount = submesh.indexCount / 3;
		if (primitiveCount == 0 || (submesh.indexOffset + submesh.indexCount) > modelData.indices.size())
			continue;

		bool isBlend = false;
		bool isTwoSided = false;
		LPDIRECT3DTEXTURE9 pD3DTexture = NULL;
		if (submesh.materialIndex >= 0 && submesh.materialIndex < (int)modelData.materials.size())
		{
			const GltfMaterial& mat = modelData.materials[submesh.materialIndex];
			if (mat.doubleSided)
				isTwoSided = true;

			if (!mat.opacityTexture.empty() || (!mat.name.empty() && !_strnicmp(mat.name.c_str(), "blend", 5)))
				isBlend = true;

			if (!mat.diffuseTexture.empty())
			{
				CResourceManager& resMgr = CResourceManager::Instance();
				CResource* pResource = resMgr.GetResourcePointer(mat.diffuseTexture.c_str());

				if (!pResource || pResource->IsEmpty())
				{
					if (!localPath.empty())
					{
						std::string localTex = localPath + mat.diffuseTexture;
						pResource = resMgr.GetResourcePointer(localTex.c_str());

						if ((!pResource || pResource->IsEmpty()) && (mat.diffuseTexture.find('/') != std::string::npos || mat.diffuseTexture.find('\\') != std::string::npos))
						{
							size_t lastSlash = mat.diffuseTexture.find_last_of("/\\");
							std::string fileNameOnly = mat.diffuseTexture.substr(lastSlash + 1);
							std::string localFile = localPath + fileNameOnly;
							pResource = resMgr.GetResourcePointer(localFile.c_str());
						}
					}
				}

				if (pResource && !pResource->IsEmpty())
				{
					CGraphicImage* pImage = static_cast<CGraphicImage*>(pResource);
					if (pImage)
					{
						pD3DTexture = pImage->GetD3DTexture();
					}
				}
			}
		}

		if (isTwoSided)
		{
			STATEMANAGER.SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		}
		else
		{
			STATEMANAGER.SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		}

		if (isBlend)
		{
			STATEMANAGER.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			STATEMANAGER.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			STATEMANAGER.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		}
		else
		{
			STATEMANAGER.SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		}

		STATEMANAGER.SetTexture(0, pD3DTexture);
		STATEMANAGER.SetTexture(1, NULL);

		UINT minVertexIndex = submesh.vertexOffset;
		UINT numVertices = submesh.vertexCount > 0 ? submesh.vertexCount : (UINT)deformedVertices.size();

		STATEMANAGER.DrawIndexedPrimitiveUP(
			D3DPT_TRIANGLELIST,
			minVertexIndex,
			numVertices,
			primitiveCount,
			&modelData.indices[submesh.indexOffset],
			D3DFMT_INDEX32,
			deformedVertices.data(),
			sizeof(GltfVertex)
		);
	}

	// Przywrocenie domyslnych stanow D3D9 po zakonczeniu renderowania glTF
	STATEMANAGER.SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	STATEMANAGER.SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	STATEMANAGER.SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);
}
