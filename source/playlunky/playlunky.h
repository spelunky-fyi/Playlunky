#pragma once

#include <Windows.h>
#include <memory>

class PlaylunkySettings;

enum class ModType {
	None = 0,
	Sprite = 1 << 1,
	CharacterSprite = 1 << 2,
	String = 1 << 3,
	Shader = 1 << 4,
	Sound = 1 << 5,
	Script = 1 << 6,
	Level = 1 << 7,
};
inline ModType operator|(ModType lhs, ModType rhs) {
	return static_cast<ModType>(static_cast<int>(lhs) | static_cast<int>(rhs));
}
inline ModType operator&(ModType lhs, ModType rhs) {
	return static_cast<ModType>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

class Playlunky {
public:
	static Playlunky& Get();
	
	static void Create(HMODULE game_module);
	static void Destroy();

	void Init();
	void PostGameInit();

	const PlaylunkySettings& GetSettings() const;

	void RegisterModType(ModType mod_type) { mLoadedModTypes = mLoadedModTypes | mod_type; }
	bool IsModTypeLoaded(ModType mod_type) const { return (mLoadedModTypes & mod_type) != ModType::None; }

private:
	Playlunky(HMODULE game_module);
	~Playlunky();

	Playlunky() = delete;
	Playlunky(const Playlunky&) = delete;
	Playlunky(Playlunky&&) = delete;
	Playlunky& operator=(const Playlunky&) = delete;
	Playlunky& operator=(Playlunky&&) = delete;

	struct PlaylunkyImpl;
	friend struct PlaylunkyDeleter;
	std::unique_ptr<PlaylunkyImpl> mImpl;

	ModType mLoadedModTypes;
};
