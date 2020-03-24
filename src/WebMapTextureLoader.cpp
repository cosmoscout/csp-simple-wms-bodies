////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "WebMapTextureLoader.hpp"

namespace csp::simpleWmsBodies {

WebMapTextureLoader::WebMapTextureLoader()
: mThreadPool(32) {
 }

WebMapTextureLoader::~WebMapTextureLoader() {

}

bool fileExist(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

std::string WebMapTextureLoader::loadTexture(std::string time, std::string requestStr, std::string name, std::string format) {
    std::string cacheDir = "../share/resources/textures/" + name + "/";
    std::string year;
    std::stringstream time_stringstream(time);
    std::getline(time_stringstream,year, '-');
    cacheDir+= year + "/";
    auto cacheDirPath(boost::filesystem::absolute(boost::filesystem::path(cacheDir)));
    if (!(boost::filesystem::exists(cacheDirPath))) {
      try {
        cs::utils::filesystem::createDirectoryRecursively(
            cacheDirPath, boost::filesystem::perms::all_all);
      } catch (std::exception& e) {
        std::cerr << "Failed to create cache directory: " << e.what() << std::endl;
      }
    }
    requestStr += "&TIME=";
    
    requestStr += time;
    std::replace(time.begin(),time.end(),'/','-');
    std::string cacheFile = cacheDir + time + ".png";
    if(fileExist(cacheFile.c_str())) {
        return cacheFile;
    }
    std::ofstream out;
    out.open(cacheFile, std::ofstream::out | std::ofstream::binary);
    if (!out) {
        std::cerr << "Failed to open for writing!" << std::endl;
    }
    curlpp::Easy request;
    request.setOpt(curlpp::options::Url(requestStr));
    request.setOpt(curlpp::options::WriteStream(&out));
    request.setOpt(curlpp::options::NoSignal(true));
    request.perform();

    out.close();
    if(curlpp::infos::ResponseCode::get(request) == 400) {
        remove(cacheFile.c_str());
        return "Error";
    }
    return cacheFile;
}

std::future<std::string> WebMapTextureLoader::loadTextureAsync(std::string time, std::string requestStr, std::string centerName, std::string format){
    return mThreadPool.enqueue([=]() {
      return loadTexture(time, requestStr, centerName, format);
    });
}

std::future<unsigned char*> WebMapTextureLoader::loadTextureFromFileAsync(std::string fileName) {
    return mThreadPool.enqueue([=]() {
        int  width, height, bpp;
        int channels = 4;
        return stbi_load(fileName.c_str(), &width, &height, &bpp, channels);
    });
}

}// namespace csp::simpleWmsBodies