/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*! \mainpage GtkRadiant Documentation Index

\section intro_sec Introduction

This documentation is generated from comments in the source code.

\section links_sec Useful Links

\link include/itextstream.h include/itextstream.h \endlink - Global output and error message streams, similar to std::cout and std::cerr. \n

FileInputStream - similar to std::ifstream (binary mode) \n
FileOutputStream - similar to std::ofstream (binary mode) \n
TextFileInputStream - similar to std::ifstream (text mode) \n
TextFileOutputStream - similar to std::ofstream (text mode) \n
StringOutputStream - similar to std::stringstream \n

\link string/string.h string/string.h \endlink - C-style string comparison and memory management. \n
\link os/path.h os/path.h \endlink - Path manipulation for radiant's standard path format \n
\link os/file.h os/file.h \endlink - OS file-system access. \n

::CopiedString - automatic string memory management \n
Array - automatic array memory management \n
HashTable - generic hashtable, similar to std::hash_map \n

\link math/matrix.h math/matrix.h \endlink - Matrices \n
\link math/quaternion.h math/quaternion.h \endlink - Quaternions \n
\link math/plane.h math/plane.h \endlink - Planes \n
\link math/aabb.h math/aabb.h \endlink - AABBs \n

Callback MemberCaller FunctionCaller - callbacks similar to using boost::function with boost::bind \n
SmartPointer SmartReference - smart-pointer and smart-reference similar to Loki's SmartPtr \n

\link generic/bitfield.h generic/bitfield.h \endlink - Type-safe bitfield \n
\link generic/enumeration.h generic/enumeration.h \endlink - Type-safe enumeration \n

DefaultAllocator - Memory allocation using new/delete, compliant with std::allocator interface \n

\link debugging/debugging.h debugging/debugging.h \endlink - Debugging macros \n

*/

#include "main.h"

#include "version.h"

#include "debugging/debugging.h"

#include "iundo.h"
#include "iregistry.h"

#include <gtk/gtkmain.h>

#include "cmdlib.h"
#include "os/file.h"
#include "os/path.h"
#include "stream/stringstream.h"
#include "stream/textfilestream.h"

#include "gtkutil/messagebox.h"
#include "gtkutil/image.h"
#include "console.h"
#include "texwindow.h"
#include "map.h"
#include "mainframe.h"
#include "preferences.h"
#include "environment.h"
#include "referencecache.h"
#include "stacktrace.h"
#include "server.h"
#include "ui/mru/MRU.h"
#include "settings/GameManager.h"

#include <iostream>

void show_splash();
void hide_splash();

#if defined (_DEBUG) && defined (WIN32) && defined (_MSC_VER)
#include "crtdbg.h"
#endif

void crt_init()
{
#if defined (_DEBUG) && defined (WIN32) && defined (_MSC_VER)
  _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
}

class Lock
{
  bool m_locked;
public:
  Lock() : m_locked(false)
  {
  }
  void lock()
  {
    m_locked = true;
  }
  void unlock()
  {
    m_locked = false;
  }
  bool locked() const
  {
    return m_locked;
  }
};

class ScopedLock
{
  Lock& m_lock;
public:
  ScopedLock(Lock& lock) : m_lock(lock)
  {
    m_lock.lock();
  }
  ~ScopedLock()
  {
    m_lock.unlock();
  }
};

class LineLimitedTextOutputStream : public TextOutputStream
{
  TextOutputStream& outputStream;
  std::size_t count;
public:
  LineLimitedTextOutputStream(TextOutputStream& outputStream, std::size_t count)
    : outputStream(outputStream), count(count)
  {
  }
  std::size_t write(const char* buffer, std::size_t length)
  {
    if(count != 0)
    {
      const char* p = buffer;
      const char* end = buffer+length;
      for(;;)
      {
        p = std::find(p, end, '\n');
        if(p == end)
        {
          break;
        }
        ++p;
        if(--count == 0)
        {
          length = p - buffer;
          break;
        }
      }
      outputStream.write(buffer, length);
    }
    return length;
  }
};

class PopupDebugMessageHandler : public DebugMessageHandler
{
  StringOutputStream m_buffer;
  Lock m_lock;
public:
  TextOutputStream& getOutputStream()
  {
    if(!m_lock.locked())
    {
      return m_buffer;
    }
    return globalErrorStream();
  }
  bool handleMessage()
  {
    getOutputStream() << "----------------\n";
    LineLimitedTextOutputStream outputStream(getOutputStream(), 24);
    write_stack_trace(outputStream);
    getOutputStream() << "----------------\n";
    globalErrorStream() << m_buffer.c_str();
    if(!m_lock.locked())
    {
      ScopedLock lock(m_lock);
#if defined _DEBUG
      m_buffer << "Break into the debugger?\n";
      bool handled = gtk_MessageBox(0, m_buffer.c_str(), "Radiant - Runtime Error", eMB_YESNO, eMB_ICONERROR) == eIDNO;
      m_buffer.clear();
      return handled;
#else
      m_buffer << "Please report this error to the developers\n";
      gtk_MessageBox(0, m_buffer.c_str(), "Radiant - Runtime Error", eMB_OK, eMB_ICONERROR);
      m_buffer.clear();
#endif
    }
    return true;
  }
};

