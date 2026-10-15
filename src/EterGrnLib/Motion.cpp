#include "StdAfx.h"
#include "Motion.h"
#include "EterModelLib/GltfTypes.h"

CGrannyMotion::CGrannyMotion()
{
	Initialize();
}

CGrannyMotion::~CGrannyMotion()
{
	Destroy();
}

bool CGrannyMotion::IsEmpty()
{
	return (m_pGltfMotion == NULL);
}

void CGrannyMotion::Destroy()
{
	Initialize();
}

void CGrannyMotion::Initialize()
{
	m_pgrnAni = NULL;
	m_pGltfMotion = NULL;
}

bool CGrannyMotion::BindGrannyAnimation(granny_animation * pgrnAni)
{
	return true;
}

bool CGrannyMotion::BindGltfAnimation(const GltfMotionData * pGltfMotion)
{
	assert(IsEmpty());

	m_pGltfMotion = pGltfMotion;
	return true;
}

granny_animation* CGrannyMotion::GetGrannyAnimationPointer() const
{
	return nullptr;
}

const GltfMotionData* CGrannyMotion::GetGltfAnimationPointer() const
{
	return m_pGltfMotion;
}

const char * CGrannyMotion::GetName() const
{
	if (m_pGltfMotion)
		return m_pGltfMotion->name.c_str();

	return "";
}

float CGrannyMotion::GetDuration() const
{
	if (m_pGltfMotion)
		return m_pGltfMotion->duration;

	return 0.0f;
}

void CGrannyMotion::GetTextTrack(const char * c_szTextTrackName, int * pCount, float * pArray) const
{
	if (m_pGltfMotion)
	{
		for (size_t i = 0; i < m_pGltfMotion->events.size(); ++i)
		{
			const auto& ev = m_pGltfMotion->events[i];
			if (!_stricmp(c_szTextTrackName, ev.type.c_str()))
			{
				pArray[(*pCount)++] = ev.time;
			}
		}
	}
}


