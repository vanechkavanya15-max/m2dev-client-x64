#pragma once

#pragma warning(disable:4786)

#include <deque>
#include "Thing.h"
#include "ModelInstance.h"
#include "Eterlib/GrpCollisionObject.h"
#include "EterModelLib/GltfTypes.h"

namespace EterModelLib
{
	class CGltfModelInstance;
}

class CGrannyLODController : public CGraphicCollisionObject
{
	friend class CGraphicThingInstance;

	public:
		static void SetMinLODMode(bool isEnable);		

	public:
		struct FSetLocalTime
		{
			float fLocalTime;
			void operator() (CGrannyLODController * pController)
			{
				pController->SetLocalTime(fLocalTime);
			}
		};

		struct FUpdateTime
		{
			float fElapsedTime;
			
			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->UpdateTime(fElapsedTime);
			}
		};

		struct FUpdateLODLevel
		{
			float fDistanceFromCenter;
			float fDistanceFromCamera;

			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->UpdateLODLevel(fDistanceFromCenter, fDistanceFromCamera);
			}
		};

		struct FRenderWithOneTexture
		{
			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->RenderWithOneTexture();
			}
		};

		struct FBlendRenderWithOneTexture
		{
			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->BlendRenderWithOneTexture();
			}
		};

		struct FRenderWithTwoTexture
		{
			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->RenderWithTwoTexture();
			}
		};

		struct FBlendRenderWithTwoTexture
		{
			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->BlendRenderWithTwoTexture();
			}
		};

		struct FRenderToShadowMap
		{
			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->RenderToShadowMap();
			}
		};

		struct FRenderShadow
		{
			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->RenderShadow();
			}
		};
		
		struct FDeform
		{
			const D3DXMATRIX * mc_pWorldMatrix;
			
			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->Deform(mc_pWorldMatrix);
			}
		};
		struct FDeformNoSkin
		{
			const D3DXMATRIX * mc_pWorldMatrix;
			
			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->DeformNoSkin(mc_pWorldMatrix);
			}
		};
		struct FDeformAll
		{
			const D3DXMATRIX * mc_pWorldMatrix;
			
			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->DeformAll(mc_pWorldMatrix);
			}
		};

		struct FCreateDeviceObjects
		{
			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->CreateDeviceObjects();
			}
		};

		struct FDestroyDeviceObjects
		{
			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->DestroyDeviceObjects();
			}
		};

		struct FBoundBox
		{
			D3DXVECTOR3* m_vtMin;
			D3DXVECTOR3* m_vtMax;

			FBoundBox(D3DXVECTOR3 * vtMin, D3DXVECTOR3 * vtMax)
			{
				m_vtMin = vtMin;
				m_vtMax = vtMax;
			}

			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->GetBoundBox(m_vtMin, m_vtMax);
			}
		};

		struct FResetLocalTime
		{
			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->ResetLocalTime();
			}
		};

		struct FReloadTexture
		{
			void operator () (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->ReloadTexture();
			}
		};

		struct FSetMotionPointer
		{
			const CGrannyMotion *	m_pMotion;
			float					m_speedRatio;
			float					m_blendTime;
			int						m_loopCount;

			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->SetMotionPointer(m_pMotion, m_blendTime, m_loopCount, m_speedRatio);
			}
		};

		struct FChangeMotionPointer
		{
			const CGrannyMotion *	m_pMotion;
			float					m_speedRatio;
			int						m_loopCount;

			void operator() (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->ChangeMotionPointer(m_pMotion, m_loopCount, m_speedRatio);
			}
		};

		struct FEndStopMotionPointer
		{
			const CGrannyMotion *	m_pMotion;

			void operator () (CGrannyLODController * pController)
			{
				if (pController->isModelInstance() || pController->isGltfModelInstance())
					pController->SetMotionAtEnd();
			}
		};

		CGrannyLODController();
		virtual ~CGrannyLODController();

		void	Clear();

		void	CreateDeviceObjects();
		void	DestroyDeviceObjects();

		void	AddModel(CGraphicThing * pThing, int iSrcModel, CGrannyLODController * pSkelLODController=NULL);		
		void	AttachModelInstance(CGrannyLODController * pSrcLODController, const char * c_szBoneName);
		void	DetachModelInstance(CGrannyLODController * pSrcLODController);
		void	SetLODLimits(float fNearLOD, float fFarLOD);
		void	SetLODLevel(BYTE bLODLevel);
		BYTE	GetLODLevel() { return m_bLODLevel; }
		void	SetMaterialImagePointer(const char* c_szImageName, CGraphicImage* pImage);
		void	SetMaterialData(const char* c_szImageName, const SMaterialData& c_rkMaterialData);
		void	SetSpecularInfo(const char* c_szMtrlName, BOOL bEnable, float fPower);

		void	RenderWithOneTexture();
		void	RenderWithTwoTexture();
		void	BlendRenderWithOneTexture();
		void	BlendRenderWithTwoTexture();

		void	Update(float fElapsedTime, float fDistanceFromCenter, float fDistanceFromCamera);
		void	UpdateLODLevel(float fDistanceFromCenter, float fDistanceFromCamera);
		void	UpdateTime(float fElapsedTime);
		
		void	UpdateSkeleton(const D3DXMATRIX * c_pWorldMatrix, float fElapsedTime);
		void	Deform(const D3DXMATRIX * c_pWorldMatrix);
		void	DeformNoSkin(const D3DXMATRIX * c_pWorldMatrix);
		void	DeformAll(const D3DXMATRIX * c_pWorldMatrix);
		
		void	RenderToShadowMap();
		void	RenderShadow();
		void	ReloadTexture();

		void	GetBoundBox(D3DXVECTOR3 * vtMin, D3DXVECTOR3 * vtMax);
		bool	Intersect(const D3DXMATRIX * c_pMatrix, float * u, float * v, float * t);

		void	SetLocalTime(float fLocalTime);
		void	ResetLocalTime();

		void	SetMotionPointer(const CGrannyMotion * c_pMotion, float fBlendTime, int iLoopCount, float speedRatio);
		void	ChangeMotionPointer(const CGrannyMotion * c_pMotion, int iLoopCount, float speedRatio);
		void	SetMotionAtEnd();

		BOOL	isModelInstance();
		CGrannyModelInstance*	GetModelInstance();
		bool	HaveBlendThing();

		bool	isGltfModelInstance() const;
		bool	IsGltf() const { return isGltfModelInstance(); }
		EterModelLib::CGltfModelInstance * GetGltfModelInstance();

		const float *			GetBoneMatrixPointer(int iBone) const;
		const float *			GetBoneMatrixPointer(const char* c_szBoneName) const;
		bool					GetBoneIndexByName(const char* c_szBoneName, int* pBoneIndex) const;

		const std::vector<EterModelLib::GltfVertex>& GetDeformedVertices() const;
		DWORD					GetDeformableVertexCount() const;
		DWORD					GetVertexCount() const;

		CGrannyLODController *	GetAttachedParentModel() const { return m_pAttachedParentModel; }

	protected:
		void	RenderGltfModel();
		void	SetCurrentModelInstance(CGrannyModelInstance * pgrnModelInstance);
		void	RefreshAttachedModelInstance();

		void	__ReserveSharedDeformableVertexBuffer(DWORD deformableVertexCount);

	protected:
		float								m_fLODDistance;
		DWORD								m_dwLODAniFPS;

		//// Attaching Link Data
		// Data of Parent Side
		typedef struct SAttachingModelData
		{
			CGrannyLODController *	pkLODController;
			std::string				strBoneName;
		} TAttachingModelData;
		
		std::vector<TAttachingModelData>	m_AttachedModelDataVector;
		// Data of Child Side
		CGrannyLODController *				m_pAttachedParentModel;

		BYTE								m_bLODLevel;
		CGrannyModelInstance *				m_pCurrentModelInstance;		

		// WORK
		std::deque<CGrannyModelInstance *>	m_que_pkModelInst;

		CGraphicVertexBuffer*	m_pkSharedDeformableVertexBuffer;
		// END_OF_WORK

		EterModelLib::CGltfModelInstance *	m_pGltfModelInstance;
		D3DXMATRIX							m_matWorld;
};
