#include "ParticlesManager.h"

#include "ParticleDef.h"
#include "StageDef.h"
#include "ParticleNode.h"
#include "RenderableParticle.h"

#include "icommandsystem.h"
#include "itextstream.h"
#include "ifilesystem.h"
#include "ifiletypes.h"
#include "iarchive.h"
#include "igame.h"
#include "i18n.h"

#include "parser/DefTokeniser.h"
#include "decl/SpliceHelper.h"
#include "stream/TemporaryOutputStream.h"
#include "math/Vector4.h"
#include "os/fs.h"

#include "debugging/ScopedDebugTimer.h"

#include <fstream>
#include <iostream>
#include <functional>
#include <regex>
#include "string/predicate.h"
#include "module/StaticModule.h"

namespace particles
{

namespace
{
    inline void writeParticleCommentHeader(std::ostream& str)
    {
        // The comment at the head of the decl
        str << "/*" << std::endl
                    << "\tGenerated by DarkRadiant's Particle Editor."
                    << std::endl <<	"*/" << std::endl;
    }
}

ParticlesManager::ParticlesManager() :
    _defLoader(_particleDefs)
{
    _defLoader.signal_finished().connect(
        sigc::mem_fun(this, &ParticlesManager::onParticlesLoaded));
}

sigc::signal<void>& ParticlesManager::signal_particlesReloaded()
{
    return _particlesReloadedSignal;
}

// Visit all of the particle defs
void ParticlesManager::forEachParticleDef(const ParticleDefVisitor& v)
{
    ensureDefsLoaded();

	// Invoke the visitor for each ParticleDef object
	for (const ParticleDefMap::value_type& pair : _particleDefs)
	{
		v(*pair.second);
	}
}

IParticleDef::Ptr ParticlesManager::getDefByName(const std::string& name)
{
    ensureDefsLoaded();

	ParticleDefMap::const_iterator found = _particleDefs.find(name);

	return found != _particleDefs.end() ? found->second : IParticleDef::Ptr();
}

IParticleNodePtr ParticlesManager::createParticleNode(const std::string& name)
{
	std::string nameCleaned = name;

	// Cut off the ".prt" from the end of the particle name
	if (string::ends_with(nameCleaned, ".prt"))
	{
		nameCleaned = nameCleaned.substr(0, nameCleaned.length() - 4);
	}

    ensureDefsLoaded();

	ParticleDefMap::const_iterator found = _particleDefs.find(nameCleaned);

	if (found == _particleDefs.end())
	{
		return IParticleNodePtr();
	}

	RenderableParticlePtr renderable(new RenderableParticle(found->second));
	return ParticleNodePtr(new ParticleNode(renderable));
}

IRenderableParticlePtr ParticlesManager::getRenderableParticle(const std::string& name)
{
    ensureDefsLoaded();

	ParticleDefMap::const_iterator found = _particleDefs.find(name);

	if (found != _particleDefs.end())
	{
		return RenderableParticlePtr(new RenderableParticle(found->second));
	}
	else
	{
		return IRenderableParticlePtr();
	}
}

IParticleDef::Ptr ParticlesManager::findOrInsertParticleDef(const std::string& name)
{
    ensureDefsLoaded();

    return findOrInsertParticleDefInternal(name);
}

ParticleDefPtr ParticlesManager::findOrInsertParticleDefInternal(const std::string& name)
{
    ParticleDefMap::iterator i = _particleDefs.find(name);

    if (i != _particleDefs.end())
    {
        // Particle def is already existing in the map
        return i->second;
    }

    // Not existing, add a new ParticleDef to the map
    std::pair<ParticleDefMap::iterator, bool> result = _particleDefs.insert(
        ParticleDefMap::value_type(name, ParticleDefPtr(new ParticleDef(name))));

    // Return the iterator from the insertion result
    return result.first->second;
}

void ParticlesManager::removeParticleDef(const std::string& name)
{
    ensureDefsLoaded();

	ParticleDefMap::iterator i = _particleDefs.find(name);

	if (i != _particleDefs.end())
	{
		_particleDefs.erase(i);
	}
}

void ParticlesManager::ensureDefsLoaded()
{
    _defLoader.ensureFinished();
}

const std::string& ParticlesManager::getName() const
{
	static std::string _name(MODULE_PARTICLESMANAGER);
	return _name;
}

const StringSet& ParticlesManager::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_VIRTUALFILESYSTEM,
        MODULE_COMMANDSYSTEM,
        MODULE_FILETYPES,
    };

	return _dependencies;
}

void ParticlesManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << "ParticlesManager::initialiseModule called" << std::endl;

	// Load the .prt files in a new thread, public methods will block until
    // this has been completed
    _defLoader.start();

	// Register the "ReloadParticles" commands
	GlobalCommandSystem().addCommand("ReloadParticles", std::bind(&ParticlesManager::reloadParticleDefs, this));

	// Register the particle file extension
	GlobalFiletypes().registerPattern("particle", FileTypePattern(_("Particle File"), "prt", "*.prt"));
}

void ParticlesManager::reloadParticleDefs()
{
    _particleDefs.clear();

    _defLoader.reset();
    _defLoader.start();
}

void ParticlesManager::onParticlesLoaded()
{
    // Notify observers about this event
    _particlesReloadedSignal.emit();
}

void ParticlesManager::saveParticleDef(const std::string& particleName)
{
    ensureDefsLoaded();

	ParticleDefMap::const_iterator found = _particleDefs.find(particleName);

	if (found == _particleDefs.end())
	{
		throw std::runtime_error(_("Cannot save particle, it has not been registered yet."));
	}

	ParticleDefPtr particle = found->second;

	std::string relativePath = PARTICLES_DIR + particle->getFilename();

	fs::path targetPath = GlobalGameManager().getModPath();

	if (targetPath.empty())
	{
		targetPath = GlobalGameManager().getUserEnginePath();

		rMessage() << "No mod base path found, falling back to user engine path to save particle file: " <<
			targetPath.string() << std::endl;
	}

	targetPath /= PARTICLES_DIR;

	// Ensure the particles folder exists
	fs::create_directories(targetPath);

	fs::path targetFile = targetPath / particle->getFilename();

	// If the file doesn't exist yet, let's check if we need to inherit stuff first from the VFS
	if (!fs::exists(targetFile))
	{
		ArchiveTextFilePtr inheritFile = GlobalFileSystem().openTextFile(relativePath);

		if (inheritFile)
		{
			// There is a file with that name already in the VFS, copy it to the target file
			TextInputStream& inheritStream = inheritFile->getInputStream();

			std::ofstream outFile(targetFile.string().c_str());

			if (!outFile.is_open())
			{
				throw std::runtime_error(
					fmt::format(_("Cannot open file for writing: {0}"), targetFile.string()));
			}

			char buf[16384];
			std::size_t bytesRead = inheritStream.read(buf, sizeof(buf));

			while (bytesRead > 0)
			{
				outFile.write(buf, bytesRead);

				bytesRead = inheritStream.read(buf, sizeof(buf));
			}

			outFile.close();
		}
	}

	// Open a temporary file
    stream::TemporaryOutputStream tempStream(targetFile);

	std::string tempString;
    auto& stream = tempStream.getStream();

	// If a previous file exists, open it for reading and filter out the particle def we'll be writing
	if (fs::exists(targetFile))
	{
		std::ifstream inheritStream(targetFile.string().c_str());

		if (!inheritStream.is_open())
		{
			throw std::runtime_error(
					fmt::format(_("Cannot open file for reading: {0}"), targetFile.string()));
		}

		// Write the file to the output stream, up to the point the particle def should be written to
        std::regex pattern("^[\\s]*particle[\\s]+" + particleName + "\\s*(\\{)*\\s*$");

        decl::SpliceHelper::PipeStreamUntilInsertionPoint(inheritStream, stream, pattern);

		if (inheritStream.eof())
		{
			// Particle def was not found in the inherited stream, write our comment
            stream << std::endl << std::endl;

			writeParticleCommentHeader(stream);
		}

		// We're at the insertion point (which might as well be EOF of the inheritStream)

		// Write the particle declaration
        stream << *particle << std::endl;

        stream << inheritStream.rdbuf();

		inheritStream.close();
	}
	else
	{
		// Just put the particle def into the file and that's it, leave a comment at the head of the decl
		writeParticleCommentHeader(stream);

		// Write the particle declaration
        stream << *particle << std::endl;
	}

	tempStream.closeAndReplaceTargetFile();
}

module::StaticModuleRegistration<ParticlesManager> particlesManagerModule;

} // namespace particles
