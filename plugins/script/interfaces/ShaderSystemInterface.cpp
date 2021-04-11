#include "ShaderSystemInterface.h"

#include <functional>
#include <pybind11/stl.h>

namespace script
{

namespace
{
	class ShaderNameToShaderWrapper
	{
	private:
        MaterialVisitor& _visitor;

	public:
		ShaderNameToShaderWrapper(MaterialVisitor& visitor) :
			_visitor(visitor)
		{}

		void visit(const std::string& name)
		{
			// Resolve the material name and pass it on
			auto material = GlobalMaterialManager().getMaterial(name);
			_visitor.visit(material);
		}
	};
}

void ShaderSystemInterface::foreachMaterial(MaterialVisitor& visitor)
{
	// Note: foreachShader only traverses the loaded materials, use a small adaptor to traverse all known
	ShaderNameToShaderWrapper adaptor(visitor);

	GlobalMaterialManager().foreachShaderName(
		std::bind(&ShaderNameToShaderWrapper::visit, &adaptor, std::placeholders::_1));
}

ScriptMaterial ShaderSystemInterface::getMaterial(const std::string& name)
{
	return ScriptMaterial(GlobalMaterialManager().getMaterial(name));
}

bool ShaderSystemInterface::materialExists(const std::string& name)
{
    return GlobalMaterialManager().materialExists(name);
}

bool ShaderSystemInterface::materialCanBeModified(const std::string& name)
{
    return GlobalMaterialManager().materialCanBeModified(name);
}

ScriptMaterial ShaderSystemInterface::createEmptyMaterial(const std::string& name)
{
    return ScriptMaterial(GlobalMaterialManager().createEmptyMaterial(name));
}

ScriptMaterial ShaderSystemInterface::copyMaterial(const std::string& nameOfOriginal, const std::string& nameOfCopy)
{
    return ScriptMaterial(GlobalMaterialManager().copyMaterial(nameOfOriginal, nameOfCopy));
}

bool ShaderSystemInterface::renameMaterial(const std::string& oldName, const std::string& newName)
{
    return GlobalMaterialManager().renameMaterial(oldName, newName);
}

void ShaderSystemInterface::removeMaterial(const std::string& name)
{
    GlobalMaterialManager().removeMaterial(name);
}

void ShaderSystemInterface::saveMaterial(const std::string& name)
{
    GlobalMaterialManager().saveMaterial(name);
}

// IScriptInterface implementation
void ShaderSystemInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Add the declaration for a Material and Stage object
	py::class_<ScriptMaterial> material(scope, "Material");
	py::class_<ScriptMaterialStage> stage(scope, "MaterialStage");

    // Add the old name as alias
    scope.add_object("Shader", material);

    // Expose the enums in the material scope
    py::enum_<Material::SortRequest>(material, "SortRequest")
        .value("SUBVIEW", Material::SORT_SUBVIEW)
        .value("GUI", Material::SORT_GUI)
        .value("BAD", Material::SORT_BAD)
        .value("OPAQUE", Material::SORT_OPAQUE)
        .value("PORTAL_SKY", Material::SORT_PORTAL_SKY)
        .value("DECAL", Material::SORT_DECAL)
        .value("FAR", Material::SORT_FAR)
        .value("MEDIUM", Material::SORT_MEDIUM)
        .value("CLOSE", Material::SORT_CLOSE)
        .value("ALMOST_NEAREST", Material::SORT_ALMOST_NEAREST)
        .value("NEAREST", Material::SORT_NEAREST)
        .value("AFTER_FOG", Material::SORT_AFTER_FOG)
        .value("POST_PROCESS", Material::SORT_POST_PROCESS)
        .export_values();

    py::enum_<Material::CullType>(material, "CullType")
        .value("BACK", Material::CULL_BACK)
        .value("FRONT", Material::CULL_FRONT)
        .value("NONE", Material::CULL_NONE)
        .export_values();

    py::enum_<ClampType>(material, "ClampType")
        .value("REPEAT", CLAMP_REPEAT)
        .value("NOREPEAT", CLAMP_NOREPEAT)
        .value("ZEROCLAMP", CLAMP_ZEROCLAMP)
        .value("ALPHAZEROCLAMP", CLAMP_ALPHAZEROCLAMP)
        .export_values();

