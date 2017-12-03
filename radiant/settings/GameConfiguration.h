#pragma once

#include "igame.h"
#include "registry/registry.h"
#include "os/path.h"

namespace game
{

/**
* Represents the game configuration as specified by the user
* in the Game Settings dialog, comprising Game name,
* engine path, mod path, etc.
*/
class GameConfiguration
{
public:
	// The name of the current game, e.g. "Doom 3"
	std::string gameType;

	// The engine path (pointing to the game executable)
	std::string enginePath;

	// The "userengine" path (where the fs_game is stored)
	// this is ~/.doom3/<fs_game> in linux, and <enginepath>/<fs_game> in Win32
	std::string modBasePath;

	// The "mod mod" path (where the fs_game_base is stored)
	// this is ~/.doom3/<fs_game_base> in linux, and <enginepath>/<fs_game_base> in Win32
	std::string modPath;

	// Loads the property values of this instance from the XMLRegistry
	void loadFromRegistry()
	{
		gameType = os::standardPathWithSlash(registry::getValue<std::string>(RKEY_GAME_TYPE));
		enginePath = os::standardPathWithSlash(registry::getValue<std::string>(RKEY_ENGINE_PATH));
		modPath = os::standardPathWithSlash(registry::getValue<std::string>(RKEY_MOD_PATH));
		modBasePath = os::standardPathWithSlash(registry::getValue<std::string>(RKEY_MOD_BASE_PATH));
	}

	// Persists the values of this instance to the XMLRegistry
	void saveToRegistry()
	{
		registry::setValue(RKEY_GAME_TYPE, gameType);
		registry::setValue(RKEY_ENGINE_PATH, enginePath);
		registry::setValue(RKEY_MOD_PATH, modPath);
		registry::setValue(RKEY_MOD_BASE_PATH, modBasePath);
	}
};

}
