////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_WMS_SIMPLE_BODIES_HPP
#define CSP_WMS_SIMPLE_BODIES_HPP

#include "WebMapTextureLoader.hpp"
#include "utils.hpp"

#include "../../../src/cs-scene/CelestialBody.hpp"

#include <VistaKernel/GraphicsManager/VistaOpenGLDraw.h>
#include <VistaOGLExt/VistaBufferObject.h>
#include <VistaOGLExt/VistaGLSLShader.h>
#include <VistaOGLExt/VistaVertexArrayObject.h>

#include <glm/gtc/type_ptr.hpp>

namespace cs::core {
class SolarSystem;
class TimeControl;
class GraphicsEngine;
} // namespace cs::core

class VistaTexture;

namespace csp::simplewmsbodies {

struct Properties {
  cs::utils::Property<bool> mEnableInterpolation = true;
  cs::utils::Property<bool> mEnableTimespan      = false;
};

/// This is just a sphere with a background texture overlaid with WMS based textures, attached to
/// the given SPICE frame. All of the textures should be in equirectangular projection.
class SimpleWMSBody : public cs::scene::CelestialBody, public IVistaOpenGLDraw {
 public:
  SimpleWMSBody(std::shared_ptr<cs::core::GraphicsEngine> const& graphicsEngine,
      std::shared_ptr<cs::core::SolarSystem> const& solarSystem, std::string const& sCenterName,
      std::string sTexture, std::string const& sFrameName, double tStartExistence,
      double tEndExistence, std::vector<WMSConfig> tWms,
      std::shared_ptr<cs::core::TimeControl> timeControl,
      std::shared_ptr<Properties>            properties = nullptr);
  ~SimpleWMSBody() override;

  /// The sun object is used for lighting computation.
  void setSun(std::shared_ptr<const cs::scene::CelestialObject> const& sun);

  /// Interface implementation of the IntersectableObject, which is a base class of
  /// CelestialBody.
  bool getIntersection(
      glm::dvec3 const& rayOrigin, glm::dvec3 const& rayDir, glm::dvec3& pos) const override;

  /// Interface implementation of CelestialBody.
  double     getHeight(glm::dvec2 lngLat) const override;
  glm::dvec3 getRadii() const override;

  /// Interface implementation of IVistaOpenGLDraw.
  bool Do() override;
  bool GetBoundingBox(VistaBoundingBox& bb) override;

  std::vector<WMSConfig> const& getWMSs();
  WMSConfig const&              getActiveWMS();

  void setActiveWMS(WMSConfig const& wms);
  void setActiveWMS(std::string const& wms);

  std::vector<TimeInterval> getTimeIntervals();

 private:
  std::shared_ptr<cs::core::GraphicsEngine>         mGraphicsEngine;
  std::shared_ptr<cs::core::SolarSystem>            mSolarSystem;
  std::shared_ptr<const cs::scene::CelestialObject> mSun;
  std::shared_ptr<cs::core::TimeControl>            mTimeControl;

  glm::dvec3 mRadii;

  std::vector<WMSConfig> mWMSs;
  WMSConfig              mActiveWMS;
  std::mutex             mWMSMutex;

  std::shared_ptr<VistaTexture> mBackgroundTexture; ///< The background texture of the body.
  std::shared_ptr<VistaTexture> mWMSTexture;        ///< The WMS texture.
  std::shared_ptr<VistaTexture> mSecondWMSTexture;  ///< Second WMS texture for time interpolation.
  std::string                   mBackgroundTextureFile; ///< Local path to background texture.
  bool                          mWMSTextureUsed;        ///< Whether to use the WMS texture.
  bool        mSecondWMSTextureUsed = false;            ///< Whether to use the second WMS texture.
  std::string mCurentTexture;                           ///< Timestep of the current WMS texture.
  std::string mCurentSecondTexture;                     ///< Timestep of the second WMS texture.
  float       mFade;                                    ///< Fading value between WMS textures.
  std::string mRequest;
  std::string mFormat;
  int         mIntervalDuration;
  std::vector<TimeInterval> mTimeIntervals;

  std::map<std::string, std::future<std::string>>    mTextureFilesBuffer;
  std::map<std::string, std::future<unsigned char*>> mTexturesBuffer;
  std::map<std::string, unsigned char*>              mTextures;
  std::shared_ptr<Properties>                        mProperties;

  VistaGLSLShader        mShader;
  VistaVertexArrayObject mSphereVAO;
  VistaBufferObject      mSphereVBO;
  VistaBufferObject      mSphereIBO;

  WebMapTextureLoader mTextureLoader;

  bool mShaderDirty              = true;
  int  mEnableLightingConnection = -1;
  int  mEnableHDRConnection      = -1;

  static const std::string SPHERE_VERT;
  static const std::string SPHERE_FRAG;

  boost::posix_time::ptime getStartTime(boost::posix_time::ptime time);
};

} // namespace csp::simplewmsbodies

#endif // CSP_WMS_SIMPLE_BODIES_HPP
