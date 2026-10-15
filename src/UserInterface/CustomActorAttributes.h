#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <d3dx9.h>

/**
 * @brief Bezpieczny kontener dodatkowych danych serwera (Custom Server Attributes).
 * Przechowuje rozszerzone atrybuty bez naruszania bazowego ukladu pamieci silnika Metin2.
 */
struct SCustomActorAttributes
{
	uint32_t m_dwSashVnum{0};                           // Szarfa / Acce / Wing
	uint32_t m_dwAuraVnum{0};                           // Aura
	std::string m_stCustomTitle;                        // Customowy tytul / ranga
	D3DXCOLOR m_kTitleColor{1.0f, 1.0f, 1.0f, 1.0f};   // Kolor tytulu RGBA
	uint32_t m_dwPetVnum{0};                            // Zwierzak / chowaniec
	uint32_t m_dwPetLevel{0};                           // Poziom zwierzaka
	
	// Elastyczny generyczny slownik na dowolne dodatkowe cechy O(1):
	std::unordered_map<std::string, uint64_t> m_customAttributes;

	void Clear()
	{
		m_dwSashVnum = 0;
		m_dwAuraVnum = 0;
		m_stCustomTitle.clear();
		m_kTitleColor = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
		m_dwPetVnum = 0;
		m_dwPetLevel = 0;
		m_customAttributes.clear();
	}

	void SetAttribute(const std::string& key, uint64_t value)
	{
		m_customAttributes[key] = value;
	}

	uint64_t GetAttribute(const std::string& key, uint64_t defaultValue = 0) const
	{
		auto it = m_customAttributes.find(key);
		return (it != m_customAttributes.end()) ? it->second : defaultValue;
	}
};
