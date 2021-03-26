#include "RadiantTest.h"

#include "ishaders.h"
#include "string/trim.h"
#include "materials/ParseLib.h"
#include <fmt/format.h>

namespace test
{

using MaterialExportTest = RadiantTest;

inline void expectDefinitionContains(const MaterialPtr& material, const std::string& expectedContainedString)
{
    EXPECT_NE(material->getDefinition().find(expectedContainedString), std::string::npos) 
        << "Material definition doesn't contain " << expectedContainedString << " as expected.\n" 
        << "Definition was: \n" << material->getDefinition();
}

inline void expectDefinitionDoesNotContain(const MaterialPtr& material, const std::string& unexpectedString)
{
    EXPECT_EQ(material->getDefinition().find(unexpectedString), std::string::npos)
        << "Material definition contains " << unexpectedString << " but that shouldn't be the case.\n"
        << "Definition was: \n" << material->getDefinition();
}

inline void expectDefinitionDoesNotContainAnyOf(const MaterialPtr& material, const std::vector<std::string>& unexpectedStrings)
{
    for (const auto& unexpectedString : unexpectedStrings)
    {
        expectDefinitionDoesNotContain(material, unexpectedString);
    }
}

TEST_F(MaterialExportTest, Description)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    constexpr auto description = "testdescription, with commas, spaces and extra";
    material->setDescription(description);

    expectDefinitionContains(material, fmt::format("description \"{0}\"", description));

    constexpr auto doubleQuoted = "testdescription with \"quotes\"";
    constexpr auto singleQuoted = "testdescription with 'quotes'";
    material->setDescription(doubleQuoted);

    expectDefinitionContains(material, fmt::format("description \"{0}\"", singleQuoted));
}

TEST_F(MaterialExportTest, PolygonOffset)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    material->setPolygonOffset(0.0);
    expectDefinitionContains(material, fmt::format("polygonOffset {0}", 0.0));

    material->setPolygonOffset(-1.5);
    expectDefinitionContains(material, fmt::format("polygonOffset {0}", -1.5));

    material->setPolygonOffset(+1.5);
    expectDefinitionContains(material, fmt::format("polygonOffset {0}", 1.5));

    material->clearMaterialFlag(Material::FLAG_POLYGONOFFSET);
    expectDefinitionDoesNotContain(material, "polygonOffset");
}

TEST_F(MaterialExportTest, SurfaceType)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    for (const auto& pair : shaders::SurfaceTypeMapping)
    {
        material->setSurfaceType(pair.second);
        expectDefinitionContains(material, pair.first);
    }

    // Test resetting the surface type to default which should clear the type
    auto lastSurfaceType = shaders::getStringForSurfaceType(material->getSurfaceType());
    EXPECT_NE(lastSurfaceType, std::string());

    material->setSurfaceType(Material::SURFTYPE_DEFAULT);
    expectDefinitionDoesNotContain(material, "lastSurfaceType");
}

TEST_F(MaterialExportTest, MaterialFlags)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    for (const auto& pair : shaders::MaterialFlagKeywords)
    {
        material->setMaterialFlag(pair.second);
        expectDefinitionContains(material, pair.first);

        material->clearMaterialFlag(pair.second);
        expectDefinitionDoesNotContain(material, pair.first);
    }
}

TEST_F(MaterialExportTest, ClampType)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    material->setClampType(CLAMP_NOREPEAT);
    expectDefinitionContains(material, "clamp");

    material->setClampType(CLAMP_ZEROCLAMP);
    expectDefinitionContains(material, "zeroclamp");

    material->setClampType(CLAMP_ALPHAZEROCLAMP);
    expectDefinitionContains(material, "alphazeroclamp");

    material->setClampType(CLAMP_REPEAT); // this is the default => no keyword necessary
    expectDefinitionDoesNotContainAnyOf(material, { "clamp", "zeroclamp", "alphazeroclamp" });
}

