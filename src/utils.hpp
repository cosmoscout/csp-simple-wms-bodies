////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_WMS_UTILS_HPP
#define CSP_WMS_UTILS_HPP

#include <string>
#include <boost/filesystem.hpp>
#include <iostream>
#include <vector>
#include <regex>
#include <optional>

#include "../../../src/cs-utils/utils.hpp"
#include "../../../src/cs-utils/convert.hpp"

namespace csp::simpleWmsBodies {

struct timeInterval {
    boost::posix_time::ptime startTime;
    boost::posix_time::ptime endTime;
    std::string mFormat;
    int mIntervalDuration;
};

struct Wms {
    std::string mName;
    std::string mCopyright;
    std::string mUrl;
    int mWidth;
    int mHeight;
    std::optional<std::string> mTime;
    std::optional<int> preFetch;
    std::string mLayers;
};

std::string timeToString(std::string format, boost::posix_time::ptime time);
void timeDuration(std::string isoString, int &duration, std::string &format);
void parseIsoString(std::string isoString, std::vector<timeInterval> &timeIntervals);
bool timeInIntervals(boost::posix_time::ptime time, std::vector<timeInterval> &timeIntervals, boost::posix_time::time_duration &timeSinceStart, int &intervalDuration, std::string &format);

}// namespace csp::simpleWmsBodies

#endif // CSP_WMS_UTILS_HPP