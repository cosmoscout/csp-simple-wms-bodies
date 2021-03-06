////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_WMS_TEXTURE_LOADER_HPP
#define CSP_WMS_TEXTURE_LOADER_HPP

#include "../../../src/cs-utils/ThreadPool.hpp"

namespace csp::simplewmsbodies {

class WebMapTextureLoader {
 public:
  /// Create a new ThreadPool with the specified amount of threads.
  WebMapTextureLoader();

  ~WebMapTextureLoader();

  /// Async WMS texture loader.
  std::future<std::string> loadTextureAsync(std::string time, std::string requestStr,
      std::string const& layer, std::string const& mapCache);

  /// WMS texture loader.
  std::string loadTexture(std::string time, std::string requestStr, std::string const& layer,
      std::string const& mapCache);

  /// Load WMS texture from file using stbi.
  std::future<unsigned char*> loadTextureFromFileAsync(std::string const& fileName);

 private:
  bool fileExist(const char* fileName);

  cs::utils::ThreadPool mThreadPool;
};

} // namespace csp::simplewmsbodies

#endif // CSP_WMS_TEXTURE_LOADER_HPP