TEST_F(MaterialExportTest, CullType)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    material->setCullType(Material::CULL_FRONT);
    expectDefinitionContains(material, "backsided");

    material->setCullType(Material::CULL_NONE);
    expectDefinitionContains(material, "twosided");

    material->setCullType(Material::CULL_BACK); // This is the default
    expectDefinitionDoesNotContainAnyOf(material, { "twosided", "backsided" });
}

TEST_F(MaterialExportTest, GuiSurf)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/guisurf1");
    expectDefinitionContains(material, "guiSurf\tguis/lvlmaps/genericmap.gui");

    // Mark the definition as modified by setting the description
    material->setDescription("-");
    material->setDescription("");

    expectDefinitionContains(material, "guisurf guis/lvlmaps/genericmap.gui");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/guisurf2");
    expectDefinitionContains(material, "guiSurf\tentity");

    // Mark the definition as modified by setting the description
    material->setDescription("-");
    material->setDescription("");

    expectDefinitionContains(material, "guisurf entity");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/guisurf3");
    expectDefinitionContains(material, "guiSurf\tentity2");

    // Mark the definition as modified by setting the description
    material->setDescription("-");
    material->setDescription("");

    expectDefinitionContains(material, "guisurf entity2");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/guisurf4");
    expectDefinitionContains(material, "guiSurf\tentity3");

    // Mark the definition as modified by setting the description
    material->setDescription("-");
    material->setDescription("");

    expectDefinitionContains(material, "guisurf entity3");
}

TEST_F(MaterialExportTest, Sort)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    material->setSortRequest(-1.2f);
    expectDefinitionContains(material, "sort -1.2");

    // Using pre-defined sort values should produce the corresponding string, like "subview"
    for (const auto& pair : shaders::PredefinedSortValues)
    {
        material->setSortRequest(static_cast<float>(pair.second));
        expectDefinitionContains(material, fmt::format("sort {0}", pair.first));
    }

    material->clearMaterialFlag(Material::FLAG_HAS_SORT_DEFINED);
    expectDefinitionDoesNotContain(material, "sort");
}

TEST_F(MaterialExportTest, Spectrum)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    for (int i = -50; i < 50; ++i)
    {
        material->setSpectrum(i);

        if (i != 0)
        {
            expectDefinitionContains(material, fmt::format("spectrum {0}", i));
        }
        else // spectrum 0 is the default, doesn't need to be declared
        {
            expectDefinitionDoesNotContain(material, "spectrum");
        }
    }
}

TEST_F(MaterialExportTest, Deform)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/deform1");
    expectDefinitionContains(material, "deform");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "deform flare 1.5");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/deform2");
    expectDefinitionContains(material, "deform");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "deform expand (0.1 * deformexporttesttable[time * (0.3 + time)] - global3)");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/deform3");
    expectDefinitionContains(material, "deform");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "deform move (1.7 + time + 4.0 - global3)");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/deform4");
    expectDefinitionContains(material, "deform");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "deform turbulent deformexporttesttable time * 2.0 (parm11 - 4.0) -1.0 * global5");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/deform5");
    expectDefinitionContains(material, "deform");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "deform particle testparticle");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/deform6");
    expectDefinitionContains(material, "deform");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "deform particle2 testparticle");
}

TEST_F(MaterialExportTest, DecalInfo)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/decalinfo");
    expectDefinitionContains(material, "decalinfo");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "decalinfo 14.3 1.5 ( 0.9 0.8 0.7 0.6 ) ( 0.5 0.5 0.4 0.3 )");
}

TEST_F(MaterialExportTest, RenderBump)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/renderBump1");
    expectDefinitionContains(material, "renderBump");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "renderbump textures/output.tga models/hipoly");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/renderBump2");
    expectDefinitionContains(material, "renderBump");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "renderbump -size 100 200 textures/output.tga models/hipoly");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/renderBump3");
    expectDefinitionContains(material, "renderBump");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "renderbump -aa 2 textures/output.tga models/hipoly");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/renderBump4");
    expectDefinitionContains(material, "renderBump");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "renderbump -aa 2 -size 10 10 textures/output.tga models/hipoly");
}

