////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
//      and may be used under the terms of the MIT license. See the LICENSE file for details.     //
//                        Copyright: (c) 2019 German Aerospace Center (DLR)                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CSP_WMS_TEXTURE_LOADER_HPP
#define CSP_WMS_TEXTURE_LOADER_HPP

#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <stb_image.h>
#include <stb_image_write.h>

#include "../../../src/cs-utils/filesystem.hpp"
#include "../../../src/cs-utils/ThreadPool.hpp"

#include "utils.hpp"

namespace csp::simpleWmsBodies {

class WebMapTextureLoader {
 public:
  /// Creates a new ThreadPool with the specified amount of threads.
  WebMapTextureLoader();
  ~WebMapTextureLoader();

  std::future<std::string> loadTextureAsync(std::string time, std::string requestStr, std::string centerName, std::string format);
  std::string loadTexture(std::string time, std::string requestStr, std::string name, std::string format);

  std::future<unsigned char*>loadTextureFromFileAsync(std::string fileName);

 private:
    cs::utils::ThreadPool mThreadPool;
};

} // namespace csp::simpleWmsBodies

#endif // CSP_WMS_TEXTURE_LOADER_HPP