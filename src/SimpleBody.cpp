////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "SimpleBody.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

namespace csp::simpleWmsBodies {

////////////////////////////////////////////////////////////////////////////////////////////////////

const unsigned GRID_RESOLUTION_X = 200;
const unsigned GRID_RESOLUTION_Y = 100;

////////////////////////////////////////////////////////////////////////////////////////////////////

const std::string SimpleBody::SPHERE_VERT = R"(
#version 330

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
}
)";

////////////////////////////////////////////////////////////////////////////////////////////////////

const std::string SimpleBody::SPHERE_FRAG = R"(
#version 330

uniform vec3 uSunDirection;
uniform sampler2D uSurfaceTexture;
uniform sampler2D uSecondSurfaceTexture;
uniform sampler2D uDefaultTexture;
uniform float uAmbientBrightness;
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

void main()
{
    vec3 normal = normalize(vPosition - vCenter);
    
    vec4 texColor = texture(uSurfaceTexture, vTexCoords);
    vec3 defColor = texture(uDefaultTexture, vTexCoords).rgb;
    oColor = mix(defColor, texColor.rgb, texColor.a);
    if(uSecondTexture) {
      vec4 secColorA = texture(uSecondSurfaceTexture, vTexCoords);
      vec3 secColor = mix(defColor, secColorA.rgb, secColorA.a);
      oColor = mix(secColor, oColor, uFade);
    }

    gl_FragDepth = length(vPosition) / uFarClip;
}
)";

////////////////////////////////////////////////////////////////////////////////////////////////////

