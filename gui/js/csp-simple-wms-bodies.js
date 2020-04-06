/* global IApi, CosmoScout, $ */

(() => {
    /**
     * Simple WMS bodies Api
     */
    class SimpleWMSBodiesApi extends IApi {
      /**
       * @inheritDoc
       */
      name = 'simpleWMSBodies';
  
      /**
       * @inheritDoc
       */
      init() {

      }
  
      /**
       * Sets an elevation data copyright tooltip
       * TODO Remove jQuery
       *
       * @param copyright {string}
       */
      // eslint-disable-next-line class-methods-use-this
      setWMSDataCopyright(copyright) {
        $('#wms-img-data-copyright').tooltip({placement: 'top'}).attr('data-original-title', `© ${copyright}`);
      }
    }
  

    CosmoScout.init(SimpleWMSBodiesApi);
  })();
  