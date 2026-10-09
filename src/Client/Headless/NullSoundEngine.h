#pragma once

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>

#include "../../AudioLib/Type.h"
#include "../../EterBase/Singleton.h"

// Forward declarations if needed, though we can define a stub for MaSoundInstance if it's strictly needed
// or just return nullptr for pointers.
class MaSoundInstance;

namespace Client::Headless
{
	/**
	 * @brief NullSoundEngine (Mock)
	 *
	 * Komponent sluzacy jako atrapa silnika dzwiekowego dla srodowisk Headless (np. testow).
	 * Zgodnie ze standardem AI-First Architecture, gwarantuje ZERO-CONFLICT i pelne bezpieczenstwo
	 * typow / pamieci (C++20/23).
	 * Wszystkie metody ignoruja faktyczne odtwarzanie dzwiekow, zawsze zwracajac sukces lub 
	 * domyslne bezpieczne wartosci.
	 */
	class NullSoundEngine : public CSingleton<NullSoundEngine>
	{
	public:
		enum ESoundConfig
		{
			SOUND_INSTANCE_3D_MAX_NUM = 32,
		};

		NullSoundEngine() noexcept = default;
		~NullSoundEngine() noexcept = default;

		constexpr bool Initialize() noexcept
		{
			return true;
		}

		constexpr void SetSoundVolume([[maybe_unused]] float volume) noexcept
		{
		}

		constexpr bool PlaySound2D([[maybe_unused]] const std::string& name) noexcept
		{
			return true;
		}

		constexpr MaSoundInstance* PlaySound3D([[maybe_unused]] const std::string& name, 
		                                       [[maybe_unused]] float fx, 
		                                       [[maybe_unused]] float fy, 
		                                       [[maybe_unused]] float fz) noexcept
		{
			return nullptr;
		}

		constexpr MaSoundInstance* PlayAmbienceSound3D([[maybe_unused]] float fx, 
		                                               [[maybe_unused]] float fy, 
		                                               [[maybe_unused]] float fz, 
		                                               [[maybe_unused]] const std::string& name, 
		                                               [[maybe_unused]] int loopCount = 1) noexcept
		{
			return nullptr;
		}

		constexpr void StopAllSound3D() noexcept
		{
		}

		void UpdateSoundInstance([[maybe_unused]] float fx, 
		                         [[maybe_unused]] float fy, 
		                         [[maybe_unused]] float fz, 
		                         [[maybe_unused]] uint32_t dwcurFrame, 
		                         [[maybe_unused]] const NSound::TSoundInstanceVector* c_pSoundInstanceVector, 
		                         [[maybe_unused]] bool checkFrequency = false) noexcept
		{
		}

		constexpr bool FadeInMusic([[maybe_unused]] const std::string& path, 
		                           [[maybe_unused]] float targetVolume = 1.0f, 
		                           [[maybe_unused]] float fadeInDurationSecondsFromMin = 1.5f) noexcept
		{
			return true;
		}

		constexpr void FadeOutMusic([[maybe_unused]] const std::string& name, 
		                            [[maybe_unused]] float targetVolume = 0.0f, 
		                            [[maybe_unused]] float fadeOutDurationSecondsFromMax = 1.5f) noexcept
		{
		}

		constexpr void FadeOutAllMusic() noexcept
		{
		}

		constexpr void SetMusicVolume(float volume) noexcept
		{
			m_MusicVolume = volume;
		}

		constexpr float GetMusicVolume() const noexcept
		{
			return m_MusicVolume;
		}

		constexpr void SaveVolume([[maybe_unused]] bool isMinimized) noexcept
		{
		}

		constexpr void RestoreVolume() noexcept
		{
		}

		constexpr void SetMasterVolume(float volume) noexcept
		{
			m_MasterVolume = volume;
		}

		constexpr void SetListenerPosition([[maybe_unused]] float x, 
		                                   [[maybe_unused]] float y, 
		                                   [[maybe_unused]] float z) noexcept
		{
		}

		constexpr void SetListenerOrientation([[maybe_unused]] float forwardX, 
		                                      [[maybe_unused]] float forwardY, 
		                                      [[maybe_unused]] float forwardZ,
		                                      [[maybe_unused]] float upX, 
		                                      [[maybe_unused]] float upY, 
		                                      [[maybe_unused]] float upZ) noexcept
		{
		}

		constexpr void Update() noexcept
		{
		}

	private:
		constexpr MaSoundInstance* Internal_GetInstance3D([[maybe_unused]] const std::string& name) noexcept
		{
			return nullptr;
		}

		constexpr bool Internal_LoadSoundFromPack([[maybe_unused]] const std::string& name) noexcept
		{
			return true;
		}

	private:
		float m_MusicVolume{ 1.0f };
		float m_MasterVolume{ 1.0f };
	};
}