SimpleBody::SimpleBody(std::string const& sCenterName, std::string texture,
    std::string const& sFrameName, double tStartExistence, double tEndExistence, std::vector<Wms> tWms, std::shared_ptr<cs::core::TimeControl> timeControl, std::shared_ptr<Properties> properties)
    : cs::scene::CelestialBody(sCenterName, sFrameName, tStartExistence, tEndExistence)
    , mRadii(cs::core::SolarSystem::getRadii(sCenterName))
    , mWmsTexture(new VistaTexture(GL_TEXTURE_2D))
    , mDefaultTexture(new VistaTexture(GL_TEXTURE_2D))
    , mOtherTexture(new VistaTexture(GL_TEXTURE_2D)) {
  pVisibleRadius = mRadii[0];
  mTimeControl = timeControl;
  mProperties = properties;
  mWms = tWms;

  mDefaultTextureFile = texture;
  int  bpp;
  int channels = 4;
  unsigned char* mDefaultTexturePixels = stbi_load(mDefaultTextureFile.c_str(), &mDefTextWidth, &mDefTextHeight, &bpp, channels);
  mDefaultTexture->UploadTexture(mDefTextWidth, mDefTextHeight, mDefaultTexturePixels);

  setActiveWms(mWms.at(0));

  // create sphere grid geometry
  std::vector<float>    vertices(GRID_RESOLUTION_X * GRID_RESOLUTION_Y * 2);
  std::vector<unsigned> indices((GRID_RESOLUTION_X - 1) * (2 + 2 * GRID_RESOLUTION_Y));

  for (int x = 0; x < GRID_RESOLUTION_X; ++x) {
    for (int y = 0; y < GRID_RESOLUTION_Y; ++y) {
      vertices[(x * GRID_RESOLUTION_Y + y) * 2 + 0] = 1.f / (GRID_RESOLUTION_X - 1) * x;
      vertices[(x * GRID_RESOLUTION_Y + y) * 2 + 1] = 1.f / (GRID_RESOLUTION_Y - 1) * y;
    }
  }

  int index = 0;

  for (int x = 0; x < GRID_RESOLUTION_X - 1; ++x) {
    indices[index++] = x * GRID_RESOLUTION_Y;
    for (int y = 0; y < GRID_RESOLUTION_Y; ++y) {
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

  // create sphere shader
  mShader.InitVertexShaderFromString(SPHERE_VERT);
  mShader.InitFragmentShaderFromString(SPHERE_FRAG);
  mShader.Link();
}
////////////////////////////////////////////////////////////////////////////////////////////////////

SimpleBody::~SimpleBody() {
  stbi_image_free(mDefaultTexturePixels);
  for(auto it=mTextures.begin(); it!=mTextures.end(); ++it) {
    stbi_image_free(it->second);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SimpleBody::setSun(std::shared_ptr<const cs::scene::CelestialObject> const& sun) {
  mSun = sun;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool SimpleBody::getIntersection(
    glm::dvec3 const& rayOrigin, glm::dvec3 const& rayDir, glm::dvec3& pos) const {

  glm::dmat4 transform = glm::inverse(getWorldTransform());

  // Transform ray into planet coordinate system
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

double SimpleBody::getHeight(glm::dvec2 lngLat) const {
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

glm::dvec3 SimpleBody::getRadii() const {
  return mRadii;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool SimpleBody::Do() {
  std::lock_guard<std::mutex> guard(mWmsMutex);
  if (!getIsInExistence() || !pVisible.get()) {
    return true;
  }

  cs::utils::FrameTimings::ScopedTimer timer("Simple Planets");
  

  if(mActiveWms.mTime.has_value()) {
    boost::posix_time::ptime time = cs::utils::convert::toBoostTime(mTimeControl->pSimulationTime.get());
    for(int i=-mPreFetch; i<=mPreFetch;i++) {
      boost::posix_time::time_duration td = boost::posix_time::seconds(mIntervalDuration);
      time = cs::utils::convert::toBoostTime(mTimeControl->pSimulationTime.get()) + td * i;
      boost::posix_time::time_duration timeSinceStart;
      boost::posix_time::ptime startTime = time - boost::posix_time::microseconds(time.time_of_day().fractional_seconds());
      bool inInterval = utils::timeInIntervals(startTime, mTimeIntervals, timeSinceStart, mIntervalDuration, mFormat);
      if(mIntervalDuration != 0) {
        startTime -=  boost::posix_time::seconds(timeSinceStart.total_seconds() % mIntervalDuration);
      }
      std::string timeString = utils::timeToString(mFormat.c_str(), startTime);
      if(mProperties->mEnableTimespan.get()) {
        boost::posix_time::time_duration timeDuration =  boost::posix_time::seconds(mIntervalDuration);
        boost::posix_time::ptime intervalAfter = getStartTime(startTime + timeDuration);
        timeString += "/" + utils::timeToString(mFormat.c_str(), intervalAfter);
      }
      auto iteratorText1 = mTextureFilesBuffer.find(timeString);
      auto iteratorText2= mTexturesBuffer.find(timeString);
      auto iteratorText3 = mTextures.find(timeString);
      if(iteratorText1 == mTextureFilesBuffer.end() && iteratorText2 == mTexturesBuffer.end() && iteratorText3 == mTextures.end() && inInterval) {
        mTextureFilesBuffer.insert( std::pair<std::string, std::future<std::string> >(
          timeString, mTextureLoader.loadTextureAsync(timeString,mRequest, mActiveWms.mLayers, mFormat)) 
        );
      }
    }

    for(auto it=mTextureFilesBuffer.begin(); it!=mTextureFilesBuffer.end(); ++it) {
      if(it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        std::string fileName = it->second.get();
        if(fileName != "Error") {
          mTexturesBuffer.insert(std::pair<std::string, std::future<unsigned char *> > (
            it->first, mTextureLoader.loadTextureFromFileAsync(fileName)));
        } else {
          mTexturesBuffer.insert(std::pair<std::string, std::future<unsigned char *> > (
            it->first, mTextureLoader.loadTextureFromFileAsync(mDefaultTextureFile)));
        }
        mTextureFilesBuffer.erase(it);
      }
    }

    for(auto it=mTexturesBuffer.begin(); it!=mTexturesBuffer.end(); ++it) {
      if(it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        mTextures.insert(std::pair<std::string, unsigned char *>(
          it->first, it->second.get()));
        mTexturesBuffer.erase(it);
      }
    }

    time = cs::utils::convert::toBoostTime(mTimeControl->pSimulationTime.get());
    boost::posix_time::time_duration timeSinceStart;
    boost::posix_time::ptime startTime = time - boost::posix_time::microseconds(time.time_of_day().fractional_seconds());
    bool inInterval = utils::timeInIntervals(startTime, mTimeIntervals, timeSinceStart, mIntervalDuration, mFormat);
    if(mIntervalDuration != 0) {
      startTime -=  boost::posix_time::seconds(timeSinceStart.total_seconds() % mIntervalDuration);
    }
    boost::posix_time::time_duration timeDuration = boost::posix_time::seconds(mIntervalDuration);
    std::string timeString = utils::timeToString(mFormat.c_str(), startTime);
      if(mProperties->mEnableTimespan.get()) {
        boost::posix_time::ptime intervalAfter = getStartTime(startTime + timeDuration);
        timeString += "/" + utils::timeToString(mFormat.c_str(), intervalAfter);
    }
    auto iterator = mTextures.find(timeString);

    if(inInterval) {
      if(mCurentTexture != timeString && iterator != mTextures.end()) {
          mDeafaultTextureUsed = false;
          mWmsTexture->UploadTexture(mTextureWidth, mTextureHeight ,iterator->second, false);
          mTexture = mWmsTexture;
          mCurentTexture = timeString;
      }
    }else {
      mDeafaultTextureUsed = true;
      mTexture = mDefaultTexture;
      mCurentTexture = utils::timeToString(mFormat.c_str(), startTime);
    }

    if(mDeafaultTextureUsed || !mProperties->mEnableInterpolation.get() || mIntervalDuration == 0) {
       mOtherTextureUsed = false;
    } else {
      boost::posix_time::ptime intervalAfter = getStartTime(startTime + timeDuration);
      auto it = mTextures.find(utils::timeToString(mFormat.c_str(), intervalAfter));
      if(it != mTextures.end()) {
        if(mCurentOtherTexture != utils::timeToString(mFormat.c_str(), intervalAfter)) {
          mOtherTexture->UploadTexture(mTextureWidth, mTextureHeight ,it->second, false);
          mCurentOtherTexture = utils::timeToString(mFormat.c_str(), intervalAfter);
          mOtherTextureUsed = true;
        }
        mFade = (double)(intervalAfter - time).total_seconds() / (double)(intervalAfter - startTime).total_seconds();
      }
    }
  }
  // set uniforms
  mShader.Bind();
  mShader.SetUniform(mShader.GetUniformLocation("uSecondTexture"), mOtherTextureUsed);
  // get modelview and projection matrices
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

  if (getCenterName() != "Sun") {
    auto sunTransform    = glm::make_mat4x4(glMatMV) * glm::mat4(mSun->getWorldTransform());
    auto planetTransform = matMV;

    auto sunDirection = glm::vec3(sunTransform[3]) - glm::vec3(planetTransform[3]);
    sunDirection      = glm::normalize(sunDirection);

    mShader.SetUniform(mShader.GetUniformLocation("uSunDirection"), sunDirection[0],
        sunDirection[1], sunDirection[2]);
    mShader.SetUniform(mShader.GetUniformLocation("uAmbientBrightness"), 0.2f);
  } else {
    mShader.SetUniform(mShader.GetUniformLocation("uAmbientBrightness"), 1.f);
  }
  if(mOtherTextureUsed) {
    mShader.SetUniform(mShader.GetUniformLocation("uFade"), mFade);
    mOtherTexture->Bind(GL_TEXTURE2);
  }
  mTexture->Bind(GL_TEXTURE0);
  mDefaultTexture->Bind(GL_TEXTURE1);

  // draw
  mSphereVAO.Bind();
  glDrawElements(GL_TRIANGLE_STRIP, (GRID_RESOLUTION_X - 1) * (2 + 2 * GRID_RESOLUTION_Y),
      GL_UNSIGNED_INT, nullptr);
  mSphereVAO.Release();

  // clean up
  mTexture->Unbind(GL_TEXTURE0);
  mDefaultTexture->Unbind(GL_TEXTURE1);
  if(mOtherTextureUsed) {
    mOtherTexture->Unbind(GL_TEXTURE2);
  }

  mShader.Release();

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool SimpleBody::GetBoundingBox(VistaBoundingBox& bb) {
  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

boost::posix_time::ptime SimpleBody::getStartTime(boost::posix_time::ptime time) {
  boost::posix_time::time_duration timeSinceStart;
  bool inInterval = utils::timeInIntervals(time, mTimeIntervals, timeSinceStart, mIntervalDuration, mFormat);
  boost::posix_time::ptime startTime = time - boost::posix_time::seconds(timeSinceStart.total_seconds() % mIntervalDuration);
  return startTime;
}
 
////////////////////////////////////////////////////////////////////////////////////////////////////

void SimpleBody::setActiveWms(Wms wms) {
  static bool firstCall = true;
  if(wmsIninialized) {
    std::lock_guard<std::mutex> guard(mWmsMutex);
    for(auto it=mTextures.begin(); it!=mTextures.end(); ++it) {
      stbi_image_free(it->second);
    }
  }
  wmsIninialized = true;
  mTextures.clear();
  mTextureFilesBuffer.clear();
  mTexturesBuffer.clear();
  mActiveWms = wms;
  std::stringstream url;
  url << mActiveWms.mUrl << "&WIDTH=" << mActiveWms.mWidth << "&HEIGHT=" << mActiveWms.mHeight << "&LAYERS=" << mActiveWms.mLayers;
  mRequest = url.str();
  std::string requestStr = mRequest;
  mTextureWidth = mActiveWms.mWidth;
  mTextureHeight = mActiveWms.mHeight;

  mPreFetch = mActiveWms.preFetch.value_or(0);

  if(mActiveWms.mTime.has_value()) {
    mTimeIntervals.clear();
    utils::parseIsoString(mActiveWms.mTime.value(), mTimeIntervals);
    mIntervalDuration = mTimeIntervals.at(0).mIntervalDuration;
    mFormat = mTimeIntervals.at(0).mFormat;
    mTexture = mDefaultTexture;
    mDeafaultTextureUsed = true;
    mCurentTexture = "";
  } else {
    std::ofstream out;
    std::string cacheFile = "../share/resources/textures/" + mActiveWms.mLayers + ".png";
    out.open(cacheFile, std::ofstream::out | std::ofstream::binary);

    if (!out) {
      std::cerr << "Failed to open for writing!" << std::endl;
    }
    curlpp::Easy request;
    request.setOpt(curlpp::options::Url(requestStr));
    request.setOpt(curlpp::options::WriteStream(&out));
    request.setOpt(curlpp::options::NoSignal(true));
    request.perform();
    out.close();

    mTexture = cs::graphics::TextureLoader::loadFromFile(cacheFile);
  }
}

void SimpleBody::setActiveWms(std::string wms) {
  for(int i=0; i < mWms.size();i++) {
    if(wms == mWms.at(i).mName) {
      setActiveWms(mWms.at(i));
    }
  }
}

std::vector<Wms> SimpleBody::getWms() {
  return mWms;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Wms SimpleBody::getActiveWms() {
  return mActiveWms;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<timeInterval> SimpleBody::getTimeIntervals() {
  return mTimeIntervals;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace csp::simpleWmsBodies
