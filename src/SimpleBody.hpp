////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_WMS_SIMPLE_BODIES_HPP
#define CSP_WMS_SIMPLE_BODIES_HPP

#include <VistaKernel/GraphicsManager/VistaOpenGLDraw.h>
#include <VistaOGLExt/VistaBufferObject.h>
#include <VistaOGLExt/VistaGLSLShader.h>
#include <VistaOGLExt/VistaTexture.h>
#include <VistaOGLExt/VistaVertexArrayObject.h>

#include "../../../src/cs-scene/CelestialBody.hpp"

#include "../../../src/cs-core/SolarSystem.hpp"
#include "../../../src/cs-graphics/TextureLoader.hpp"
#include "../../../src/cs-utils/FrameTimings.hpp"
#include "../../../src/cs-utils/utils.hpp"
#include "../../../src/cs-utils/convert.hpp"
#include "../../../src/cs-core/TimeControl.hpp"

#include "WebMapTextureLoader.hpp"

#include <VistaMath/VistaBoundingBox.h>
#include <VistaOGLExt/VistaOGLUtils.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Info.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <fstream>
#include <sstream>
#include <mutex>


#include <boost/date_time/posix_time/posix_time.hpp>

#include <glm/gtc/type_ptr.hpp>

namespace csp::simpleWmsBodies {

 struct Properties {
    cs::utils::Property<bool> mEnableInterpolation = true;
    cs::utils::Property<bool> mEnableTimespan = false;
  };

/// This is just a sphere with a texture...
class SimpleBody : public cs::scene::CelestialBody, public IVistaOpenGLDraw {
 public:
  SimpleBody(std::string const& sCenterName, std::string texture,
      std::string const& sFrameName, double tStartExistence, double tEndExistence, std::vector<Wms> tWms, std::shared_ptr<cs::core::TimeControl> timeControl, std::shared_ptr<Properties> properties = nullptr);
  ~SimpleBody() override;

  void setSun(std::shared_ptr<const cs::scene::CelestialObject> const& sun);

  bool getIntersection(
      glm::dvec3 const& rayOrigin, glm::dvec3 const& rayDir, glm::dvec3& pos) const override;
  double     getHeight(glm::dvec2 lngLat) const override;
  glm::dvec3 getRadii() const override;

  bool Do() override;
  bool GetBoundingBox(VistaBoundingBox& bb) override;

  std::vector<Wms> getWms();
  Wms getActiveWms();

  std::vector<timeInterval> getTimeIntervals();

  void setActiveWms(Wms wms);
  void setActiveWms(std::string wms);

 private:
  std::vector<Wms> mWms;
  Wms mActiveWms;
  std::mutex mWmsMutex;
  bool wmsIninialized = false;

  std::shared_ptr<VistaTexture> mTexture;
  std::shared_ptr<VistaTexture> mWmsTexture;
  std::shared_ptr<VistaTexture> mOtherTexture;
  std::shared_ptr<VistaTexture> mDefaultTexture;
  std::string mDefaultTextureFile;
  unsigned char * mDefaultTexturePixels;
  int mDefTextWidth;
  int mDefTextHeight;
  std::string mRequest;
  std::string mFormat;
  int mIntervalDuration;
  int mTextureWidth;
  int mTextureHeight;
  int mPreFetch; //Amount of textures that gets prefetched in every direction.
  std::vector<timeInterval> mTimeIntervals;
  bool mDeafaultTextureUsed;

  VistaGLSLShader        mShader;
  VistaVertexArrayObject mSphereVAO;
  VistaBufferObject      mSphereVBO;
  VistaBufferObject      mSphereIBO;

  WebMapTextureLoader mTextureLoader;

  std::shared_ptr<const cs::scene::CelestialObject> mSun;
  std::shared_ptr<cs::core::TimeControl>             mTimeControl;

  glm::dvec3 mRadii;

  static const std::string SPHERE_VERT;
  static const std::string SPHERE_FRAG;

  std::map<std::string, std::future<std::string> > mTextureFilesBuffer;
  std::map<std::string, std::future<unsigned char *> > mTexturesBuffer;

  std::map<std::string, unsigned char *> mTextures;
  std::string mCurentTexture;
  std::string mCurentOtherTexture;
  bool mOtherTextureBefore;
  bool mOtherTextureUsed = false;
  float mFade;
  std::shared_ptr<Properties> mProperties;

  boost::posix_time::ptime getStartTime(boost::posix_time::ptime time);
};

} // namespace csp::simpleWmsBodies

#endif // CSP_WMS_SIMPLE_BODIES_HPP