    py::enum_<Material::Flags>(material, "Flag")
        .value("NOSHADOWS", Material::FLAG_NOSHADOWS)
        .value("NOSELFSHADOW", Material::FLAG_NOSELFSHADOW)
        .value("FORCESHADOWS", Material::FLAG_FORCESHADOWS)
        .value("NOOVERLAYS", Material::FLAG_NOOVERLAYS)
        .value("FORCEOVERLAYS", Material::FLAG_FORCEOVERLAYS)
        .value("TRANSLUCENT", Material::FLAG_TRANSLUCENT)
        .value("FORCEOPAQUE", Material::FLAG_FORCEOPAQUE)
        .value("NOFOG", Material::FLAG_NOFOG)
        .value("NOPORTALFOG", Material::FLAG_NOPORTALFOG)
        .value("UNSMOOTHEDTANGENTS", Material::FLAG_UNSMOOTHEDTANGENTS)
        .value("MIRROR", Material::FLAG_MIRROR)
        .value("POLYGONOFFSET", Material::FLAG_POLYGONOFFSET)
        .value("ISLIGHTGEMSURF", Material::FLAG_ISLIGHTGEMSURF)
        .export_values();

    py::enum_<Material::SurfaceFlags>(material, "SurfaceFlag")
        .value("SOLID", Material::SURF_SOLID)
        .value("OPAQUE", Material::SURF_OPAQUE)
        .value("WATER", Material::SURF_WATER)
        .value("PLAYERCLIP", Material::SURF_PLAYERCLIP)
        .value("MONSTERCLIP", Material::SURF_MONSTERCLIP)
        .value("MOVEABLECLIP", Material::SURF_MOVEABLECLIP)
        .value("IKCLIP", Material::SURF_IKCLIP)
        .value("BLOOD", Material::SURF_BLOOD)
        .value("TRIGGER", Material::SURF_TRIGGER)
        .value("AASSOLID", Material::SURF_AASSOLID)
        .value("AASOBSTACLE", Material::SURF_AASOBSTACLE)
        .value("FLASHLIGHT_TRIGGER", Material::SURF_FLASHLIGHT_TRIGGER)
        .value("NONSOLID", Material::SURF_NONSOLID)
        .value("NULLNORMAL", Material::SURF_NULLNORMAL)
        .value("AREAPORTAL", Material::SURF_AREAPORTAL)
        .value("NOCARVE", Material::SURF_NOCARVE)
        .value("DISCRETE", Material::SURF_DISCRETE)
        .value("NOFRAGMENT", Material::SURF_NOFRAGMENT)
        .value("SLICK", Material::SURF_SLICK)
        .value("COLLISION", Material::SURF_COLLISION)
        .value("NOIMPACT", Material::SURF_NOIMPACT)
        .value("NODAMAGE", Material::SURF_NODAMAGE)
        .value("LADDER", Material::SURF_LADDER)
        .value("NOSTEPS", Material::SURF_NOSTEPS)
        .value("GUISURF", Material::SURF_GUISURF)
        .value("ENTITYGUI", Material::SURF_ENTITYGUI)
        .value("ENTITYGUI2", Material::SURF_ENTITYGUI2)
        .value("ENTITYGUI3", Material::SURF_ENTITYGUI3)
        .export_values();

    py::enum_<Material::SurfaceType>(material, "SurfaceType")
        .value("DEFAULT", Material::SURFTYPE_DEFAULT)
        .value("METAL", Material::SURFTYPE_METAL)
        .value("STONE", Material::SURFTYPE_STONE)
        .value("FLESH", Material::SURFTYPE_FLESH)
        .value("WOOD", Material::SURFTYPE_WOOD)
        .value("CARDBOARD", Material::SURFTYPE_CARDBOARD)
        .value("LIQUID", Material::SURFTYPE_LIQUID)
        .value("GLASS", Material::SURFTYPE_GLASS)
        .value("PLASTIC", Material::SURFTYPE_PLASTIC)
        .value("RICOCHET", Material::SURFTYPE_RICOCHET)
        .value("AASOBSTACLE", Material::SURFTYPE_AASOBSTACLE)
        .value("SURFTYPE10", Material::SURFTYPE_10)
        .value("SURFTYPE11", Material::SURFTYPE_11)
        .value("SURFTYPE12", Material::SURFTYPE_12)
        .value("SURFTYPE13", Material::SURFTYPE_13)
        .value("SURFTYPE14", Material::SURFTYPE_14)
        .value("SURFTYPE15", Material::SURFTYPE_15)
        .export_values();

