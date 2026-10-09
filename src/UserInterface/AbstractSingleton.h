#pragma once

template <typename T> 
class TAbstractSingleton
{ 
	inline static T * ms_singleton = nullptr;
	
public: 
	TAbstractSingleton()
	{ 
		assert(!ms_singleton);
		ms_singleton = static_cast<T*>(this);
	} 

	virtual ~TAbstractSingleton()
	{ 
		assert(ms_singleton);
		ms_singleton = nullptr; 
	}

	__forceinline static T & GetSingleton()
	{
		assert(ms_singleton != nullptr);
		return (*ms_singleton);
	}

	__forceinline static T * GetSingletonPtr()
	{
		return ms_singleton;
	}

	__forceinline static bool HasSingleton()
	{
		return ms_singleton != nullptr;
	}
};
