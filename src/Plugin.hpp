////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_SIMPLE_WMS_BODIES_PLUGIN_HPP
#define CSP_SIMPLE_WMS_BODIES_PLUGIN_HPP

#include "SimpleWMSBody.hpp"

#include "../../../src/cs-core/PluginBase.hpp"

#include <VistaKernel/GraphicsManager/VistaOpenGLNode.h>

namespace csp::simpleWmsBodies {

/// This plugin provides the rendering of planets as spheres with a texture and an additional WMS
/// based texture. Despite its name it can also render moons :P. It can be configured via the 
/// applications config file. See README.md for details.
class Plugin : public cs::core::PluginBase {
 public:
  /// The startup settings of the plugin.
  struct Settings {

    /// The startup settings for a planet.
    struct Body {
      std::vector<Wms> mWms;  ///< The path to surface texture.
      std::string mTexture;   ///< The data sets containing WMS data.
    };

    std::map<std::string, Body> mBodies;
  };

  Plugin();

  void init() override;
  void deInit() override;

 private:
  Settings                                    mPluginSettings;
  std::vector<std::shared_ptr<SimpleWMSBody>> mSimpleWMSBodies;
  std::vector<VistaOpenGLNode*>               mSimpleWMSBodyNodes;
  std::shared_ptr<Properties>                 mProperties;
  std::vector<timeInterval>                   mIntervalsOnTimeline;

  int mActiveBodyConnection = -1;

  /// Add the time intervalls of the current data set to timeline.
  void addTimeIntervall(std::vector<timeInterval> timeIntervals, std::string wmsName, 
      std::string planetName);

  /// Remove the time intervalls of the current data set to timeline.
  void removeTimeIntervall(std::vector<timeInterval> timeIntervals);
};

} // namespace csp::simpleWmsBodies

#endif // CSP_SIMPLE_WMS_BODIES_PLUGIN_HPP
