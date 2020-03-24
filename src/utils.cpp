////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "utils.hpp"

namespace csp::simpleWmsBodies {

std::string timeToString(std::string format, boost::posix_time::ptime time) {
    std::stringstream sstr;
    auto              facet = new boost::posix_time::time_facet();
    facet->format(format.c_str());
    sstr.imbue(std::locale(std::locale::classic(), facet));
    sstr << time;

    return sstr.str();
}

void matchDuration(const std::string &input, const std::regex& re, int &duration) {
    std::smatch match;
    std::regex_search(input, match, re);
    if (match.empty()) {
        std::cout << "Pattern do NOT match" << std::endl;
        return;
    }

    std::vector<int> vec = {0,0,0,0,0,0}; // years, months, days, hours, minutes, seconds

    for (size_t i = 1; i < match.size(); ++i) {

        if (match[i].matched) {
            std::string str = match[i];
            str.pop_back(); // remove last character.
            vec[i-1] = std::stod(str);
        }
    }

    duration = 31556926   * vec[0] +  // years  
                   2629744 * vec[1] +  // months
                   86400      * vec[2] +  // days
                   3600       * vec[3] +  // hours
                   60         * vec[4] +  // minutes
                   1          * vec[5];   // seconds

    if (duration == 0) {
        std::cout << "Not valid input" << std::endl;
        return;
    }
}

void timeDuration(std::string isoString, int &duration, std::string &format) {
    std::regex rshort("^((?!T).)*$");
    duration = 0;
    format = "";
    if (std::regex_match(isoString, rshort)) // no T (Time) exist
    {
        std::regex r("P([[:d:]]+Y)?([[:d:]]+M)?([[:d:]]+D)?");
        matchDuration(isoString, r, duration);
    }
    else {

        std::regex r("P([[:d:]]+Y)?([[:d:]]+M)?([[:d:]]+D)?T([[:d:]]+H)?([[:d:]]+M)?([[:d:]]+S|[[:d:]]+\\.[[:d:]]+S)?");
        matchDuration(isoString, r, duration);
    }

    if(duration % 86400 == 0) {
        format = "%Y-%m-%d";
    } else if(duration % 2629744 == 0) {
        format = "%Y-%m";
    } else if(duration % 31556926 == 0) {
        format = "%Y";
    } else {
        format = "%Y-%m-%dT%H:%MZ";
    }
}

void convertIsoDate(std::string date, boost::posix_time::ptime &time) {
    if(date == "current") {
        time = boost::posix_time::microsec_clock::universal_time();
        return;
    }
    date.erase(std::remove_if(date.begin(), date.end(), [](unsigned char x){return std::ispunct(x);}), date.end());
    std::string dateSubStr = date.substr(0, date.find("T"));
    std::size_t pos = date.find("T");
    std::string timeSubStr = "T";
    if(pos != std::string::npos) {
        timeSubStr = date.substr(pos);
    }
    dateSubStr.resize(8, '0');
    timeSubStr.resize(7, '0');
    time = boost::posix_time::from_iso_string(dateSubStr + timeSubStr);
}

void parseIsoString(std::string isoString, std::vector<timeInterval> &timeIntervals) {

    std::string timeRange;
    std::stringstream iso_stringstream(isoString);

    while(std::getline(iso_stringstream,timeRange, ','))
    {
        std::string startDate, endDate, duration;
        std::stringstream timeRange_stringstream(timeRange);
        std::getline(timeRange_stringstream,startDate, '/');
        std::getline(timeRange_stringstream,endDate, '/');
        std::getline(timeRange_stringstream,duration, '/');
        timeInterval tmp;
        boost::posix_time::ptime start, end;
        convertIsoDate(startDate, start);
        if(endDate == "") {
            end = start;
            tmp.mIntervalDuration = 0;
            tmp.mFormat = "%Y-%m-%dT%H:%MZ";
        } else {
            timeDuration(duration, tmp.mIntervalDuration, tmp.mFormat);
            convertIsoDate(endDate, end);
        }

        tmp.endTime = end;
        tmp.startTime = start;
        timeIntervals.push_back(tmp);
    }
}

bool timeInIntervals(boost::posix_time::ptime time, std::vector<timeInterval> &timeIntervals, boost::posix_time::time_duration &timeSinceStart, int &intervalDuration, std::string &format) {
    for(int i=0; i < timeIntervals.size(); i++) {
        boost::posix_time::time_duration td = boost::posix_time::seconds(timeIntervals.at(i).mIntervalDuration);
        if(timeIntervals.at(i).startTime <= time && timeIntervals.at(i).endTime + td >= time) {
            timeSinceStart = time - timeIntervals.at(i).startTime;
            intervalDuration = timeIntervals.at(i).mIntervalDuration;
            format = timeIntervals.at(i).mFormat;
            return true;
        }
    }
    return false;
}


}// namespace csp::simpleWmsBodies