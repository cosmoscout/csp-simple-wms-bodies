////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Plugin.hpp"

#include "../../../src/cs-core/InputManager.hpp"
#include "../../../src/cs-core/GuiManager.hpp"
#include "../../../src/cs-utils/utils.hpp"
#include "../../../src/cs-utils/logger.hpp"

#include <VistaKernel/GraphicsManager/VistaSceneGraph.h>
#include <VistaKernel/GraphicsManager/VistaTransformNode.h>
#include <VistaKernelOpenSGExt/VistaOpenSGMaterialTools.h>

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
  o.mCopyright = cs::core::parseProperty<std::string>("copyright", j);
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
      "WMS", "panorama", "../share/resources/gui/wms_settings.html");                   // TODO: is it necessary?

  mGuiManager->getGui()->registerCallback("wms.setEnableInterpolation",
      "Enables or disables interpolation.",                                             // TODO: interpolation of what?
      std::function([this](bool enable) { mProperties->mEnableInterpolation = enable; }));

  mGuiManager->getGui()->registerCallback("wms.setEnableTimeSpan",
      "Enables or disables timespan.",                                                  // TODO: timespan of what?
      std::function([this](bool enable) { mProperties->mEnableTimespan = enable; }));

  for (auto const& bodySettings : mPluginSettings.mBodies) {
    auto anchor = mAllSettings->mAnchors.find(bodySettings.first);
    
    if (anchor == mAllSettings->mAnchors.end()) {
      throw std::runtime_error(
          "There is no Anchor \"" + bodySettings.first + "\" defined in the settings.");
    }

    auto   existence       = cs::core::getExistenceFromSettings(*anchor);
    double tStartExistence = existence.first;
    double tEndExistence   = existence.second;

    auto body = std::make_shared<SimpleBody>(mGraphicsEngine, mSolarSystem, anchor->second.mCenter, bodySettings.second.mTexture,
      anchor->second.mFrame, tStartExistence, tEndExistence, bodySettings.second.mWms, mTimeControl, mProperties);

    mSolarSystem->registerBody(body);
    mInputManager->registerSelectable(body);

    body->setSun(mSolarSystem->getSun());
    auto parent = mSceneGraph->NewOpenGLNode(mSceneGraph->GetRoot(), body.get());
    mSimpleBodyNodes.push_back(parent);
    mSimpleBodies.push_back(body);

    VistaOpenSGMaterialTools::SetSortKeyOnSubtree(
        parent, static_cast<int>(cs::utils::DrawOrder::ePlanets));
  }

  mActiveBodyConnection = mSolarSystem->pActiveBody.connectAndTouch(
      [this](std::shared_ptr<cs::scene::CelestialBody> const& body) {
        auto simpleBody = std::dynamic_pointer_cast<SimpleBody>(body);

        if (!simpleBody) {
          return;
        }

        removeTimeIntervall(mIntervalsOnTimeline);
        mGuiManager->getGui()->callJavascript(
            "CosmoScout.gui.clearDropdown", "wms.setTilesImg");
        for (auto const& wms : simpleBody->getWms()) {
          bool active = wms.mName == simpleBody->getActiveWms().mName;
          mGuiManager->getGui()->callJavascript("CosmoScout.gui.addDropdownValue",
            "wms.setTilesImg", wms.mName, wms.mName, active);
          if(active) {
            // TODO: Copyright doesn't change
            std::string javaCode = "$('#wms-img-data-copyright').tooltip({title: `© " + wms.mCopyright + "`, placement: 'top'})";
            mGuiManager->getGui()->executeJavascript(javaCode);

            mIntervalsOnTimeline = simpleBody->getTimeIntervals();
            addTimeIntervall(mIntervalsOnTimeline);
          }
        }
      });

  mGuiManager->getGui()->registerCallback("wms.setTilesImg",
        "Set the current planet's wms channel to the TileSource with the given name.",        // TODO: correct
         std::function([this](std::string&& name) {
          auto body = std::dynamic_pointer_cast<SimpleBody>(mSolarSystem->pActiveBody.get());
          if (body) {
            removeTimeIntervall(mIntervalsOnTimeline);
            body->setActiveWms(name);
            mIntervalsOnTimeline = body->getTimeIntervals();
            addTimeIntervall(mIntervalsOnTimeline);
          }
        }));

  spdlog::info("Loading done.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::removeTimeIntervall(std::vector<timeInterval> timeIntervals) {
  for(int i=0; i < timeIntervals.size(); i++) {
    std::string start = utils::timeToString("%Y-%m-%dT%H:%M", timeIntervals.at(i).startTime);
    std::string end = utils::timeToString("%Y-%m-%dT%H:%M", timeIntervals.at(i).endTime);
    if(start == end) {
      end = "";
    }
    std::string id = "wms" + start + end;
    // mGuiManager->getTimeline()->callJavascript("remove_item", id);       // TODO: fix
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::addTimeIntervall(std::vector<timeInterval> timeIntervals) {
  for(int i=0; i < timeIntervals.size(); i++) {
    std::string start = utils::timeToString("%Y-%m-%dT%H:%M", timeIntervals.at(i).startTime);
    std::string end = utils::timeToString("%Y-%m-%dT%H:%M", timeIntervals.at(i).endTime);
    if(start == end) {
      end = "";
    }
    std::string id = "wms" + start + end;
    mGuiManager->addEventToTimenavigationBar(start, end, id, "Valid Time", "border-color: green", "", "", "");
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::deInit() {
  spdlog::info("Unloading plugin...");

  for (auto const& simpleBody : mSimpleBodies) {
    mSolarSystem->unregisterBody(simpleBody);
    mInputManager->unregisterSelectable(simpleBody);
  }

  for (auto const& simpleBodyNode : mSimpleBodyNodes) {
    mSceneGraph->GetRoot()->DisconnectChild(simpleBodyNode);
  }

  mSolarSystem->pActiveBody.disconnect(mActiveBodyConnection);
  
  mGuiManager->getGui()->unregisterCallback("wms.setEnableInterpolation");
  mGuiManager->getGui()->unregisterCallback("wms.setEnableTimeSpan");
  mGuiManager->getGui()->unregisterCallback("wms.setTilesImg");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::simpleWmsBodies
