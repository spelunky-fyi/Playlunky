#pragma once

#include <Windows.h>
#include <memory>

class Playlunky {
public:
	static void Create(HMODULE game_module);
	static void Destroy();

	Playlunky& Get();
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
};