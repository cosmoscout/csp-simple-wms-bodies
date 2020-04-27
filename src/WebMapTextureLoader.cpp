////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "WebMapTextureLoader.hpp"

#include "../../../src/cs-utils/convert.hpp"
#include "../../../src/cs-utils/filesystem.hpp"
#include "../../../src/cs-utils/logger.hpp"

#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

namespace csp::simplewmsbodies {

////////////////////////////////////////////////////////////////////////////////////////////////////

WebMapTextureLoader::WebMapTextureLoader()
    : mThreadPool(32) {
}

////////////////////////////////////////////////////////////////////////////////////////////////////

WebMapTextureLoader::~WebMapTextureLoader() {
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool fileExist(const char* fileName) {
  std::ifstream infile(fileName);
  return infile.good();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::string WebMapTextureLoader::loadTexture(std::string time, std::string requestStr,
    std::string const& name, std::string const& format, std::string const& mapCache) {
  std::string       cacheDir = mapCache + name + "/";
  std::string       year;
  std::stringstream time_stringstream(time);

  // Create dir for year.
  std::getline(time_stringstream, year, '-');
  cacheDir += year + "/";
  auto cacheDirPath(boost::filesystem::absolute(boost::filesystem::path(cacheDir)));

  if (!(boost::filesystem::exists(cacheDirPath))) {
    try {
      cs::utils::filesystem::createDirectoryRecursively(
          cacheDirPath, boost::filesystem::perms::all_all);
    } catch (std::exception& e) {
      spdlog::error("Failed to create cache directory: '{}'!", e.what());
    }
  }

  requestStr += "&TIME=";

  requestStr += time;
  std::replace(time.begin(), time.end(), '/', '-');
  std::string cacheFile = cacheDir + time + ".png";

  // No need to download the file if it is already in cache.
  if (fileExist(cacheFile.c_str())) {
    return cacheFile;
  }

  // Create cache file for write.
  std::ofstream out;
  out.open(cacheFile, std::ofstream::out | std::ofstream::binary);

  if (!out) {
    spdlog::error("Failed to open '{}' for writing!", cacheFile);
  }

  curlpp::Easy request;
  request.setOpt(curlpp::options::Url(requestStr));
  request.setOpt(curlpp::options::WriteStream(&out));
  request.setOpt(curlpp::options::NoSignal(true));

  // Load to cache file.
  try {
    request.perform();
  } catch (std::exception& e) {
    spdlog::error("Failed to load '{}'! Exception: '{}'", requestStr, e.what());
    out.close();
    remove(cacheFile.c_str());
    return "Error";
  }

  out.close();

  if (curlpp::infos::ResponseCode::get(request) == 400) {
    remove(cacheFile.c_str());
    return "Error";
  }

  return cacheFile;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::future<std::string> WebMapTextureLoader::loadTextureAsync(std::string time,
    std::string requestStr, std::string const& centerName, std::string const& format,
    std::string const& mapCache) {
  return mThreadPool.enqueue(
      [=]() { return loadTexture(time, requestStr, centerName, format, mapCache); });
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::future<unsigned char*> WebMapTextureLoader::loadTextureFromFileAsync(
    std::string const& fileName) {
  return mThreadPool.enqueue([=]() {
    int width, height, bpp;
    int channels = 4;
    return stbi_load(fileName.c_str(), &width, &height, &bpp, channels);
  });
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::simplewmsbodies