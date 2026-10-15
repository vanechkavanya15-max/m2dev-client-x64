#include "StdAfx.h"
#include "InstanceVisualComponent.h"
#include "PythonTextTail.h"

namespace UserInterface::InstanceComponents {

InstanceVisualComponent::InstanceVisualComponent(CActorInstance& actorInstance)
    : m_actorInstance(actorInstance)
{
}

bool InstanceVisualComponent::SetArmor(uint32_t shapeIndex, float specular)
{
    m_armorShape = shapeIndex;
    m_actorInstance.SetShape(shapeIndex, specular);
    return true;
}

bool InstanceVisualComponent::SetWeapon(uint32_t weaponVnum)
{
    m_weaponVnum = weaponVnum;
    m_actorInstance.AttachWeapon(weaponVnum);
    return true;
}

bool InstanceVisualComponent::SetHair(uint32_t hairVnum)
{
    m_hairVnum = hairVnum;
    m_actorInstance.SetHair(hairVnum);
    return true;
}

void InstanceVisualComponent::SetShape(uint32_t shapeIndex, float specular)
{
    m_armorShape = shapeIndex;
    m_actorInstance.SetShape(shapeIndex, specular);
}

uint32_t InstanceVisualComponent::GetPart(uint32_t partIndex) const noexcept
{
    return const_cast<CActorInstance&>(m_actorInstance).GetPartItemID(partIndex);
}

uint32_t InstanceVisualComponent::AttachSpecialEffect(uint32_t effectIndex)
{
    return m_actorInstance.AttachEffectByID(0, nullptr, effectIndex);
}

void InstanceVisualComponent::DetachSpecialEffect(uint32_t effectId)
{
    m_actorInstance.DettachEffect(effectId);
}

void InstanceVisualComponent::ClearAllEffects()
{
    m_actorInstance.ClearAttachingEffect();
}

void InstanceVisualComponent::AttachTextTail(uint32_t guildId, uint32_t vid, float height)
{
    m_textTailVid = vid;
    m_hasTextTail = true;
    static D3DXCOLOR s_defaultColor(1.0f, 1.0f, 1.0f, 1.0f);
    CPythonTextTail::Instance().RegisterCharacterTextTail(guildId, vid, s_defaultColor, height);
}

void InstanceVisualComponent::DetachTextTail(uint32_t vid)
{
    if (m_hasTextTail)
    {
        CPythonTextTail::Instance().DeleteCharacterTextTail(vid);
        m_hasTextTail = false;
        m_textTailVid = 0;
    }
}

void InstanceVisualComponent::RefreshTextTail()
{
    if (m_hasTextTail)
    {
        CPythonTextTail::Instance().ArrangeTextTail();
    }
}

void InstanceVisualComponent::UpdateTextTailLevel(uint32_t vid, std::string_view levelText)
{
    if (m_hasTextTail)
    {
        static D3DXCOLOR s_levelColor(1.0f, 1.0f, 0.0f, 1.0f);
        std::string lvlStr(levelText);
        CPythonTextTail::Instance().AttachLevel(vid, lvlStr.c_str(), s_levelColor);
    }
}

void InstanceVisualComponent::SetAlpha(float alpha)
{
    m_alpha = alpha;
    m_actorInstance.SetAlphaValue(alpha);
}

void InstanceVisualComponent::SetVisible(bool isVisible)
{
    m_isVisible = isVisible;
    if (!isVisible && m_hasTextTail)
    {
        DetachTextTail(m_textTailVid);
    }
}

void InstanceVisualComponent::Clear()
{
    if (m_hasTextTail)
    {
        DetachTextTail(m_textTailVid);
    }
    ClearAllEffects();
    m_armorShape = 0;
    m_weaponVnum = 0;
    m_hairVnum = 0;
    m_alpha = 1.0f;
    m_isVisible = true;
    m_textTailVid = 0;
}

} // namespace UserInterface::InstanceComponents
