////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "SimpleWMSBody.hpp"

#include "../../../src/cs-core/GraphicsEngine.hpp"
#include "../../../src/cs-graphics/TextureLoader.hpp"
#include "../../../src/cs-utils/FrameTimings.hpp"
#include "../../../src/cs-utils/filesystem.hpp"
#include "../../../src/cs-utils/utils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>

namespace csp::simpleWmsBodies {

////////////////////////////////////////////////////////////////////////////////////////////////////

const uint32_t GRID_RESOLUTION_X = 200;
const uint32_t GRID_RESOLUTION_Y = 100;

////////////////////////////////////////////////////////////////////////////////////////////////////

const std::string SimpleWMSBody::SPHERE_VERT = R"(
uniform vec3 uSunDirection;
uniform vec3 uRadii;
uniform mat4 uMatModelView;
uniform mat4 uMatProjection;

// inputs
layout(location = 0) in vec2 iGridPos;

// outputs
out vec2 vTexCoords;
out vec3 vPosition;
out vec3 vCenter;
out vec2 vLonLat;

const float PI = 3.141592654;

void main()
{
    vTexCoords = vec2(iGridPos.x, 1-iGridPos.y);
    vLonLat.x = iGridPos.x * 2.0 * PI;
    vLonLat.y = (iGridPos.y-0.5) * PI;
    vPosition = uRadii * vec3(
        -sin(vLonLat.x) * cos(vLonLat.y),
        -cos(vLonLat.y+PI*0.5),
        -cos(vLonLat.x) * cos(vLonLat.y)
    );
    vPosition   = (uMatModelView * vec4(vPosition, 1.0)).xyz;
    vCenter     = (uMatModelView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    gl_Position =  uMatProjection * vec4(vPosition, 1);

    if (gl_Position.w > 0) {
     gl_Position /= gl_Position.w;
     if (gl_Position.z >= 1) {
       gl_Position.z = 0.999999;
     }
    }
}
)";

////////////////////////////////////////////////////////////////////////////////////////////////////

const std::string SimpleWMSBody::SPHERE_FRAG = R"(
uniform vec3 uSunDirection;
uniform sampler2D uSurfaceTexture;
uniform sampler2D uSecondSurfaceTexture;
uniform sampler2D uDefaultTexture;
uniform float uAmbientBrightness;
uniform float uSunIlluminance;
uniform float uFarClip;
uniform float uFade;
uniform bool uSecondTexture;

// inputs
in vec2 vTexCoords;
in vec3 vSunDirection;
in vec3 vPosition;
in vec3 vCenter;
in vec2 vLonLat;

// outputs
layout(location = 0) out vec3 oColor;

vec3 SRGBtoLINEAR(vec3 srgbIn)
{
  vec3 bLess = step(vec3(0.04045),srgbIn);
  return mix( srgbIn/vec3(12.92), pow((srgbIn+vec3(0.055))/vec3(1.055),vec3(2.4)), bLess );
}

void main()
{
    // WMS texture
    vec4 texColor = texture(uSurfaceTexture, vTexCoords);
    // Default planet surface texture
    vec3 defColor = texture(uDefaultTexture, vTexCoords).rgb;
    oColor = mix(defColor, texColor.rgb, texColor.a); 
    // Fade texture in.
    if(uSecondTexture) {
      vec4 secColorA = texture(uSecondSurfaceTexture, vTexCoords);
      vec3 secColor = mix(defColor, secColorA.rgb, secColorA.a);
      oColor = mix(secColor, oColor, uFade);
    }

    #ifdef ENABLE_HDR
      oColor = SRGBtoLINEAR(oColor);
    #endif

    oColor = oColor * uSunIlluminance;

    #ifdef ENABLE_LIGHTING
      vec3 normal = normalize(vPosition - vCenter);
      float light = max(dot(normal, uSunDirection), 0.0);
      oColor = mix(oColor*uAmbientBrightness, oColor, light);
    #endif

    gl_FragDepth = length(vPosition) / uFarClip;
}
)";

////////////////////////////////////////////////////////////////////////////////////////////////////

