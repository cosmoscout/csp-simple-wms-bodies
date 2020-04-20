////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Plugin.hpp"

#include "../../../src/cs-core/GraphicsEngine.hpp"
#include "../../../src/cs-core/GuiManager.hpp"
#include "../../../src/cs-core/InputManager.hpp"
#include "../../../src/cs-core/SolarSystem.hpp"
#include "../../../src/cs-core/TimeControl.hpp"
#include "../../../src/cs-utils/logger.hpp"
#include "../../../src/cs-utils/utils.hpp"

#include <VistaKernel/GraphicsManager/VistaSceneGraph.h>
#include <VistaKernel/GraphicsManager/VistaTransformNode.h>
#include <VistaKernelOpenSGExt/VistaOpenSGMaterialTools.h>
#include <VistaOGLExt/VistaTexture.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT_FN cs::core::PluginBase* create() {
  return new csp::simplewmsbodies::Plugin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT_FN void destroy(cs::core::PluginBase* pluginBase) {
  delete pluginBase;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace csp::simplewmsbodies {

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(const nlohmann::json& j, WMSConfig& o) {
  o.mName          = cs::core::parseProperty<std::string>("name", j);
  o.mCopyright     = cs::core::parseProperty<std::string>("copyright", j);
  o.mUrl           = cs::core::parseProperty<std::string>("url", j);
  o.mWidth         = cs::core::parseProperty<int>("width", j);
  o.mHeight        = cs::core::parseProperty<int>("height", j);
  o.mTime          = cs::core::parseOptional<std::string>("time", j);
  o.mPrefetchCount = cs::core::parseOptional<int>("preFetch", j);
  o.mLayers        = cs::core::parseProperty<std::string>("layers", j);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(const nlohmann::json& j, Plugin::Settings::Body& o) {
  o.mWMS     = cs::core::parseVector<WMSConfig>("wms", j);
  o.mTexture = cs::core::parseProperty<std::string>("texture", j);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(const nlohmann::json& j, Plugin::Settings& o) {
  cs::core::parseSection("csp-simple-wms-bodies",
      [&] { o.mBodies = cs::core::parseMap<std::string, Plugin::Settings::Body>("bodies", j); });
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Plugin::Plugin()
    : mProperties(std::make_shared<Properties>()) {
  // Create default logger for this plugin.
  spdlog::set_default_logger(cs::utils::logger::createLogger("csp-simple-wms-bodies"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::init() {
  spdlog::info("Loading plugin...");

  mPluginSettings = mAllSettings->mPlugins.at("csp-simple-wms-bodies");

  mGuiManager->addPluginTabToSideBarFromHTML(
      "WMS", "panorama", "../share/resources/gui/wms_body_tab.html");
  mGuiManager->addSettingsSectionToSideBarFromHTML(
      "WMS", "panorama", "../share/resources/gui/wms_settings.html");
  mGuiManager->addScriptToGuiFromJS("../share/resources/gui/js/csp-simple-wms-bodies.js");

  // Set whether to interpolate textures between timesteps.
  mGuiManager->getGui()->registerCallback("simpleWmsBodies.setEnableInterpolation",
      "Enables or disables interpolation.",
      std::function([this](bool enable) { mProperties->mEnableInterpolation = enable; }));

  // Set whether to display the entire timespan.
  mGuiManager->getGui()->registerCallback("simpleWmsBodies.setEnableTimeSpan",
      "Enables or disables timespan.",
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

    auto body = std::make_shared<SimpleWMSBody>(mGraphicsEngine, mSolarSystem,
        anchor->second.mCenter, bodySettings.second.mTexture, anchor->second.mFrame,
        tStartExistence, tEndExistence, bodySettings.second.mWMS, mTimeControl, mProperties);

    mSolarSystem->registerBody(body);
    mInputManager->registerSelectable(body);

    body->setSun(mSolarSystem->getSun());
    auto parent = mSceneGraph->NewOpenGLNode(mSceneGraph->GetRoot(), body.get());
    mSimpleWMSBodyNodes.push_back(parent);
    mSimpleWMSBodies.push_back(body);

    VistaOpenSGMaterialTools::SetSortKeyOnSubtree(
        parent, static_cast<int>(cs::utils::DrawOrder::ePlanets));
  }

  mActiveBodyConnection = mSolarSystem->pActiveBody.connectAndTouch(
      [this](std::shared_ptr<cs::scene::CelestialBody> const& body) {
        auto simpleWMSBody = std::dynamic_pointer_cast<SimpleWMSBody>(body);

        // Remove time intervalls of the old body.
        removeTimeIntervall(mIntervalsOnTimeline);

        if (!simpleWMSBody) {
          return;
        }

        mGuiManager->getGui()->callJavascript(
            "CosmoScout.gui.clearDropdown", "simpleWmsBodies.setWMS");
        for (auto const& wms : simpleWMSBody->getWMSs()) {
          bool active = wms.mName == simpleWMSBody->getActiveWMS().mName;
          mGuiManager->getGui()->callJavascript("CosmoScout.gui.addDropdownValue",
              "simpleWmsBodies.setWMS", wms.mName, wms.mName, active);
          if (active) {
            mGuiManager->getGui()->callJavascript(
                "CosmoScout.simpleWMSBodies.setWMSDataCopyright", wms.mCopyright);

            mIntervalsOnTimeline = simpleWMSBody->getTimeIntervals();
            addTimeIntervall(mIntervalsOnTimeline, wms.mName, simpleWMSBody->getCenterName());
          }
        }
      });

  mGuiManager->getGui()->registerCallback("simpleWmsBodies.setWMS",
      "Set the current planet's WMS source to the one with the given name.",
      std::function([this](std::string&& name) {
        auto body = std::dynamic_pointer_cast<SimpleWMSBody>(mSolarSystem->pActiveBody.get());
        if (body) {
          removeTimeIntervall(mIntervalsOnTimeline);
          body->setActiveWMS(name);
          auto wms = body->getActiveWMS();

          mGuiManager->getGui()->callJavascript(
              "CosmoScout.simpleWMSBodies.setWMSDataCopyright", wms.mCopyright);

          mIntervalsOnTimeline = body->getTimeIntervals();
          addTimeIntervall(mIntervalsOnTimeline, name, body->getCenterName());
        }
      }));

  spdlog::info("Loading done.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::removeTimeIntervall(std::vector<TimeInterval> timeIntervals) {
  for (int i = 0; i < timeIntervals.size(); i++) {
    std::string start = utils::timeToString("%Y-%m-%dT%H:%M", timeIntervals.at(i).mStartTime);
    std::string end   = utils::timeToString("%Y-%m-%dT%H:%M", timeIntervals.at(i).mEndTime);
    if (start == end) {
      end = "";
    }
    std::string id = "wms" + start + end;
    mGuiManager->removeEventFromTimenavigationBar(id);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::addTimeIntervall(
    std::vector<TimeInterval> timeIntervals, std::string wmsName, std::string planetName) {
  for (int i = 0; i < timeIntervals.size(); i++) {
    std::string start = utils::timeToString("%Y-%m-%dT%H:%M", timeIntervals.at(i).mStartTime);
    std::string end   = utils::timeToString("%Y-%m-%dT%H:%M", timeIntervals.at(i).mEndTime);
    if (start == end) {
      end = "";
    }
    std::string id = "wms" + start + end;
    mGuiManager->addEventToTimenavigationBar(
        start, end, id, "Valid WMS Time", "border-color: green", wmsName, planetName, "");
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Plugin::deInit() {
  spdlog::info("Unloading plugin...");

  for (auto const& simpleWMSBody : mSimpleWMSBodies) {
    mSolarSystem->unregisterBody(simpleWMSBody);
    mInputManager->unregisterSelectable(simpleWMSBody);
  }

  for (auto const& simpleWMSBodyNode : mSimpleWMSBodyNodes) {
    mSceneGraph->GetRoot()->DisconnectChild(simpleWMSBodyNode);
  }

  mSolarSystem->pActiveBody.disconnect(mActiveBodyConnection);

  mGuiManager->getGui()->unregisterCallback("simpleWmsBodies.setEnableInterpolation");
  mGuiManager->getGui()->unregisterCallback("simpleWmsBodies.setEnableTimeSpan");
  mGuiManager->getGui()->unregisterCallback("simpleWmsBodies.setWMS");

  spdlog::info("Unloading done.");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::simplewmsbodies