    py::enum_<Material::DeformType>(material, "DeformType")
        .value("NONE", Material::DEFORM_NONE)
        .value("SPRITE", Material::DEFORM_SPRITE)
        .value("TUBE", Material::DEFORM_TUBE)
        .value("FLARE", Material::DEFORM_FLARE)
        .value("EXPAND", Material::DEFORM_EXPAND)
        .value("MOVE", Material::DEFORM_MOVE)
        .value("TURBULENT", Material::DEFORM_TURBULENT)
        .value("EYEBALL", Material::DEFORM_EYEBALL)
        .value("PARTICLE", Material::DEFORM_PARTICLE)
        .value("PARTICLE2", Material::DEFORM_PARTICLE2)
        .export_values();

    py::class_<Material::DecalInfo> decalInfo(material, "DecalInfo");

    decalInfo.def_readwrite("stayMilliSeconds", &Material::DecalInfo::stayMilliSeconds);
    decalInfo.def_readwrite("fadeMilliSeconds", &Material::DecalInfo::fadeMilliSeconds);
    decalInfo.def_readwrite("startColour", &Material::DecalInfo::startColour);
    decalInfo.def_readwrite("endColour", &Material::DecalInfo::endColour);

    py::enum_<Material::Coverage>(material, "Coverage")
        .value("UNDETERMINED", Material::MC_UNDETERMINED)
        .value("OPAQUE", Material::MC_OPAQUE)
        .value("PERFORATED", Material::MC_PERFORATED)
        .value("TRANSLUCENT", Material::MC_TRANSLUCENT)
        .export_values();

    py::enum_<IShaderLayer::Type>(stage, "Type")
        .value("DIFFUSE", IShaderLayer::Type::DIFFUSE)
        .value("BUMP", IShaderLayer::Type::BUMP)
        .value("SPECULAR", IShaderLayer::Type::SPECULAR)
        .value("BLEND", IShaderLayer::Type::BLEND)
        .export_values();

    py::enum_<IShaderLayer::MapType>(stage, "MapType")
        .value("MAP", IShaderLayer::MapType::Map)
        .value("CUBEMAP", IShaderLayer::MapType::CubeMap)
        .value("CAMERACUBEMAP", IShaderLayer::MapType::CameraCubeMap)
        .value("VIDEOMAP", IShaderLayer::MapType::VideoMap)
        .value("SOUNDMAP", IShaderLayer::MapType::SoundMap)
        .value("MIRRORRENDERMAP", IShaderLayer::MapType::MirrorRenderMap)
        .value("REMOTERENDERMAP", IShaderLayer::MapType::RemoteRenderMap)
        .export_values();

