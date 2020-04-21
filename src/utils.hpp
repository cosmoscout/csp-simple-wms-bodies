////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_WMS_UTILS_HPP
#define CSP_WMS_UTILS_HPP

#include "../../../src/cs-utils/convert.hpp"

#include <optional>
#include <regex>

namespace csp::simplewmsbodies {

/// Struct of timeintervals of the data set.
struct TimeInterval {
  boost::posix_time::ptime mStartTime;
  boost::posix_time::ptime mEndTime;
  std::string              mFormat;
  int                      mIntervalDuration;
};

/// A single WMS data set.
struct WMSConfig {
  std::string mName;      ///< The name of the data set as shown in the UI.
  std::string mCopyright; ///< The copyright holder of the data set (also shown in the UI).
  std::string mUrl;       ///< The URL of the mapserver including the "SERVICE=wms" parameter.
  int         mWidth;     ///< The width of the WMS image.
  int         mHeight;    ///< The height of the WMS image.
  std::optional<std::string> mTime; ///< Time intervals of WMS images.
  std::optional<int>
              mPrefetchCount; ///< The amount of textures that gets prefetched in every direction.
  std::string mLayers;        ///< A comma,seperated list of WMS layers.
};

namespace utils {

std::string timeToString(std::string const& format, boost::posix_time::ptime time);
void        matchDuration(std::string const& input, std::regex const& re, int& duration);
void        timeDuration(std::string const& isoString, int& duration, std::string& format);
void        convertIsoDate(std::string& date, boost::posix_time::ptime& time);
void        parseIsoString(std::string const& isoString, std::vector<TimeInterval>& timeIntervals);
bool        timeInIntervals(boost::posix_time::ptime time, std::vector<TimeInterval>& timeIntervals,
           boost::posix_time::time_duration& timeSinceStart, int& intervalDuration, std::string& format);

} // namespace utils

} // namespace csp::simplewmsbodies

#endif // CSP_WMS_UTILS_HPP