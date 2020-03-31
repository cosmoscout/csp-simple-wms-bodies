# Simple WMS bodies for CosmoScout VR

A CosmoSout VR plugin which renders simple spherical celestial bodies with WMS based textures. The bodies are drawn as an ellipsoid with an equirectangular texture.

## Configuration

This plugin can be enabled with the following configuration in your `settings.json`:

```javascript
{
  ...
  "plugins": {
    ...
    "csp-simple-wms-bodies": {
      "bodies": {
        <anchor name>: {
          "texture": <string>,        // The path to surface texture
          "wms": [
            {
              "name": <string>,       // The name of the data set as shown in the UI.
              "copyright": <string>,  // The copyright holder of the data set (also shown in the UI).
              "url": <string>,        // The URL of the mapserver including the "SERVICE=wms" parameter.
              "width": <int>,         // The width of the WMS image.
              "height": <int>,        // The height of the WMS image.
              "time": <string>,       // Time intervals of WMS images.
              "layers": <string>,     // A comma,seperated list of WMS layers.
              "preFetch": <int>       // The amount of textures that gets prefetched in every direction.
            },
            ... <more WMS datasets> ...
          ]
        },
        ... <more bodies> ...
      }
    }
  }
}
```

**More in-depth information and some tutorials will be provided soon.**

## MIT License

Copyright (c) 2019 German Aerospace Center (DLR)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