	material.def(py::init<const MaterialPtr&>());
	material.def("getName", &ScriptMaterial::getName);
	material.def("getShaderFileName", &ScriptMaterial::getShaderFileName);
	material.def("setShaderFileName", &ScriptMaterial::setShaderFileName);
	material.def("getDescription", &ScriptMaterial::getDescription);
    material.def("setDescription", &ScriptMaterial::setDescription);
	material.def("getDefinition", &ScriptMaterial::getDefinition);
	material.def("isVisible", &ScriptMaterial::isVisible);
	material.def("isAmbientLight", &ScriptMaterial::isAmbientLight);
	material.def("isBlendLight", &ScriptMaterial::isBlendLight);
	material.def("isFogLight", &ScriptMaterial::isFogLight);
	material.def("isCubicLight", &ScriptMaterial::isCubicLight);
	material.def("setIsAmbientLight", &ScriptMaterial::setIsAmbientLight);
	material.def("setIsBlendLight", &ScriptMaterial::setIsBlendLight);
	material.def("setIsFogLight", &ScriptMaterial::setIsFogLight);
	material.def("setIsCubicLight", &ScriptMaterial::setIsCubicLight);
	material.def("isNull", &ScriptMaterial::isNull);
	material.def("getEditorImageExpressionString", &ScriptMaterial::getEditorImageExpressionString);
	material.def("setEditorImageExpressionFromString", &ScriptMaterial::setEditorImageExpressionFromString);
	material.def("getSortRequest", &ScriptMaterial::getSortRequest);
	material.def("setSortRequest", static_cast<void(ScriptMaterial::*)(float)>(&ScriptMaterial::setSortRequest));
	material.def("setSortRequest", static_cast<void(ScriptMaterial::*)(Material::SortRequest)>(&ScriptMaterial::setSortRequest));
	material.def("resetSortRequest", &ScriptMaterial::resetSortRequest);
	material.def("getPolygonOffset", &ScriptMaterial::getPolygonOffset);
	material.def("setPolygonOffset", &ScriptMaterial::setPolygonOffset);
	material.def("clearPolygonOffset", &ScriptMaterial::clearPolygonOffset);
	material.def("getClampType", &ScriptMaterial::getClampType);
	material.def("setClampType", &ScriptMaterial::setClampType);
	material.def("getCullType", &ScriptMaterial::getCullType);
	material.def("setCullType", &ScriptMaterial::setCullType);
	material.def("getMaterialFlags", &ScriptMaterial::getMaterialFlags);
	material.def("setMaterialFlag", &ScriptMaterial::setMaterialFlag);
	material.def("clearMaterialFlag", &ScriptMaterial::clearMaterialFlag);
	material.def("getSurfaceFlags", &ScriptMaterial::getSurfaceFlags);
	material.def("setSurfaceFlag", &ScriptMaterial::setSurfaceFlag);
	material.def("clearSurfaceFlag", &ScriptMaterial::clearSurfaceFlag);
    material.def("getSurfaceType", &ScriptMaterial::getSurfaceType);
    material.def("setSurfaceType", &ScriptMaterial::setSurfaceType);
    material.def("getDeformType", &ScriptMaterial::getDeformType);
    material.def("getDeformExpressionString", &ScriptMaterial::getDeformExpressionString);
    material.def("getDeformDeclName", &ScriptMaterial::getDeformDeclName);
    material.def("getSpectrum", &ScriptMaterial::getSpectrum);
    material.def("setSpectrum", &ScriptMaterial::setSpectrum);
    material.def("getDecalInfo", &ScriptMaterial::getDecalInfo);
    material.def("getCoverage", &ScriptMaterial::getCoverage);
    material.def("getLightFalloffExpressionString", &ScriptMaterial::getLightFalloffExpressionString);
    material.def("setLightFalloffExpressionFromString", &ScriptMaterial::setLightFalloffExpressionFromString);
    material.def("getLightFalloffCubeMapType", &ScriptMaterial::getLightFalloffCubeMapType);
    material.def("setLightFalloffCubeMapType", &ScriptMaterial::setLightFalloffCubeMapType);
    material.def("getGuiSurfArgument", &ScriptMaterial::getGuiSurfArgument);
    material.def("getRenderBumpArguments", &ScriptMaterial::getRenderBumpArguments);
    material.def("getRenderBumpFlatArguments", &ScriptMaterial::getRenderBumpFlatArguments);
    material.def("isModified", &ScriptMaterial::isModified);
    material.def("revertModifications", &ScriptMaterial::revertModifications);
    material.def("getAllStages", &ScriptMaterial::getAllStages);
    material.def("getNumStages", &ScriptMaterial::getNumStages);
    material.def("getStage", &ScriptMaterial::getStage);
    material.def("addStage", &ScriptMaterial::addStage);
    material.def("removeStage", &ScriptMaterial::removeStage);
    material.def("duplicateStage", &ScriptMaterial::duplicateStage);
    material.def("swapStagePosition", &ScriptMaterial::swapStagePosition);

    // Shader Stage
    stage.def(py::init<const IShaderLayer::Ptr&>());
    stage.def("getType", &ScriptMaterialStage::getType);
    stage.def("getStageFlags", &ScriptMaterialStage::getStageFlags);

	// Expose the MaterialVisitor interface

	py::class_<MaterialVisitor, MaterialVisitorWrapper> visitor(scope, "MaterialVisitor");
	visitor.def(py::init<>());
	visitor.def("visit", &MaterialVisitor::visit);

	// Add the module declaration to the given python namespace
	py::class_<ShaderSystemInterface> materialManager(scope, "MaterialManager");

	materialManager.def("foreachMaterial", &ShaderSystemInterface::foreachMaterial);
	materialManager.def("getMaterial", &ShaderSystemInterface::getMaterial);
	materialManager.def("materialExists", &ShaderSystemInterface::materialExists);
	materialManager.def("materialCanBeModified", &ShaderSystemInterface::materialCanBeModified);
	materialManager.def("createEmptyMaterial", &ShaderSystemInterface::createEmptyMaterial);
	materialManager.def("copyMaterial", &ShaderSystemInterface::copyMaterial);
	materialManager.def("renameMaterial", &ShaderSystemInterface::renameMaterial);
	materialManager.def("removeMaterial", &ShaderSystemInterface::removeMaterial);
	materialManager.def("saveMaterial", &ShaderSystemInterface::saveMaterial);

    scope.add_object("ShaderVisitor", visitor); // old compatibility name
	materialManager.def("foreachShader", &ShaderSystemInterface::foreachMaterial); // old compatibility name
	materialManager.def("getMaterialForName", &ShaderSystemInterface::getMaterial); // old compatibility name

	// Now point the Python variable "GlobalMaterialManager" to this instance
	globals["GlobalMaterialManager"] = this;
}

} // namespace script
