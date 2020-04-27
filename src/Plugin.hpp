////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_SIMPLE_WMS_BODIES_PLUGIN_HPP
#define CSP_SIMPLE_WMS_BODIES_PLUGIN_HPP

#include "utils.hpp"

#include "../../../src/cs-core/PluginBase.hpp"
#include "../../../src/cs-utils/Property.hpp"

#include <VistaKernel/GraphicsManager/VistaOpenGLNode.h>

namespace csp::simplewmsbodies {

class SimpleWMSBody;

/// This plugin provides the rendering of planets as spheres with a texture and an additional WMS
/// based texture. Despite its name it can also render moons :P. It can be configured via the
/// applications config file. See README.md for details.
class Plugin : public cs::core::PluginBase {
 public:
  struct Properties {
    cs::utils::Property<bool> mEnableInterpolation = true;
    cs::utils::Property<bool> mEnableTimespan      = false;
  };

  /// The startup settings of the plugin.
  struct Settings {

    /// A single WMS data set.
    struct WMSConfig {
      std::string mName;      ///< The name of the data set as shown in the UI.
      std::string mCopyright; ///< The copyright holder of the data set (also shown in the UI).
      std::string mUrl;       ///< The URL of the map server including the "SERVICE=wms" parameter.
      int         mWidth;     ///< The width of the WMS image.
      int         mHeight;    ///< The height of the WMS image.
      std::optional<std::string> mTime;   ///< Time intervals of WMS images.
      std::string                mLayers; ///< A comma,seperated list of WMS layers.
      std::optional<int>
          mPrefetchCount; ///< The amount of textures that gets pre-fetched in every time direction.
    };

    /// The startup settings for a planet.
    struct Body {
      std::vector<WMSConfig> mWMS;             ///< The data sets containing WMS data.
      std::string            mTexture;         ///< The path to surface texture.
      int                    mGridResolutionX; ///< The x resolution of the body grid.
      int                    mGridResolutionY; ///< The y resolution of the body gird.
    };

    std::string                 mMapCache; ///< Path to the map cache folder.
    std::map<std::string, Body> mBodies;   ///< A list of planets with their anchor names.
  };

  Plugin();

  void init() override;
  void deInit() override;

 private:
  Settings                                    mPluginSettings;
  std::vector<std::shared_ptr<SimpleWMSBody>> mSimpleWMSBodies;
  std::vector<VistaOpenGLNode*>               mSimpleWMSBodyNodes;
  std::shared_ptr<Properties>                 mProperties;
  std::vector<TimeInterval>                   mIntervalsOnTimeline;

  int mActiveBodyConnection = -1;

  /// Add the time intervals of the current data set to timeline.
  void addTimeInterval(
      std::vector<TimeInterval> timeIntervals, std::string wmsName, std::string planetName);

  /// Remove the time intervals of the current data set to timeline.
  void removeTimeInterval(std::vector<TimeInterval> timeIntervals);
};

} // namespace csp::simplewmsbodies

#endif // CSP_SIMPLE_WMS_BODIES_PLUGIN_HPP
