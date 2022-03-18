#include "GLSLBumpProgram.h"
#include "../GLProgramFactory.h"

#include "GLProgramAttributes.h"
#include "itextstream.h"
#include "igame.h"
#include "string/convert.h"
#include "debugging/gl.h"
#include "math/Matrix4.h"

namespace render
{

namespace
{
    // Lightscale registry path
    const char* LOCAL_RKEY_LIGHTSCALE = "/defaults/lightScale";

    // Filenames of shader code
    const char* BUMP_VP_FILENAME = "interaction_vp.glsl";
    const char* BUMP_FP_FILENAME = "interaction_fp.glsl";

}

// Main construction
void GLSLBumpProgram::create()
{
	// Initialise the lightScale value
    auto currentGame = GlobalGameManager().currentGame();
    auto scaleList = currentGame->getLocalXPath(LOCAL_RKEY_LIGHTSCALE);
    _lightScale = !scaleList.empty() ? string::convert<float>(scaleList[0].getContent()) : 1.0f;

    // Create the program object
    rMessage() << "[renderer] Creating GLSL bump program" << std::endl;

    _programObj = GLProgramFactory::createGLSLProgram(
        BUMP_VP_FILENAME, BUMP_FP_FILENAME
    );

    // Bind vertex attribute locations and link the program
    glBindAttribLocation(_programObj, GLProgramAttribute::Position, "attr_Position");
    glBindAttribLocation(_programObj, GLProgramAttribute::TexCoord, "attr_TexCoord");
    glBindAttribLocation(_programObj, GLProgramAttribute::Tangent, "attr_Tangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Bitangent, "attr_Bitangent");
    glBindAttribLocation(_programObj, GLProgramAttribute::Normal, "attr_Normal");
    glBindAttribLocation(_programObj, GLProgramAttribute::Colour, "attr_Colour");
    glLinkProgram(_programObj);
    debug::assertNoGlErrors();

    // Set the uniform locations to the correct bound values
    _locLightOrigin = glGetUniformLocation(_programObj, "u_light_origin");
    _locLightColour = glGetUniformLocation(_programObj, "u_light_color");
    _locViewOrigin = glGetUniformLocation(_programObj, "u_view_origin");
    _locLightScale = glGetUniformLocation(_programObj, "u_light_scale");
    _locAmbientLight = glGetUniformLocation(_programObj, "uAmbientLight");
    _locColourModulation = glGetUniformLocation(_programObj, "u_ColourModulation");
    _locColourAddition = glGetUniformLocation(_programObj, "u_ColourAddition");
    _locModelViewProjection = glGetUniformLocation(_programObj, "u_ModelViewProjection");
    _locObjectTransform = glGetUniformLocation(_programObj, "u_ObjectTransform");

    _locDiffuseTextureMatrix = glGetUniformLocation(_programObj, "u_DiffuseTextureMatrix");
    _locBumpTextureMatrix = glGetUniformLocation(_programObj, "u_BumpTextureMatrix");
    _locSpecularTextureMatrix = glGetUniformLocation(_programObj, "u_SpecularTextureMatrix");

    // Set up the texture uniforms. The renderer uses fixed texture units for
    // particular textures, so make sure they are correct here.
    // Texture 0 - diffuse
    // Texture 1 - bump
    // Texture 2 - specular
    // Texture 3 - XY attenuation map
    // Texture 4 - Z attenuation map

    glUseProgram(_programObj);
    debug::assertNoGlErrors();

    GLint samplerLoc;

    samplerLoc = glGetUniformLocation(_programObj, "u_Diffusemap");
    glUniform1i(samplerLoc, 0);

    samplerLoc = glGetUniformLocation(_programObj, "u_Bumpmap");
    glUniform1i(samplerLoc, 1);

    samplerLoc = glGetUniformLocation(_programObj, "u_Specularmap");
    glUniform1i(samplerLoc, 2);

    samplerLoc = glGetUniformLocation(_programObj, "u_attenuationmap_xy");
    glUniform1i(samplerLoc, 3);

    samplerLoc = glGetUniformLocation(_programObj, "u_attenuationmap_z");
    glUniform1i(samplerLoc, 4);

    // Light scale is constant at this point
    glUniform1f(_locLightScale, _lightScale);

    debug::assertNoGlErrors();
    glUseProgram(0);

    debug::assertNoGlErrors();
}

void GLSLBumpProgram::enable()
{
    GLSLProgramBase::enable();

    glEnableVertexAttribArray(GLProgramAttribute::Position);
    glEnableVertexAttribArray(GLProgramAttribute::TexCoord);
    glEnableVertexAttribArray(GLProgramAttribute::Tangent);
    glEnableVertexAttribArray(GLProgramAttribute::Bitangent);
    glEnableVertexAttribArray(GLProgramAttribute::Normal);
    glEnableVertexAttribArray(GLProgramAttribute::Colour);

    debug::assertNoGlErrors();
}

void GLSLBumpProgram::disable()
{
    GLSLProgramBase::disable();

    glDisableVertexAttribArray(GLProgramAttribute::Position);
    glDisableVertexAttribArray(GLProgramAttribute::TexCoord);
    glDisableVertexAttribArray(GLProgramAttribute::Tangent);
    glDisableVertexAttribArray(GLProgramAttribute::Bitangent);
    glDisableVertexAttribArray(GLProgramAttribute::Normal);
    glDisableVertexAttribArray(GLProgramAttribute::Colour);

    // Switch back to texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE0);

    debug::assertNoGlErrors();
}

void GLSLBumpProgram::setIsAmbientLight(bool isAmbientLight)
{
    glUniform1i(_locAmbientLight, isAmbientLight);
}

void GLSLBumpProgram::setLightColour(const Colour4& lightColour)
{
    glUniform3fv(_locLightColour, 1, lightColour);
}

void GLSLBumpProgram::setStageVertexColour(IShaderLayer::VertexColourMode vertexColourMode, const Colour4& stageColour)
{
    // Define the colour factors to blend into the final fragment
    switch (vertexColourMode)
    {
    case IShaderLayer::VERTEX_COLOUR_NONE:
        // Nullify the vertex colour, add the stage colour as additive constant
        glUniform4f(_locColourModulation, 0, 0, 0, 0);
        glUniform4f(_locColourAddition,
            static_cast<float>(stageColour.x()),
            static_cast<float>(stageColour.y()),
            static_cast<float>(stageColour.z()),
            static_cast<float>(stageColour.w()));
        break;

    case IShaderLayer::VERTEX_COLOUR_MULTIPLY:
        // Multiply the fragment with 1*vertexColour
        glUniform4f(_locColourModulation, 1, 1, 1, 1);
        glUniform4f(_locColourAddition, 0, 0, 0, 0);
        break;

    case IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY:
        // Multiply the fragment with (1 - vertexColour)
        glUniform4f(_locColourModulation, -1, -1, -1, -1);
        glUniform4f(_locColourAddition, 1, 1, 1, 1);
        break;
    }
}

void GLSLBumpProgram::setModelViewProjection(const Matrix4& modelViewProjection)
{
    loadMatrixUniform(_locModelViewProjection, modelViewProjection);
}

void GLSLBumpProgram::setObjectTransform(const Matrix4& transform)
{
    loadMatrixUniform(_locObjectTransform, transform);
}

void GLSLBumpProgram::setDiffuseTextureTransform(const Matrix4& transform)
{
    loadTextureMatrixUniform(_locDiffuseTextureMatrix, transform);
}

void GLSLBumpProgram::setBumpTextureTransform(const Matrix4& transform)
{
    loadTextureMatrixUniform(_locBumpTextureMatrix, transform);
}

void GLSLBumpProgram::setSpecularTextureTransform(const Matrix4& transform)
{
    loadTextureMatrixUniform(_locSpecularTextureMatrix, transform);
}

void GLSLBumpProgram::setUpLightingCalculation(const Vector3& worldLightOrigin,
    const Matrix4& worldToLight,
    const Vector3& viewer,
    const Matrix4& objectTransform,
    const Matrix4& inverseObjectTransform)
{
    debug::assertNoGlErrors();

    const auto& worldToObject = inverseObjectTransform;

    // Calculate the light origin in object space
    Vector3 localLight = worldToObject.transformPoint(worldLightOrigin);

    Matrix4 local2light(worldToLight);
    local2light.multiplyBy(objectTransform); // local->world->light

    // Calculate viewer location in object space
    auto osViewer = inverseObjectTransform.transformPoint(viewer);

    // Set lighting parameters in the shader
    glUniform3f(_locViewOrigin,
        static_cast<float>(osViewer.x()),
        static_cast<float>(osViewer.y()),
        static_cast<float>(osViewer.z())
    );
    glUniform3f(_locLightOrigin,
        static_cast<float>(localLight.x()),
        static_cast<float>(localLight.y()),
        static_cast<float>(localLight.z())
    );
#if 0
    glUniform3fv(_locLightColour, 1, parms.lightColour);
    glUniform1f(_locLightScale, _lightScale);
    glUniform1i(_locAmbientLight, parms.isAmbientLight);

    // Define the colour factors to blend into the final fragment
    switch (parms.vertexColourMode)
    {
    case IShaderLayer::VERTEX_COLOUR_NONE:
        // Nullify the vertex colour, add the stage colour as additive constant
        glUniform4f(_locColourModulation, 0, 0, 0, 0);
        glUniform4f(_locColourAddition,
            static_cast<float>(parms.stageColour.x()),
            static_cast<float>(parms.stageColour.y()),
            static_cast<float>(parms.stageColour.z()),
            static_cast<float>(parms.stageColour.w()));
        break;

    case IShaderLayer::VERTEX_COLOUR_MULTIPLY:
        // Multiply the fragment with 1*vertexColour
        glUniform4f(_locColourModulation, 1, 1, 1, 1);
        glUniform4f(_locColourAddition, 0, 0, 0, 0);
        break;

    case IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY:
        // Multiply the fragment with (1 - vertexColour)
        glUniform4f(_locColourModulation, -1, -1, -1, -1);
        glUniform4f(_locColourAddition, 1, 1, 1, 1);
        break;
    }
#endif

    glActiveTexture(GL_TEXTURE3);
    glClientActiveTexture(GL_TEXTURE3);

    glMatrixMode(GL_TEXTURE);
    glLoadMatrixd(local2light);
    glMatrixMode(GL_MODELVIEW);

    debug::assertNoGlErrors();
}

}