SimpleWMSBody::SimpleWMSBody(std::shared_ptr<cs::core::GraphicsEngine> const& graphicsEngine,
    std::shared_ptr<cs::core::SolarSystem> const& solarSystem, std::string const& sCenterName,
    std::string sTexture, std::string const& sFrameName, double tStartExistence,
    double tEndExistence, std::vector<Wms> tWms, std::shared_ptr<cs::core::TimeControl> timeControl,
    std::shared_ptr<Properties> properties)
    : cs::scene::CelestialBody(sCenterName, sFrameName, tStartExistence, tEndExistence)
    , mGraphicsEngine(graphicsEngine)
    , mSolarSystem(solarSystem)
    , mRadii(cs::core::SolarSystem::getRadii(sCenterName))
    , mWmsTexture(new VistaTexture(GL_TEXTURE_2D))
    , mDefaultTexture(cs::graphics::TextureLoader::loadFromFile(sTexture))
    , mOtherTexture(new VistaTexture(GL_TEXTURE_2D)) {
  pVisibleRadius      = mRadii[0];
  mTimeControl        = timeControl;
  mProperties         = properties;
  mWms                = tWms;
  mDefaultTextureFile = sTexture;

  setActiveWms(mWms.at(0));

  // For rendering the sphere, we create a 2D-grid which is warped into a sphere in the vertex
  // shader. The vertex positions are directly used as texture coordinates.
  std::vector<float>    vertices(GRID_RESOLUTION_X * GRID_RESOLUTION_Y * 2);
  std::vector<unsigned> indices((GRID_RESOLUTION_X - 1) * (2 + 2 * GRID_RESOLUTION_Y));

  for (uint32_t x = 0; x < GRID_RESOLUTION_X; ++x) {
    for (uint32_t y = 0; y < GRID_RESOLUTION_Y; ++y) {
      vertices[(x * GRID_RESOLUTION_Y + y) * 2 + 0] = 1.f / (GRID_RESOLUTION_X - 1) * x;
      vertices[(x * GRID_RESOLUTION_Y + y) * 2 + 1] = 1.f / (GRID_RESOLUTION_Y - 1) * y;
    }
  }

  uint32_t index = 0;

  for (uint32_t x = 0; x < GRID_RESOLUTION_X - 1; ++x) {
    indices[index++] = x * GRID_RESOLUTION_Y;
    for (uint32_t y = 0; y < GRID_RESOLUTION_Y; ++y) {
      indices[index++] = x * GRID_RESOLUTION_Y + y;
      indices[index++] = (x + 1) * GRID_RESOLUTION_Y + y;
    }
    indices[index] = indices[index - 1];
    ++index;
  }

  mSphereVAO.Bind();

  mSphereVBO.Bind(GL_ARRAY_BUFFER);
  mSphereVBO.BufferData(vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

  mSphereIBO.Bind(GL_ELEMENT_ARRAY_BUFFER);
  mSphereIBO.BufferData(indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

  mSphereVAO.EnableAttributeArray(0);
  mSphereVAO.SpecifyAttributeArrayFloat(
      0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0, &mSphereVBO);

  mSphereVAO.Release();
  mSphereIBO.Release();
  mSphereVBO.Release();

  // Recreate the shader if lighting or HDR rendering mode are toggled.
  mEnableLightingConnection =
      mGraphicsEngine->pEnableLighting.connect([this](bool) { mShaderDirty = true; });
  mEnableHDRConnection = mGraphicsEngine->pEnableHDR.connect([this](bool) { mShaderDirty = true; });
}
////////////////////////////////////////////////////////////////////////////////////////////////////

SimpleWMSBody::~SimpleWMSBody() {
  for (auto it = mTextures.begin(); it != mTextures.end(); ++it) {
    stbi_image_free(it->second);
  }

  mGraphicsEngine->pEnableLighting.disconnect(mEnableLightingConnection);
  mGraphicsEngine->pEnableHDR.disconnect(mEnableHDRConnection);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SimpleWMSBody::setSun(std::shared_ptr<const cs::scene::CelestialObject> const& sun) {
  mSun = sun;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool SimpleWMSBody::getIntersection(
    glm::dvec3 const& rayOrigin, glm::dvec3 const& rayDir, glm::dvec3& pos) const {

  glm::dmat4 transform = glm::inverse(getWorldTransform());

  // Transform ray into planet coordinate system.
  glm::dvec4 origin(rayOrigin, 1.0);
  origin = transform * origin;

  glm::dvec4 direction(rayDir, 0.0);
  direction = transform * direction;
  direction = glm::normalize(direction);

  double b    = glm::dot(origin, direction);
  double c    = glm::dot(origin, origin) - mRadii[0] * mRadii[0];
  double fDet = b * b - c;

  if (fDet < 0.0) {
    return false;
  }

  fDet = std::sqrt(fDet);
  pos  = (origin + direction * (-b - fDet));

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

double SimpleWMSBody::getHeight(glm::dvec2 lngLat) const {
  // This is why we call them 'SimpleBodies'.
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

glm::dvec3 SimpleWMSBody::getRadii() const {
  return mRadii;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool SimpleWMSBody::Do() {
  std::lock_guard<std::mutex> guard(mWmsMutex);
  if (!getIsInExistence() || !pVisible.get()) {
    return true;
  }

  cs::utils::FrameTimings::ScopedTimer timer("Simple Wms Planets");

  if (mActiveWms.mTime.has_value()) {
    boost::posix_time::ptime time =
        cs::utils::convert::toBoostTime(mTimeControl->pSimulationTime.get());
    for (int i = -mPreFetch; i <= mPreFetch; i++) {
      boost::posix_time::time_duration td = boost::posix_time::seconds(mIntervalDuration);
      time = cs::utils::convert::toBoostTime(mTimeControl->pSimulationTime.get()) + td * i;
      boost::posix_time::time_duration timeSinceStart;
      boost::posix_time::ptime         startTime =
          time - boost::posix_time::microseconds(time.time_of_day().fractional_seconds());
      bool inInterval = utils::timeInIntervals(
          startTime, mTimeIntervals, timeSinceStart, mIntervalDuration, mFormat);
      if (mIntervalDuration != 0) {
        startTime -= boost::posix_time::seconds(timeSinceStart.total_seconds() % mIntervalDuration);
      }
      std::string timeString = utils::timeToString(mFormat.c_str(), startTime);
      if (mProperties->mEnableTimespan.get()) {
        boost::posix_time::time_duration timeDuration =
            boost::posix_time::seconds(mIntervalDuration);
        boost::posix_time::ptime intervalAfter = getStartTime(startTime + timeDuration);
        timeString += "/" + utils::timeToString(mFormat.c_str(), intervalAfter);
      }
      auto iteratorText1 = mTextureFilesBuffer.find(timeString);
      auto iteratorText2 = mTexturesBuffer.find(timeString);
      auto iteratorText3 = mTextures.find(timeString);
      if (iteratorText1 == mTextureFilesBuffer.end() && iteratorText2 == mTexturesBuffer.end() &&
          iteratorText3 == mTextures.end() && inInterval) {
        mTextureFilesBuffer.insert(std::pair<std::string, std::future<std::string>>(timeString,
            mTextureLoader.loadTextureAsync(timeString, mRequest, mActiveWms.mLayers, mFormat)));
      }
    }

    bool fileError = false;

    // Load Wms textures to buffer.
    for (auto it = mTextureFilesBuffer.begin(); it != mTextureFilesBuffer.end(); ++it) {
      if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        std::string fileName = it->second.get();
        if (fileName != "Error") {
          mTexturesBuffer.insert(std::pair<std::string, std::future<unsigned char*>>(
              it->first, mTextureLoader.loadTextureFromFileAsync(fileName)));
        } else {
          fileError = true;
          mTexturesBuffer.insert(std::pair<std::string, std::future<unsigned char*>>(
              it->first, mTextureLoader.loadTextureFromFileAsync(mDefaultTextureFile)));
        }
        mTextureFilesBuffer.erase(it);
      }
    }

    // Add loaded textures to map.
    for (auto it = mTexturesBuffer.begin(); it != mTexturesBuffer.end(); ++it) {
      if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        mTextures.insert(std::pair<std::string, unsigned char*>(it->first, it->second.get()));
        mTexturesBuffer.erase(it);
      }
    }

    time = cs::utils::convert::toBoostTime(mTimeControl->pSimulationTime.get());
    boost::posix_time::time_duration timeSinceStart;
    boost::posix_time::ptime         startTime =
        time - boost::posix_time::microseconds(time.time_of_day().fractional_seconds());
    bool inInterval = utils::timeInIntervals(
        startTime, mTimeIntervals, timeSinceStart, mIntervalDuration, mFormat);
    if (mIntervalDuration != 0) {
      startTime -= boost::posix_time::seconds(timeSinceStart.total_seconds() % mIntervalDuration);
    }
    boost::posix_time::time_duration timeDuration = boost::posix_time::seconds(mIntervalDuration);
    std::string                      timeString   = utils::timeToString(mFormat.c_str(), startTime);
    if (mProperties->mEnableTimespan.get()) {
      boost::posix_time::ptime intervalAfter = getStartTime(startTime + timeDuration);
      timeString += "/" + utils::timeToString(mFormat.c_str(), intervalAfter);
    }
    auto iterator = mTextures.find(timeString);

    // Use Wms texture inside the interval.
    if (inInterval && !fileError) {
      if (mCurentTexture != timeString && iterator != mTextures.end()) {
        mDefaultTextureUsed = false;
        mWmsTexture->UploadTexture(mTextureWidth, mTextureHeight, iterator->second, false);
        mTexture       = mWmsTexture;
        mCurentTexture = timeString;
      }
      // Use default planet texture.
    } else {
      mDefaultTextureUsed = true;
      mTexture            = mDefaultTexture;
      mCurentTexture      = utils::timeToString(mFormat.c_str(), startTime);
    }

    if (mDefaultTextureUsed || !mProperties->mEnableInterpolation.get() || mIntervalDuration == 0) {
      mOtherTextureUsed = false;
      // Create fading between Wms textures.
    } else {
      boost::posix_time::ptime intervalAfter = getStartTime(startTime + timeDuration);
      auto it = mTextures.find(utils::timeToString(mFormat.c_str(), intervalAfter));
      if (it != mTextures.end()) {
        if (mCurentOtherTexture != utils::timeToString(mFormat.c_str(), intervalAfter)) {
          mOtherTexture->UploadTexture(mTextureWidth, mTextureHeight, it->second, false);
          mCurentOtherTexture = utils::timeToString(mFormat.c_str(), intervalAfter);
          mOtherTextureUsed   = true;
        }
        mFade = static_cast<float>((double)(intervalAfter - time).total_seconds() /
                                   (double)(intervalAfter - startTime).total_seconds());
      }
    }
  }

  if (mShaderDirty) {
    mShader = VistaGLSLShader();

    // (Re-)create sphere shader.
    std::string defines = "#version 330\n";

    if (mGraphicsEngine->pEnableHDR.get()) {
      defines += "#define ENABLE_HDR\n";
    }

    if (mGraphicsEngine->pEnableLighting.get()) {
      defines += "#define ENABLE_LIGHTING\n";
    }

    mShader.InitVertexShaderFromString(defines + SPHERE_VERT);
    mShader.InitFragmentShaderFromString(defines + SPHERE_FRAG);
    mShader.Link();

    mShaderDirty = false;
  }

  mShader.Bind();

  glm::vec3 sunDirection(1, 0, 0);
  float     sunIlluminance(1.f);
  float     ambientBrightness(mGraphicsEngine->pAmbientBrightness.get());

  if (getCenterName() == "Sun") {
    // If the SimpleWMSBody is actually the sun, we have to calculate the lighting differently.
    if (mGraphicsEngine->pEnableHDR.get()) {
      double sceneScale = 1.0 / mSolarSystem->getObserver().getAnchorScale();
      sunIlluminance    = static_cast<float>(
          mSolarSystem->pSunLuminousPower.get() /
          (sceneScale * sceneScale * mRadii[0] * mRadii[0] * 4.0 * glm::pi<double>()));
    }

    ambientBrightness = 1.0f;

  } else if (mSun) {
    // For all other bodies we can use the utility methods from the SolarSystem.
    if (mGraphicsEngine->pEnableHDR.get()) {
      sunIlluminance = static_cast<float>(mSolarSystem->getSunIlluminance(getWorldTransform()[3]));
    }

    sunDirection = mSolarSystem->getSunDirection(getWorldTransform()[3]);
  }

  // Set uniforms.
  mShader.SetUniform(mShader.GetUniformLocation("uSunDirection"), sunDirection[0], sunDirection[1],
      sunDirection[2]);
  mShader.SetUniform(mShader.GetUniformLocation("uSunIlluminance"), sunIlluminance);
  mShader.SetUniform(mShader.GetUniformLocation("uAmbientBrightness"), ambientBrightness);
  mShader.SetUniform(mShader.GetUniformLocation("uSecondTexture"), mOtherTextureUsed);

  // Get modelview and projection matrices.
  GLfloat glMatMV[16], glMatP[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, &glMatMV[0]);
  glGetFloatv(GL_PROJECTION_MATRIX, &glMatP[0]);
  auto matMV = glm::make_mat4x4(glMatMV) * glm::mat4(getWorldTransform());
  glUniformMatrix4fv(
      mShader.GetUniformLocation("uMatModelView"), 1, GL_FALSE, glm::value_ptr(matMV));
  glUniformMatrix4fv(mShader.GetUniformLocation("uMatProjection"), 1, GL_FALSE, glMatP);

  mShader.SetUniform(mShader.GetUniformLocation("uSurfaceTexture"), 0);
  mShader.SetUniform(mShader.GetUniformLocation("uDefaultTexture"), 1);
  mShader.SetUniform(mShader.GetUniformLocation("uSecondSurfaceTexture"), 2);
  mShader.SetUniform(
      mShader.GetUniformLocation("uRadii"), (float)mRadii[0], (float)mRadii[0], (float)mRadii[0]);
  mShader.SetUniform(
      mShader.GetUniformLocation("uFarClip"), cs::utils::getCurrentFarClipDistance());

  if (mOtherTextureUsed) {
    mShader.SetUniform(mShader.GetUniformLocation("uFade"), mFade);
    mOtherTexture->Bind(GL_TEXTURE2);
  }
  mTexture->Bind(GL_TEXTURE0);
  mDefaultTexture->Bind(GL_TEXTURE1);

  // Draw.
  mSphereVAO.Bind();
  glDrawElements(GL_TRIANGLE_STRIP, (GRID_RESOLUTION_X - 1) * (2 + 2 * GRID_RESOLUTION_Y),
      GL_UNSIGNED_INT, nullptr);
  mSphereVAO.Release();

  // Clean up.
  mTexture->Unbind(GL_TEXTURE0);
  mDefaultTexture->Unbind(GL_TEXTURE1);
  if (mOtherTextureUsed) {
    mOtherTexture->Unbind(GL_TEXTURE2);
  }

  mShader.Release();

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool SimpleWMSBody::GetBoundingBox(VistaBoundingBox& bb) {
  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

boost::posix_time::ptime SimpleWMSBody::getStartTime(boost::posix_time::ptime time) {
  boost::posix_time::time_duration timeSinceStart;
  bool                             inInterval =
      utils::timeInIntervals(time, mTimeIntervals, timeSinceStart, mIntervalDuration, mFormat);
  boost::posix_time::ptime startTime =
      time - boost::posix_time::seconds(timeSinceStart.total_seconds() % mIntervalDuration);
  return startTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SimpleWMSBody::setActiveWms(Wms wms) {
  if (wmsInitialized) {
    std::lock_guard<std::mutex> guard(mWmsMutex);
    for (auto it = mTextures.begin(); it != mTextures.end(); ++it) {
      stbi_image_free(it->second);
    }
  }
  wmsInitialized = true;
  mTextures.clear();
  mTextureFilesBuffer.clear();
  mTexturesBuffer.clear();
  mActiveWms = wms;
  std::stringstream url;
  url << mActiveWms.mUrl << "&WIDTH=" << mActiveWms.mWidth << "&HEIGHT=" << mActiveWms.mHeight
      << "&LAYERS=" << mActiveWms.mLayers;
  mRequest               = url.str();
  std::string requestStr = mRequest;
  mTextureWidth          = mActiveWms.mWidth;
  mTextureHeight         = mActiveWms.mHeight;

  mPreFetch = mActiveWms.preFetch.value_or(0);

  if (mActiveWms.mTime.has_value()) {
    mTimeIntervals.clear();
    utils::parseIsoString(mActiveWms.mTime.value(), mTimeIntervals);
    mIntervalDuration   = mTimeIntervals.at(0).mIntervalDuration;
    mFormat             = mTimeIntervals.at(0).mFormat;
    mTexture            = mDefaultTexture;
    mDefaultTextureUsed = true;
    mCurentTexture      = "";
  } else {
    std::ofstream out;
    std::string   cacheFile = "../share/resources/textures/" + mActiveWms.mLayers + ".png";
    out.open(cacheFile, std::ofstream::out | std::ofstream::binary);

    if (!out) {
      spdlog::error("Failed to open '{}' for writing!", cacheFile);
    }
    curlpp::Easy request;
    request.setOpt(curlpp::options::Url(requestStr));
    request.setOpt(curlpp::options::WriteStream(&out));
    request.setOpt(curlpp::options::NoSignal(true));
    try {
      request.perform();
    } catch (std::exception& e) {
      spdlog::error("Failed to load '{}'! Exception: '{}'", requestStr, e.what());
    }
    out.close();

    mTexture = cs::graphics::TextureLoader::loadFromFile(cacheFile);
  }
}

void SimpleWMSBody::setActiveWms(std::string wms) {
  for (int i = 0; i < mWms.size(); i++) {
    if (wms == mWms.at(i).mName) {
      setActiveWms(mWms.at(i));
    }
  }
}

std::vector<Wms> SimpleWMSBody::getWms() {
  return mWms;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Wms SimpleWMSBody::getActiveWms() {
  return mActiveWms;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<timeInterval> SimpleWMSBody::getTimeIntervals() {
  return mTimeIntervals;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace csp::simpleWmsBodies
