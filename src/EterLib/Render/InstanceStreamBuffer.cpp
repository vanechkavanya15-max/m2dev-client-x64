
#include "InstanceStreamBuffer.h"
#include <algorithm>
#include <cstring> // For memcpy if needed

namespace EterLib::Render
{
    InstanceStreamBuffer::InstanceStreamBuffer()
        : m_pVB(nullptr),
          m_maxInstances(0),
          m_currentInstances(0)
    {
    }

    InstanceStreamBuffer::~InstanceStreamBuffer()
    {
        if (m_pVB)
        {
            m_pVB->Release();
            m_pVB = nullptr;
        }
    }

    bool InstanceStreamBuffer::Initialize(LPDIRECT3DDEVICE9 dev, size_t maxInstances)
    {
        if (!dev || maxInstances == 0)
            return false;

        if (m_pVB)
        {
            m_pVB->Release();
            m_pVB = nullptr;
        }

        m_maxInstances = maxInstances;
        m_currentInstances = 0;

        HRESULT hr = dev->CreateVertexBuffer(
            static_cast<UINT>(m_maxInstances * sizeof(InstanceData)),
            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
            0, // FVF is 0 since we use vertex declarations for instancing
            D3DPOOL_DEFAULT,
            &m_pVB,
            nullptr
        );

        return SUCCEEDED(hr);
    }

    void InstanceStreamBuffer::UpdateInstances(std::span<const InstanceData> instances)
    {
        if (!m_pVB || instances.empty())
        {
            m_currentInstances = 0;
            return;
        }

        size_t instancesToCopy = std::min(instances.size(), m_maxInstances);
        m_currentInstances = instancesToCopy;

        void* pData = nullptr;
        HRESULT hr = m_pVB->Lock(0, static_cast<UINT>(instancesToCopy * sizeof(InstanceData)), &pData, D3DLOCK_DISCARD);
        if (SUCCEEDED(hr) && pData)
        {
            std::memcpy(pData, instances.data(), instancesToCopy * sizeof(InstanceData));
            m_pVB->Unlock();
        }
        else
        {
            m_currentInstances = 0;
        }
    }

    void InstanceStreamBuffer::Bind(LPDIRECT3DDEVICE9 dev, uint32_t streamIndex)
    {
        if (!dev || !m_pVB || m_currentInstances == 0)
            return;

        dev->SetStreamSource(streamIndex, m_pVB, 0, sizeof(InstanceData));
        dev->SetStreamSourceFreq(streamIndex, (D3DSTREAMSOURCE_INSTANCEDATA | 1));
    }
}