TEST_F(MaterialExportTest, RenderBumpFlat)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/renderBumpFlat1");
    expectDefinitionContains(material, "renderBumpflat");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "renderbumpflat -size 200 100 models/hipoly");

    material = GlobalMaterialManager().getMaterial("textures/exporttest/renderBumpFlat2");
    expectDefinitionContains(material, "renderBumpflat");

    // Mark the definition as modified by setting the description
    material->setDescription("-");

    expectDefinitionContains(material, "renderbumpflat models/hipoly");
}

TEST_F(MaterialExportTest, LightFlags)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    material->setIsAmbientLight(true);
    expectDefinitionContains(material, "ambientLight");
    material->setIsAmbientLight(false);
    expectDefinitionDoesNotContain(material, "ambientLight");

    material->setIsBlendLight(true);
    expectDefinitionContains(material, "blendLight");
    material->setIsBlendLight(false);
    expectDefinitionDoesNotContain(material, "blendLight");

    material->setIsFogLight(true);
    expectDefinitionContains(material, "fogLight");
    material->setIsFogLight(false);
    expectDefinitionDoesNotContain(material, "fogLight");

    material->setIsCubicLight(true);
    expectDefinitionContains(material, "cubicLight");
    material->setIsCubicLight(false);
    expectDefinitionDoesNotContain(material, "cubicLight");

    material->setIsCubicLight(true);
    material->setIsAmbientLight(true);
    expectDefinitionContains(material, "ambientCubicLight");

    material->setIsAmbientLight(false);
    material->setIsCubicLight(false);
    expectDefinitionDoesNotContain(material, "ambientCubicLight");
}

TEST_F(MaterialExportTest, LightFalloffImage)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    material->setLightFalloffCubeMapType(IShaderLayer::MapType::Map);
    material->setLightFalloffExpressionFromString("makeintensity(lights/standard)");
    expectDefinitionContains(material, "lightFalloffImage makeIntensity(lights/standard)");

    material->setLightFalloffExpressionFromString("");
    expectDefinitionDoesNotContain(material, "lightFalloffImage");

    material->setLightFalloffCubeMapType(IShaderLayer::MapType::CameraCubeMap);
    material->setLightFalloffExpressionFromString("env/standard");
    expectDefinitionContains(material, "lightFalloffCubeMap env/standard");
}

TEST_F(MaterialExportTest, SurfaceFlags)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    for (const auto& pair : shaders::SurfaceFlags)
    {
        material->setSurfaceFlag(pair.second);
        expectDefinitionContains(material, pair.first);

        material->clearSurfaceFlag(pair.second);
        expectDefinitionDoesNotContain(material, pair.first);
    }
}

TEST_F(MaterialExportTest, StageBlendTypes)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    auto layer = material->getEditableLayer(material->addLayer(IShaderLayer::DIFFUSE));
    layer->setMapExpressionFromString("_white");

    // one custom option to prevent cutting the stage down to "diffusemap _white"
    layer->setStageFlag(IShaderLayer::FLAG_MASK_GREEN); 

    expectDefinitionContains(material, "blend diffusemap");
    expectDefinitionContains(material, "map _white");

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BUMP));
    layer->setMapExpressionFromString("_flat");
    layer->setStageFlag(IShaderLayer::FLAG_MASK_GREEN);

    expectDefinitionContains(material, "blend bumpmap");
    expectDefinitionContains(material, "map _flat");

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::SPECULAR));
    layer->setMapExpressionFromString("_black");
    layer->setStageFlag(IShaderLayer::FLAG_MASK_GREEN);

    expectDefinitionContains(material, "blend specularmap");
    expectDefinitionContains(material, "map _black");

    // Test that the shortcuts get preserved
    for (const auto& testCase : shaders::BlendTypeShortcuts)
    {
        material->revertModifications();

        layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
        layer->setBlendFuncStrings(std::make_pair(testCase.first, ""));
        layer->setMapExpressionFromString("_black");
        layer->setStageFlag(IShaderLayer::FLAG_MASK_GREEN);

        expectDefinitionContains(material, fmt::format("blend {0}", testCase.first));
        expectDefinitionDoesNotContain(material, fmt::format("blend {0},", testCase.first));
    }

    // Test custom blend funcs
    for (const auto& testCase : shaders::BlendTypeShortcuts)
    {
        material->revertModifications();

        layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
        layer->setBlendFuncStrings(testCase.second);
        layer->setMapExpressionFromString("_black");
        layer->setStageFlag(IShaderLayer::FLAG_MASK_GREEN);

        expectDefinitionContains(material, fmt::format("blend {0}, {1}", testCase.second.first, testCase.second.second));
    }
}

