////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_WMS_SIMPLE_BODIES_HPP
#define CSP_WMS_SIMPLE_BODIES_HPP

#include "WebMapTextureLoader.hpp"
#include "utils.hpp"

#include "../../../src/cs-core/SolarSystem.hpp"
#include "../../../src/cs-core/TimeControl.hpp"
#include "../../../src/cs-scene/CelestialBody.hpp"

#include <VistaKernel/GraphicsManager/VistaOpenGLDraw.h>
#include <VistaOGLExt/VistaBufferObject.h>
#include <VistaOGLExt/VistaGLSLShader.h>
#include <VistaOGLExt/VistaTexture.h>
#include <VistaOGLExt/VistaVertexArrayObject.h>

#include <glm/gtc/type_ptr.hpp>

namespace csp::simpleWmsBodies {

struct Properties {
  cs::utils::Property<bool> mEnableInterpolation = true;
  cs::utils::Property<bool> mEnableTimespan      = false;
};

/// This is just a sphere with a texture, attached to the given SPICE frame. The texture should be
/// in equirectangular projection.
class SimpleWMSBody : public cs::scene::CelestialBody, public IVistaOpenGLDraw {
 public:
  SimpleWMSBody(std::shared_ptr<cs::core::GraphicsEngine> const& graphicsEngine,
      std::shared_ptr<cs::core::SolarSystem> const& solarSystem, std::string const& sCenterName,
      std::string sTexture, std::string const& sFrameName, double tStartExistence,
      double tEndExistence, std::vector<Wms> tWms,
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

  std::vector<Wms> getWms();
  Wms              getActiveWms();

  void setActiveWms(Wms wms);
  void setActiveWms(std::string wms);

  std::vector<timeInterval> getTimeIntervals();

 private:
  std::shared_ptr<cs::core::GraphicsEngine> mGraphicsEngine;
  std::shared_ptr<cs::core::SolarSystem>    mSolarSystem;

  std::vector<Wms> mWms;
  Wms              mActiveWms;
  std::mutex       mWmsMutex;
  bool             wmsInitialized = false;

  std::shared_ptr<VistaTexture> mTexture;
  std::shared_ptr<VistaTexture> mWmsTexture;
  std::shared_ptr<VistaTexture> mOtherTexture;
  std::shared_ptr<VistaTexture> mDefaultTexture;
  std::string                   mDefaultTextureFile;
  int                           mDefTextWidth;
  int                           mDefTextHeight;
  std::string                   mRequest;
  std::string                   mFormat;
  int                           mIntervalDuration;
  int                           mTextureWidth;
  int                           mTextureHeight;
  int                           mPreFetch; /// Amount of textures that gets prefetched in every direction.
  std::vector<timeInterval>     mTimeIntervals;
  bool                          mDefaultTextureUsed;

  VistaGLSLShader        mShader;
  VistaVertexArrayObject mSphereVAO;
  VistaBufferObject      mSphereVBO;
  VistaBufferObject      mSphereIBO;

  WebMapTextureLoader mTextureLoader;

  std::shared_ptr<const cs::scene::CelestialObject> mSun;
  std::shared_ptr<cs::core::TimeControl>            mTimeControl;

  glm::dvec3 mRadii;

  bool mShaderDirty              = true;
  int  mEnableLightingConnection = -1;
  int  mEnableHDRConnection      = -1;

  static const std::string SPHERE_VERT;
  static const std::string SPHERE_FRAG;

  std::map<std::string, std::future<std::string>>    mTextureFilesBuffer;
  std::map<std::string, std::future<unsigned char*>> mTexturesBuffer;

  std::map<std::string, unsigned char*> mTextures;
  std::string                           mCurentTexture;
  std::string                           mCurentOtherTexture;
  bool                                  mOtherTextureBefore;
  bool                                  mOtherTextureUsed = false;
  float                                 mFade;
  std::shared_ptr<Properties>           mProperties;

  boost::posix_time::ptime getStartTime(boost::posix_time::ptime time);
};

} // namespace csp::simpleWmsBodies

#endif // CSP_WMS_SIMPLE_BODIES_HPP
