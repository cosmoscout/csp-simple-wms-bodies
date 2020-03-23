////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_SIMPLE_BODIES_PLUGIN_HPP
#define CSP_SIMPLE_BODIES_PLUGIN_HPP

#include "../../../src/cs-core/PluginBase.hpp"
#include "SimpleBody.hpp"

#include <VistaKernel/GraphicsManager/VistaOpenGLNode.h>

namespace csp::simpleWmsBodies {

/// This plugin provides the rendering of planets as spheres with a texture. Despite its name it
/// can also render moons :P. It can be configured via the applications config file. See README.md
/// for details.
class Plugin : public cs::core::PluginBase {
 public:
  struct Settings {

    struct Body {
      std::vector<Wms> mWms;
      std::string mTexture;
    };

    std::map<std::string, Body> mBodies;
  };
  Plugin();
  void init() override;
  void deInit() override;

 private:
  Settings                                 mPluginSettings;
  std::vector<std::shared_ptr<SimpleBody>> mSimpleBodies;
  std::vector<VistaOpenGLNode*>            mSimpleBodyNodes;
  std::shared_ptr<Properties>              mProperties;
  std::vector<timeInterval> mIntervalsOnTimeline;

  void removeTimeIntervall(std::vector<timeInterval> timeIntervals);
  void addTimeIntervall(std::vector<timeInterval> timeIntervals);
};

} // namespace csp::simplebodies

#endif // CSP_SIMPLE_BODIES_PLUGIN_HPP