TEST_F(MaterialExportTest, StageMaps)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    auto layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setMapType(IShaderLayer::MapType::CubeMap);
    layer->setMapExpressionFromString("env/shot");
    expectDefinitionContains(material, "cubeMap env/shot");

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setMapType(IShaderLayer::MapType::CameraCubeMap);
    layer->setMapExpressionFromString("env/shot");
    expectDefinitionContains(material, "cameraCubeMap env/shot");

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setMapType(IShaderLayer::MapType::MirrorRenderMap);
    layer->setRenderMapSize(Vector2(512, 256));
    expectDefinitionContains(material, "mirrorRenderMap 512, 256");

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setMapType(IShaderLayer::MapType::RemoteRenderMap);
    layer->setRenderMapSize(Vector2(512, 256));
    expectDefinitionContains(material, "remoteRenderMap 512, 256");

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setMapType(IShaderLayer::MapType::VideoMap);
    layer->setVideoMapProperties("guis/videos/test", false);
    expectDefinitionContains(material, "videoMap guis/videos/test");

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setMapType(IShaderLayer::MapType::VideoMap);
    layer->setVideoMapProperties("guis/videos/test", true);
    expectDefinitionContains(material, "videoMap loop guis/videos/test");

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setMapType(IShaderLayer::MapType::SoundMap);
    layer->setSoundMapWaveForm(false);
    expectDefinitionContains(material, "soundMap");
    expectDefinitionDoesNotContain(material, "waveform");

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setMapType(IShaderLayer::MapType::SoundMap);
    layer->setSoundMapWaveForm(true);
    expectDefinitionContains(material, "soundMap waveform");
}

TEST_F(MaterialExportTest, TextureFilter)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    auto layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setStageFlag(IShaderLayer::FLAG_FILTER_NEAREST);
    expectDefinitionContains(material, "nearest");
    expectDefinitionDoesNotContain(material, "linear");

    layer->clearStageFlag(IShaderLayer::FLAG_FILTER_NEAREST);
    expectDefinitionDoesNotContainAnyOf(material, { "nearest", "linear" });

    layer->setStageFlag(IShaderLayer::FLAG_FILTER_LINEAR);
    expectDefinitionContains(material, "linear");
    expectDefinitionDoesNotContain(material, "nearest");

    layer->clearStageFlag(IShaderLayer::FLAG_FILTER_LINEAR);
    expectDefinitionDoesNotContainAnyOf(material, { "nearest", "linear" });
}

