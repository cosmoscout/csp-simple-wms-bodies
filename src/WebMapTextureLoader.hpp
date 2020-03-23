#ifndef CSP_WMS_TEXTURE_LOADER_HPP
#define CSP_WMS_TEXTURE_LOADER_HPP


#include <mutex>
#include <filesystem>
#include <VistaOGLExt/VistaTexture.h>
#include <boost/filesystem.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Info.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>
#include <fstream>
#include <sstream>
#include <stb_image.h>
#include <stb_image_write.h>

#include "../../../src/cs-utils/utils.hpp"
#include "../../../src/cs-utils/filesystem.hpp"
#include "../../../src/cs-utils/convert.hpp"
#include "../../../src/cs-graphics/TextureLoader.hpp"

#include "utils.h"

#include "ThreadPool.hpp"

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
    ThreadPool mThreadPool;
};

} // namespace csp::simpleWmsBodies

#endif // CSP_WMS_TEXTURE_LOADER_HPP