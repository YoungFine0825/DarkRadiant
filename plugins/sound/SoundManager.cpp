#include "SoundManager.h"
#include "SoundFileLoader.h"

#include "ifilesystem.h"
#include "icommandsystem.h"

#include "debugging/ScopedDebugTimer.h"
#include "os/path.h"
#include "string/case_conv.h"

#include <algorithm>
#include "itextstream.h"

#include "WavFileLoader.h"
#include "OggFileLoader.h"

namespace sound
{

namespace
{

// Load the given file, trying different extensions (first OGG, then WAV) as fallback
ArchiveFilePtr openSoundFile(const std::string& fileName)
{
    // Make a copy of the filename
    std::string name = fileName;

    // Try to open the file as it is
    auto file = GlobalFileSystem().openFile(name);

    if (file)
    {
        // File found, play it
        return file;
    }

    std::string root = name;

    // File not found, try to strip the extension
    if (name.rfind(".") != std::string::npos)
    {
        root = name.substr(0, name.rfind("."));
    }

    // Try to open the .ogg variant
    name = root + ".ogg";

    file = GlobalFileSystem().openFile(name);

    if (file)
    {
        return file;
    }

    // Try to open the file with .wav extension
    name = root + ".wav";

    return GlobalFileSystem().openFile(name);
}

}

// Constructor
SoundManager::SoundManager() :
    _defLoader(std::bind(&SoundManager::loadShadersFromFilesystem, this)),
	_emptyShader(new SoundShader("", "", vfs::FileInfo(), ""))
{}

// Enumerate shaders
void SoundManager::forEachShader(std::function<void(const ISoundShader&)> f)
{
    ensureShadersLoaded();

	for (const auto& pair : _shaders)
	{
		f(*pair.second);
	}
}

bool SoundManager::playSound(const std::string& fileName)
{
	return playSound(fileName, false);
}

bool SoundManager::playSound(const std::string& fileName, bool loopSound)
{
    auto file = openSoundFile(fileName);

	if (file && _soundPlayer)
	{
		_soundPlayer->play(*file, loopSound);
		return true;
	}

    return false;
}

void SoundManager::stopSound()
{
	if (_soundPlayer) _soundPlayer->stop();
}

sigc::signal<void>& SoundManager::signal_soundShadersReloaded()
{
    return _sigSoundShadersReloaded;
}

ISoundShaderPtr SoundManager::getSoundShader(const std::string& shaderName)
{
    ensureShadersLoaded();

	auto found = _shaders.find(shaderName);

    // If the name was found, return it, otherwise return an empty shader object
	return found != _shaders.end() ? found->second : _emptyShader;
}

const std::string& SoundManager::getName() const
{
	static std::string _name(MODULE_SOUNDMANAGER);
	return _name;
}

const StringSet& SoundManager::getDependencies() const
{
    static StringSet _dependencies { 
        MODULE_VIRTUALFILESYSTEM, MODULE_COMMANDSYSTEM
    };
	return _dependencies;
}

void SoundManager::loadShadersFromFilesystem()
{
    auto foundShaders = std::make_shared<ShaderMap>();

	// Pass a SoundFileLoader to the filesystem
    SoundFileLoader loader(*foundShaders);

    GlobalFileSystem().forEachFile(
        SOUND_FOLDER,			// directory
        "sndshd", 				// required extension
        std::bind(&SoundFileLoader::parseShaderFile, loader, std::placeholders::_1),	// loader callback
        99						// max depth
    );

    _shaders.swap(*foundShaders);

    rMessage() << _shaders.size() << " sound shaders found." << std::endl;

    _sigSoundShadersReloaded.emit();
}

void SoundManager::ensureShadersLoaded()
{
    _defLoader.ensureFinished();
}

void SoundManager::initialiseModule(const IApplicationContext& ctx)
{
    GlobalCommandSystem().addCommand("ReloadSounds", 
        std::bind(&SoundManager::reloadSoundsCmd, this, std::placeholders::_1));

    // Create the SoundPlayer if sound is not disabled
    const auto& args = ctx.getCmdLineArgs();
    auto found = std::find(args.begin(), args.end(), "--disable-sound");

    if (found == args.end())
    {
        rMessage() << "SoundManager: initialising sound playback" << std::endl;
        _soundPlayer.reset(new SoundPlayer);
    }
    else
    {
        rMessage() << "SoundManager: sound output disabled" << std::endl;
    }

    _defLoader.start();
}

float SoundManager::getSoundFileDuration(const std::string& vfsPath)
{
    auto file = openSoundFile(vfsPath);

    if (!file)
    {
        throw std::out_of_range("Could not resolve sound file " + vfsPath);
    }

    auto extension = string::to_lower_copy(os::getExtension(file->getName()));

    try
    {
        if (extension == "wav")
        {
            return WavFileLoader::GetDuration(file->getInputStream());
        }
        else if (extension == "ogg")
        {
            return OggFileLoader::GetDuration(*file);
        }
    }
    catch (const std::runtime_error& ex)
    {
        rError() << "Error determining sound file duration " << ex.what() << std::endl;
    }

    return 0.0f;
}

void SoundManager::reloadSounds()
{
    _defLoader.reset();
    _defLoader.start();
}

void SoundManager::reloadSoundsCmd(const cmd::ArgumentList& args)
{
    reloadSounds();
}

} // namespace sound