TEST_F(MaterialExportTest, TextureQuality)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    auto layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setStageFlag(IShaderLayer::FLAG_HIGHQUALITY);
    expectDefinitionContains(material, "highQuality");
    expectDefinitionDoesNotContain(material, "uncompressed");

    layer->clearStageFlag(IShaderLayer::FLAG_HIGHQUALITY);
    expectDefinitionDoesNotContain(material, "highQuality");
    expectDefinitionDoesNotContain(material, "uncompressed");

    layer->setStageFlag(IShaderLayer::FLAG_FORCE_HIGHQUALITY);
    expectDefinitionContains(material, "forceHighQuality");
    expectDefinitionDoesNotContain(material, "highQuality");

    layer->clearStageFlag(IShaderLayer::FLAG_FORCE_HIGHQUALITY);
    expectDefinitionDoesNotContain(material, "forceHighQuality");

    layer->setStageFlag(IShaderLayer::FLAG_NO_PICMIP);
    expectDefinitionContains(material, "nopicmip");

    layer->clearStageFlag(IShaderLayer::FLAG_NO_PICMIP);
    expectDefinitionDoesNotContain(material, "nopicmip");
}

TEST_F(MaterialExportTest, TexGen)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    auto layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setTexGenType(IShaderLayer::TEXGEN_NORMAL);
    expectDefinitionDoesNotContain(material, "texgen");

    layer->setTexGenType(IShaderLayer::TEXGEN_REFLECT);
    expectDefinitionContains(material, "texgen reflect");

    layer->setTexGenType(IShaderLayer::TEXGEN_SKYBOX);
    expectDefinitionContains(material, "texgen skybox");

    layer->setTexGenType(IShaderLayer::TEXGEN_WOBBLESKY);
    layer->setTexGenExpressionFromString(0, "1");
    layer->setTexGenExpressionFromString(1, "0.5");
    layer->setTexGenExpressionFromString(2, "(time * 0.6)");
    expectDefinitionContains(material, "texgen wobblesky 1.0 0.5 (time * 0.6)");
}

TEST_F(MaterialExportTest, StageClamp)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    // Material has default (CLAMP_REPEAT)
    auto layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setClampType(CLAMP_REPEAT);
    expectDefinitionDoesNotContain(material, "noclamp"); // not needed on the stage level

    // Set the material to no-repeat
    material->setClampType(CLAMP_NOREPEAT);

    layer->setClampType(CLAMP_REPEAT);
    expectDefinitionContains(material, "noclamp"); // noclamp is necessary

    layer->setClampType(CLAMP_NOREPEAT);
    expectDefinitionContains(material, "clamp");

    layer->setClampType(CLAMP_ZEROCLAMP);
    expectDefinitionContains(material, "zeroclamp");

    layer->setClampType(CLAMP_ALPHAZEROCLAMP);
    expectDefinitionContains(material, "alphazeroclamp");
}

