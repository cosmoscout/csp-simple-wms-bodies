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
  std::future<std::string> loadTextureAsync(
      std::string time, std::string requestStr, std::string centerName, std::string format);

  /// WMS texture loader.
  std::string loadTexture(
      std::string time, std::string requestStr, std::string name, std::string format);

  /// Load WMS texture from file using stbi.
  std::future<unsigned char*> loadTextureFromFileAsync(std::string fileName);

 private:
  cs::utils::ThreadPool mThreadPool;
};

} // namespace csp::simplewmsbodies

#endif // CSP_WMS_TEXTURE_LOADER_HPP