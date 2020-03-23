////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Plugin.hpp"

#include "../../../src/cs-core/InputManager.hpp"
#include "../../../src/cs-core/Settings.hpp"
#include "../../../src/cs-core/SolarSystem.hpp"
#include "../../../src/cs-core/GuiManager.hpp"

#include <VistaKernel/GraphicsManager/VistaSceneGraph.h>
#include <VistaKernel/GraphicsManager/VistaTransformNode.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT_FN cs::core::PluginBase* create() {
  return new csp::simpleWmsBodies::Plugin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT_FN void destroy(cs::core::PluginBase* pluginBase) {
  delete pluginBase;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace csp::simpleWmsBodies {

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(const nlohmann::json& j, Wms& o) {
  o.mName = cs::core::parseProperty<std::string>("name", j);
  o.mCopyringht = cs::core::parseProperty<std::string>("copyright", j);
  o.mUrl = cs::core::parseProperty<std::string>("url", j);
  o.mWidth = cs::core::parseProperty<int>("width", j);
  o.mHeight = cs::core::parseProperty<int>("height", j);
  o.mTime = cs::core::parseOptional<std::string>("time", j);
  o.preFetch = cs::core::parseOptional<int>("preFetch", j);
  o.mLayers = cs::core::parseProperty<std::string>("layers", j);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(const nlohmann::json& j, Plugin::Settings::Body& o) {
  o.mWms = cs::core::parseVector<Wms>("wms", j);
  o.mTexture = cs::core::parseProperty<std::string>("texture", j);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(const nlohmann::json& j, Plugin::Settings& o) {
  cs::core::parseSection("csp-simple-wms-bodies",
      [&] { o.mBodies = cs::core::parseMap<std::string, Plugin::Settings::Body>("bodies", j); });
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Plugin::Plugin() {
  mProperties = std::make_shared<Properties>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::init() {
  std::cout << "Loading: CosmoScout VR Plugin Simple WMS Bodies" << std::endl;

  mPluginSettings = mAllSettings->mPlugins.at("csp-simple-wms-bodies");

  mGuiManager->addPluginTabToSideBarFromHTML(
      "WMS", "panorama", "../share/resources/gui/wms_body_tab.html");
  mGuiManager->addSettingsSectionToSideBarFromHTML(
      "WMS", "panorama", "../share/resources/gui/wms_settings.html");

  //mGuiManager->getSideBar()->registerCallback<bool>(
  //    "set_enable_interpolation", ([this](bool value) { mProperties->mEnableInterpolation = value; }));

  //mGuiManager->getSideBar()->registerCallback<bool>(
  //    "set_enable_timespan", ([this](bool value) { mProperties->mEnableTimespan = value; }));

  for (auto const& bodySettings : mPluginSettings.mBodies) {
    auto anchor = mAllSettings->mAnchors.find(bodySettings.first);
    
    if (anchor == mAllSettings->mAnchors.end()) {
      throw std::runtime_error(
          "There is no Anchor \"" + bodySettings.first + "\" defined in the settings.");
    }

    auto   existence       = cs::core::getExistenceFromSettings(*anchor);
    double tStartExistence = existence.first;
    double tEndExistence   = existence.second;
    
    std::shared_ptr<csp::simpleWmsBodies::SimpleBody> body;

      body = std::make_shared<SimpleBody>(anchor->second.mCenter, bodySettings.second.mTexture,
        anchor->second.mFrame, tStartExistence, tEndExistence, bodySettings.second.mWms, mTimeControl, mProperties);

    mSolarSystem->registerBody(body);
    mInputManager->registerSelectable(body);

    body->setSun(mSolarSystem->getSun());
    mSimpleBodyNodes.push_back(mSceneGraph->NewOpenGLNode(mSceneGraph->GetRoot(), body.get()));
    mSimpleBodies.push_back(body);
  }
  /* mSolarSystem->pActiveBody.onChange().connect(
      [this](std::shared_ptr<cs::scene::CelestialBody> const& body) {
        auto simpleBody = std::dynamic_pointer_cast<SimpleBody>(body);

        if (!simpleBody) {
          return;
        }
        removeTimeIntervall(mIntervalsOnTimeline);
        // mGuiManager->getSideBar()->callJavascript("clear_container", "set_wms");
        for (auto const& wms : simpleBody->getWms()) {
          bool active = wms.mName == simpleBody->getActiveWms().mName;
          // mGuiManager->getSideBar()->callJavascript(
              "add_dropdown_value", "set_wms", wms.mName, wms.mName, active);
          if(active) {
            mIntervalsOnTimeline = simpleBody->getTimeIntervals();
            addTimeIntervall(mIntervalsOnTimeline);
          }
        }
      }); */

  /* mGuiManager->getSideBar()->registerCallback<std::string>(
        "set_wms", ([this](std::string const& name) {
          auto body = std::dynamic_pointer_cast<SimpleBody>(mSolarSystem->pActiveBody.get());
          if (body) {
            removeTimeIntervall(mIntervalsOnTimeline);
            body->setActiveWms(name);
            mIntervalsOnTimeline = body->getTimeIntervals();
            addTimeIntervall(mIntervalsOnTimeline);
          }
        })); */
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::removeTimeIntervall(std::vector<timeInterval> timeIntervals) {
  for(int i=0; i < timeIntervals.size(); i++) {
    std::string start = timeToString("%Y-%m-%dT%H:%M", timeIntervals.at(i).startTime);
    std::string end = timeToString("%Y-%m-%dT%H:%M", timeIntervals.at(i).endTime);
    if(start == end) {
      end = "";
    }
    std::string id = "wms" + start + end;
    // mGuiManager->getTimeline()->callJavascript("remove_item", id);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::addTimeIntervall(std::vector<timeInterval> timeIntervals) {
  for(int i=0; i < timeIntervals.size(); i++) {
    std::string start = timeToString("%Y-%m-%dT%H:%M", timeIntervals.at(i).startTime);
    std::string end = timeToString("%Y-%m-%dT%H:%M", timeIntervals.at(i).endTime);
    if(start == end) {
      end = "";
    }
    std::string id = "wms" + start + end;
    // mGuiManager->getTimeline()->callJavascript("add_item", start, end, id, "Valid Time", "border-color: green",
    //   "", "", "");
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::deInit() {
  for (auto const& simpleBody : mSimpleBodies) {
    mSolarSystem->unregisterBody(simpleBody);
    mInputManager->unregisterSelectable(simpleBody);
  }

  for (auto const& simpleBodyNode : mSimpleBodyNodes) {
    mSceneGraph->GetRoot()->DisconnectChild(simpleBodyNode);
  }
  
  // mGuiManager->getSideBar()->unregisterCallback("set_enable_interpolation");
  // mGuiManager->getSideBar()->unregisterCallback("set_enable_timespan");
  // mGuiManager->getSideBar()->unregisterCallback("set_wms");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::simplebodies