typedef Static<PopupDebugMessageHandler> GlobalPopupDebugMessageHandler;

void streams_init()
{
  GlobalErrorStream::instance().setOutputStream(getSysPrintErrorStream());
  GlobalOutputStream::instance().setOutputStream(getSysPrintOutputStream());
}

void createPIDFile(const std::string& name) {
  /*!
  the global prefs loading / game selection dialog might fail for any reason we don't know about
  we need to catch when it happens, to cleanup the stateful prefs which might be killing it
  and to turn on console logging for lookup of the problem
  this is the first part of the two step .pid system
  http://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=297
  */
	std::string pidFile = GlobalRegistry().get(RKEY_SETTINGS_PATH) + name;

	FILE *pid;
	pid = fopen(pidFile.c_str(), "r");
	
	// Check for an existing radiant.pid file
	if (pid != NULL) {
		fclose (pid);

		if (remove (pidFile.c_str()) == -1) {
			std::string msg = "WARNING: Could not delete " + pidFile;
			gtk_MessageBox(0, msg.c_str(), "Radiant", eMB_OK, eMB_ICONERROR );
		}

    // in debug, never prompt to clean registry, turn console logging auto after a failed start
#if !defined(_DEBUG)
    std::string msg("Radiant failed to start properly the last time it was run.\n");
    msg += "The failure may be related to current global preferences.\n";
	msg += "Do you want to reset global preferences to defaults?";

	if (gtk_MessageBox(0, msg.c_str(), "Radiant - Startup Failure", eMB_YESNO, eMB_ICONQUESTION) == eIDYES) {
		ui::GameDialog::Instance().Reset();
		Preferences_Reset();
	}
#endif
  }

	// create a primary .pid for global init run
	pid = fopen (pidFile.c_str(), "w");
	if (pid) {
		fclose (pid);
	}
}

void removePIDFile(const std::string& name) {
	std::string pidFile = GlobalRegistry().get(RKEY_SETTINGS_PATH) + name;

	// close the primary
	if (remove (pidFile.c_str()) == -1) {
		std::string msg = "WARNING: Could not delete " + pidFile;
		gtk_MessageBox (0, msg.c_str(), "Radiant", eMB_OK, eMB_ICONERROR );
	}
}

int main (int argc, char* argv[])
{
  crt_init();

  streams_init();

  gtk_disable_setlocale();
  gtk_init(&argc, &argv);

  GlobalDebugMessageHandler::instance().setHandler(GlobalPopupDebugMessageHandler::instance());

	// Retrieve the application path and such
	Environment::Instance().init(argc, argv);

	// Load the Radiant modules from the modules/ and plugins/ dir.
	ModuleLoader::loadModules(Environment::Instance().getAppPath());

	// Initialise and instantiate the XMLRegistry
	GlobalModuleServer::instance().set(GlobalModuleServer_get());
	GlobalRegistryModuleRef registryRef;

	// Tell the Environment class to store the paths into the Registry
	Environment::Instance().savePathsToRegistry();

	Sys_LogFile(true);

  show_splash();

	// Create the radiant.pid file in the settings folder 
	// (emits a warning if the file already exists (due to a previous startup failure) 
	createPIDFile("radiant.pid");

	// Load the game files from the <application>/games folder and 
	// let the user choose the game, if nothing is found in the Registry
	game::Manager::Instance().initialise();

	ui::GameDialog::Instance().initialise();
  g_Preferences.Init(); 

  Radiant_Initialise();

  g_pParentWnd = 0;
  g_pParentWnd = new MainFrame();

  hide_splash();

	if (GlobalMRU().loadLastMap() && GlobalMRU().getLastMapName() != "") {
		Map_LoadFile(GlobalMRU().getLastMapName().c_str());
	}
	else {
		Map_New();
	}

	// Remove the radiant.pid file again after loading all the settings
	removePIDFile("radiant.pid");

  gtk_main();

  // avoid saving prefs when the app is minimized
  if (g_pParentWnd->IsSleeping())
  {
    globalOutputStream() << "Shutdown while sleeping, not saving prefs\n";
    g_preferences_globals.disable_ini = true;
  }

  Map_Free();

  delete g_pParentWnd;

  Radiant_Shutdown();

  // close the log file if any
  Sys_LogFile(false);

  return EXIT_SUCCESS;
}

