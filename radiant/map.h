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

#if !defined(INCLUDED_MAP_H)
#define INCLUDED_MAP_H

#include "iscenegraph.h"
#include "ireference.h"
#include "generic/callback.h"
#include "signal/signal.h"
#include "moduleobserver.h"

#include <ostream>
#include <string>

class AABB;

class Map : 
	public ModuleObserver
{
public:
	// The map name
	std::string m_name;
	
	// Pointer to the Model Resource for this map
	ReferenceCache::ResourcePtr m_resource;
	
	bool m_valid;

	bool m_modified;

	Signal0 m_mapValidCallbacks;

	scene::INodePtr m_world_node; // "classname" "worldspawn" !

public:
	Map();
	
	void realise();
	void unrealise();
  
	// Accessor methods for the "valid" flag
	void setValid(bool valid);
	bool isValid() const;
	
	void addValidCallback(const SignalHandler& handler);
	
	// Updates the window title of the mainframe
	void updateTitle();
	
	// Accessor methods for the worldspawn node
	void setWorldspawn(scene::INodePtr node);
	scene::INodePtr getWorldspawn();
};

extern Map g_map;

class MapFormat;
const MapFormat& MapFormat_forFile(const std::string& filename);

class DeferredDraw
{
  Callback m_draw;
  bool m_defer;
  bool m_deferred;
public:
  DeferredDraw(const Callback& draw) : m_draw(draw), m_defer(false), m_deferred(false)
  {
  }
  void defer()
  {
    m_defer = true;
  }
  void draw()
  {
    if(m_defer)
    {
      m_deferred = true;
    }
    else
    {
      m_draw();
    }
  }
  void flush()
  {
    if(m_defer && m_deferred)
    {
      m_draw();
    }
    m_deferred = false;
    m_defer = false;
  }
};

inline void DeferredDraw_onMapValidChanged(DeferredDraw& self)
{
  if(g_map.isValid())
  {
    self.flush();
  }
  else
  {
    self.defer();
  }
}
typedef ReferenceCaller<DeferredDraw, DeferredDraw_onMapValidChanged> DeferredDrawOnMapValidChangedCaller;

bool ConfirmModified(const char* title);

const MapFormat& Map_getFormat(const Map& map);
bool Map_Unnamed(const Map& map);

//scene::INodePtr Map_GetWorldspawn(const Map& map);
scene::INodePtr Map_FindWorldspawn(Map& map);
scene::INodePtr Map_FindOrInsertWorldspawn(Map& map);

template<typename Element> class BasicVector3;
typedef BasicVector3<double> Vector3;

void Map_LoadFile(const std::string& filename);
bool Map_SaveFile(const char* filename);

void Map_New();
void Map_Free();

class TextInputStream;
class TextOutputStream;

// Map import and export functions
void Map_ImportSelected(TextInputStream& in, const MapFormat& format);
void Map_ExportSelected(std::ostream& out, const MapFormat& format);

bool Map_Modified(const Map& map);

void Map_Save();
bool Map_SaveAs();

scene::INodePtr Node_Clone(scene::INodePtr node);

void DoMapInfo();

void Scene_parentSelectedPrimitivesToEntity(scene::Graph& graph, scene::INodePtr parent);

void OnUndoSizeChanged();

void NewMap();
void OpenMap();
void ImportMap();
void SaveMapAs();
void SaveMap();
void ExportMap();

class Entity;
Entity* Scene_FindEntityByClass(const std::string& className);

void Map_Traverse(scene::INodePtr root, const scene::Traversable::Walker& walker);


void SelectBrush (int entitynum, int brushnum);

void Map_Construct();
void Map_Destroy();

namespace map {
	/**
	 * Return the filename of the current map, as a string.
	 */
	std::string getFileName();

	/** Subtract the provided origin vector from all selected brushes. This is 
	 * necessary when reparenting worldspawn brushes to an entity, since the entity's
	 * "origin" key will be added to all child brushes.
	 * 
	 * @param origin
	 * Vector3 containing the new origin for the selected brushes.
	 */
	 
	void selectedPrimitivesSubtractOrigin(const Vector3& origin);
	
	/** Count the number of selected primitives in the current map.
	 * 
	 * @returns
	 * The number of selected primitives.
	 */
	 
	int countSelectedPrimitives();
	
	/** Count the number of selected brushes in the current map.
	 * 
	 * @returns
	 * The number of selected brushes.
	 */
	 
	int countSelectedBrushes();
	
	/**
	 * Set the "modified" flag on the current map, to allow saving.
	 */
	void setModified(bool modifiedFlag);
	
	/** greebo: Focus the XYViews and the Camera to the given point/angle.
	 */
	void focusViews(const Vector3& point, const Vector3& angles);
	
	/** greebo: This adds/removes the origin from all the child primitivies
	 * 			of container entities like func_static. This has to be called
	 * 			right after/before a map save and load process.
	 */
	void removeOriginFromChildPrimitives();
	void addOriginToChildPrimitives();
	
	/** greebo: Returns the AABB enclosing all visible map objects.
	 */
	AABB getVisibleBounds();
	
	/** greebo: Asks the user for the .pfb file and imports/exports the file/selection
	 */
	void loadPrefab();
	void saveSelectedAsPrefab();
	
	/** greebo: Loads a prefab and translates it to the given target coordinates
	 */
	void loadPrefabAt(const Vector3& targetCoords);
}


#endif