TEST_F(MaterialExportTest, StageFlags)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    auto layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setStageFlag(IShaderLayer::FLAG_IGNORE_ALPHATEST);
    expectDefinitionContains(material, "ignoreAlphaTest");

    layer->clearStageFlag(IShaderLayer::FLAG_IGNORE_ALPHATEST);
    expectDefinitionDoesNotContain(material, "ignoreAlphaTest");

    layer->setStageFlag(IShaderLayer::FLAG_IGNORE_DEPTH);
    expectDefinitionContains(material, "ignoreDepth");

    layer->clearStageFlag(IShaderLayer::FLAG_IGNORE_DEPTH);
    expectDefinitionDoesNotContain(material, "ignoreDepth");

    layer->setStageFlag(IShaderLayer::FLAG_MASK_RED);
    expectDefinitionContains(material, "maskRed");

    layer->clearStageFlag(IShaderLayer::FLAG_MASK_RED);
    expectDefinitionDoesNotContain(material, "maskRed");

    layer->setStageFlag(IShaderLayer::FLAG_MASK_GREEN);
    expectDefinitionContains(material, "maskGreen");

    layer->clearStageFlag(IShaderLayer::FLAG_MASK_GREEN);
    expectDefinitionDoesNotContain(material, "maskGreen");

    layer->setStageFlag(IShaderLayer::FLAG_MASK_BLUE);
    expectDefinitionContains(material, "maskBlue");

    layer->clearStageFlag(IShaderLayer::FLAG_MASK_BLUE);
    expectDefinitionDoesNotContain(material, "maskBlue");

    layer->setStageFlag(IShaderLayer::FLAG_MASK_ALPHA);
    expectDefinitionContains(material, "maskAlpha");

    layer->clearStageFlag(IShaderLayer::FLAG_MASK_ALPHA);
    expectDefinitionDoesNotContain(material, "maskAlpha");

    layer->setStageFlag(IShaderLayer::FLAG_MASK_DEPTH);
    expectDefinitionContains(material, "maskDepth");

    layer->clearStageFlag(IShaderLayer::FLAG_MASK_DEPTH);
    expectDefinitionDoesNotContain(material, "maskDepth");

    layer->setStageFlag(IShaderLayer::FLAG_MASK_RED);
    layer->setStageFlag(IShaderLayer::FLAG_MASK_GREEN);
    layer->setStageFlag(IShaderLayer::FLAG_MASK_BLUE);
    expectDefinitionContains(material, "maskColor");
    expectDefinitionDoesNotContainAnyOf(material, { "maskRed", "maskGreen", "maskBlue" });

    layer->setStageFlag(IShaderLayer::FLAG_MASK_RED);
    layer->setStageFlag(IShaderLayer::FLAG_MASK_GREEN);
    layer->setStageFlag(IShaderLayer::FLAG_MASK_BLUE);
    layer->setStageFlag(IShaderLayer::FLAG_MASK_ALPHA);
    layer->setStageFlag(IShaderLayer::FLAG_MASK_DEPTH);
    expectDefinitionContains(material, "maskColor");
    expectDefinitionDoesNotContainAnyOf(material, { "maskRed", "maskGreen", "maskBlue" });
    expectDefinitionContains(material, "maskAlpha");
    expectDefinitionContains(material, "maskDepth");
}

