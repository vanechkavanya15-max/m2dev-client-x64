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
	return (m_pgrnAni == NULL && m_pGltfMotion == NULL);
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
	assert(IsEmpty());

	m_pgrnAni = pgrnAni;
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
	return m_pgrnAni;
}

const GltfMotionData* CGrannyMotion::GetGltfAnimationPointer() const
{
	return m_pGltfMotion;
}

const char * CGrannyMotion::GetName() const
{
	if (m_pGltfMotion)
		return m_pGltfMotion->name.c_str();

	if (m_pgrnAni)
		return m_pgrnAni->Name;

	return "";
}

float CGrannyMotion::GetDuration() const
{
	if (m_pGltfMotion)
		return m_pGltfMotion->duration;

	if (m_pgrnAni)
		return m_pgrnAni->Duration;

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
		return;
	}

	if (!m_pgrnAni)
		return;

	if (m_pgrnAni->TrackGroupCount != 1)
	{
//		assert(!"CGrannyMotion::GetTextTrack - TrackCount is not 1");
	}

	granny_track_group * pTrack = m_pgrnAni->TrackGroups[0];

	for (int i = 0; i < pTrack->TextTrackCount; ++i)
	{
		granny_text_track & rTextTrack = pTrack->TextTracks[i];

		for (int j = 0; j < rTextTrack.EntryCount; ++j)
			if (!_stricmp(c_szTextTrackName, rTextTrack.Entries[j].Text))
				pArray[(*pCount)++] = rTextTrack.Entries[j].TimeStamp;
	}
}