TEST_F(MaterialExportTest, StageVertexColours)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    auto layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setColourExpressionFromString(IShaderLayer::COMP_RED, "time * 0.1");
    expectDefinitionContains(material, "red time * 0.1");
    expectDefinitionDoesNotContainAnyOf(material, { "blue", "green", "alpha", "colored", "color", "rgb ", "rgba" });

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setColourExpressionFromString(IShaderLayer::COMP_BLUE, "time * 0.1");
    expectDefinitionContains(material, "blue time * 0.1");
    expectDefinitionDoesNotContainAnyOf(material, { "red", "green", "alpha", "colored", "color", "rgb ", "rgba" });

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setColourExpressionFromString(IShaderLayer::COMP_GREEN, "time * 0.1");
    expectDefinitionContains(material, "green time * 0.1");
    expectDefinitionDoesNotContainAnyOf(material, { "red", "blue", "alpha", "colored", "color", "rgb ", "rgba" });

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setColourExpressionFromString(IShaderLayer::COMP_ALPHA, "time * 0.1");
    expectDefinitionContains(material, "alpha time * 0.1");
    expectDefinitionDoesNotContainAnyOf(material, { "red", "green", "blue", "colored", "color", "rgb ", "rgba" });

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setColourExpressionFromString(IShaderLayer::COMP_RED, "time * 0.1");
    layer->setColourExpressionFromString(IShaderLayer::COMP_GREEN, "time * 7");
    expectDefinitionContains(material, "red time * 0.1");
    expectDefinitionContains(material, "green time * 7.0");
    expectDefinitionDoesNotContainAnyOf(material, { "blue", "alpha", "colored", "color", "rgb ", "rgba" });

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setColourExpressionFromString(IShaderLayer::COMP_RED, "time * 0.1");
    layer->setColourExpressionFromString(IShaderLayer::COMP_GREEN, "time * 0.1");
    layer->setColourExpressionFromString(IShaderLayer::COMP_BLUE, "time * 0.1");
    layer->setColourExpressionFromString(IShaderLayer::COMP_ALPHA, "7");
    expectDefinitionContains(material, "rgb time * 0.1");
    expectDefinitionContains(material, "alpha 7.0");
    expectDefinitionDoesNotContainAnyOf(material, { "red", "green", "blue", "colored", "color", "rgba" });

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setColourExpressionFromString(IShaderLayer::COMP_RED, "time * 0.1");
    layer->setColourExpressionFromString(IShaderLayer::COMP_GREEN, "time * 0.1");
    layer->setColourExpressionFromString(IShaderLayer::COMP_BLUE, "time * 0.1");
    layer->setColourExpressionFromString(IShaderLayer::COMP_ALPHA, "time * 0.1");
    expectDefinitionContains(material, "rgba time * 0.1");
    // Note: we use a space after "rgb " because a valid RGBA keyword also contains "RGB"
    expectDefinitionDoesNotContainAnyOf(material, { "red", "green", "blue", "alpha", "colored", "color", "rgb " });

    material->revertModifications();

    // Recognise colored
    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setColourExpressionFromString(IShaderLayer::COMP_RED, "parm0");
    layer->setColourExpressionFromString(IShaderLayer::COMP_GREEN, "parm1");
    layer->setColourExpressionFromString(IShaderLayer::COMP_BLUE, "parm2");
    layer->setColourExpressionFromString(IShaderLayer::COMP_ALPHA, "parm3");
    expectDefinitionContains(material, "colored");
    // Note: we use a space after "red " because the colored keyword also contains "red", some for "color "
    expectDefinitionDoesNotContainAnyOf(material, { "red ", "green", "blue", "alpha", "color ", "rgb ", "rgba" });

    material->revertModifications();

    // Make use of color shortcut
    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setColourExpressionFromString(IShaderLayer::COMP_RED, "time * 0.1");
    layer->setColourExpressionFromString(IShaderLayer::COMP_GREEN, "2");
    layer->setColourExpressionFromString(IShaderLayer::COMP_BLUE, "parm2");
    layer->setColourExpressionFromString(IShaderLayer::COMP_ALPHA, "parm3");
    expectDefinitionContains(material, "color time * 0.1, 2.0, parm2, parm3");
    expectDefinitionDoesNotContainAnyOf(material, { "red", "green", "blue", "alpha", "colored", "rgb ", "rgba" });

    material->revertModifications();

    // Make use of color shortcut
    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setVertexColourMode(IShaderLayer::VERTEX_COLOUR_MULTIPLY);
    expectDefinitionContains(material, "vertexColor");

    layer->setVertexColourMode(IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY);
    expectDefinitionContains(material, "inverseVertexColor");
    
    layer->setVertexColourMode(IShaderLayer::VERTEX_COLOUR_NONE);
    expectDefinitionDoesNotContainAnyOf(material, { "vertexColor", "inverseVertexColor" });
}

TEST_F(MaterialExportTest, StagePrivatePolygonOffset)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    auto layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setPrivatePolygonOffset(0.1);
    expectDefinitionContains(material, "privatePolygonOffset 0.1");

    layer->setPrivatePolygonOffset(-15.7);
    expectDefinitionContains(material, "privatePolygonOffset -15.7");

    layer->setPrivatePolygonOffset(0);
    expectDefinitionDoesNotContain(material, "privatePolygonOffset");
}

TEST_F(MaterialExportTest, StageTransforms)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    auto layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->appendTransformation(IShaderLayer::Transformation{ IShaderLayer::TransformType::Translate });
    layer->updateTransformation(0, IShaderLayer::TransformType::Translate, "time * 0.5", "sinTable[7.6]");

    expectDefinitionContains(material, "translate time * 0.5, sinTable[7.6]");
    expectDefinitionDoesNotContainAnyOf(material, { "rotate", "scroll", "scale", "shear", "centerScale" });

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->appendTransformation(IShaderLayer::Transformation{ IShaderLayer::TransformType::CenterScale });
    layer->updateTransformation(0, IShaderLayer::TransformType::CenterScale, "0.4", "time");

    expectDefinitionContains(material, "centerScale 0.4, time");
    expectDefinitionDoesNotContainAnyOf(material, { "rotate", "scroll", "scale", "shear", "translate" });

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->appendTransformation(IShaderLayer::Transformation{ IShaderLayer::TransformType::Rotate });
    layer->updateTransformation(0, IShaderLayer::TransformType::Rotate, "time", "");

    expectDefinitionContains(material, "rotate time");
    expectDefinitionDoesNotContainAnyOf(material, { "centerScale", "scroll", "scale", "shear", "translate" });

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->appendTransformation(IShaderLayer::Transformation{ IShaderLayer::TransformType::Scale });
    layer->updateTransformation(0, IShaderLayer::TransformType::Scale, "time", "time % 4");

    expectDefinitionContains(material, "scale time, time % 4.0");
    expectDefinitionDoesNotContainAnyOf(material, { "centerScale", "scroll", "rotate", "shear", "translate" });

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->appendTransformation(IShaderLayer::Transformation{ IShaderLayer::TransformType::Shear });
    layer->updateTransformation(0, IShaderLayer::TransformType::Shear, "time / 6", "global4");

    expectDefinitionContains(material, "shear time / 6.0, global4");
    expectDefinitionDoesNotContainAnyOf(material, { "centerScale", "scroll", "rotate", "scale", "translate" });

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->appendTransformation(IShaderLayer::Transformation{ IShaderLayer::TransformType::Translate });
    layer->updateTransformation(0, IShaderLayer::TransformType::Translate, "1", "2");
    layer->appendTransformation(IShaderLayer::Transformation{ IShaderLayer::TransformType::Scale });
    layer->updateTransformation(1, IShaderLayer::TransformType::Scale, "1", "1");
    layer->appendTransformation(IShaderLayer::Transformation{ IShaderLayer::TransformType::Translate });
    layer->updateTransformation(2, IShaderLayer::TransformType::Translate, "time", "1");
    layer->appendTransformation(IShaderLayer::Transformation{ IShaderLayer::TransformType::Rotate });
    layer->updateTransformation(3, IShaderLayer::TransformType::Rotate, "time", "");

    expectDefinitionContains(material, "translate 1.0, 2.0");
    expectDefinitionContains(material, "scale 1.0, 1.0");
    expectDefinitionContains(material, "translate time, 1.0");
    expectDefinitionContains(material, "rotate time");
    expectDefinitionDoesNotContainAnyOf(material, { "shear", "centerScale", "scroll" });
}

TEST_F(MaterialExportTest, StageAlphaTest)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    auto layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setAlphaTestExpressionFromString("sinTable[time]");
    expectDefinitionContains(material, "alphaTest sinTable[time]"); 

    material->revertModifications();

    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setAlphaTestExpressionFromString("0.775");
    expectDefinitionContains(material, "alphaTest 0.775");

    material->revertModifications();
    
    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setAlphaTestExpressionFromString("");
    expectDefinitionDoesNotContain(material, "alphaTest");
}

TEST_F(MaterialExportTest, StageCondition)
{
    auto material = GlobalMaterialManager().getMaterial("textures/exporttest/empty");

    EXPECT_EQ(string::trim_copy(material->getDefinition()), "");

    // Add condition without parentheses
    auto layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setConditionExpressionFromString("parm5 > 3");
    expectDefinitionContains(material, "if (parm5 > 3.0)");

    material->revertModifications();

    // Add condition with parentheses
    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setConditionExpressionFromString("(parm5 > 3");
    expectDefinitionContains(material, "if (parm5 > 3.0)");

    material->revertModifications();

    // Clear condition
    layer = material->getEditableLayer(material->addLayer(IShaderLayer::BLEND));
    layer->setConditionExpressionFromString("");
    expectDefinitionDoesNotContain(material, "if");
}

}
