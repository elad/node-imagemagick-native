module.exports = require(__dirname + '/build/Release/imagemagick.node');

// Idea for stream conversions from
// http://codewinds.com/blog/2013-08-20-nodejs-transform-streams.html
var stream = require('stream');
var util = require('util');

function getStreamHandler(method, readableObjectMode) {
  function MagickStream(options) {
    // allow use without new
    if (!(this instanceof MagickStream)) {
      return new MagickStream(options);
    }
    
    options = options || {};
    if(readableObjectMode) options.readableObjectMode = true;
    this._options = options || {};
    this._bufs = [];
    this._method = method;

    // init Transform
    stream.Transform.call(this, options);
  }
  util.inherits(MagickStream, stream.Transform);
  
  MagickStream.prototype._transform = function (chunk, enc, done) {
    this._bufs.push(chunk);
    done();
  };

  MagickStream.prototype._flush = function(done) {
    this._options.srcData = Buffer.concat(this._bufs);
    var self = this;
    this._method(this._options,function(err,data){
      if(err) {
        return done(err);
      }
      self.push(data);
      done();
    });
  }

  return MagickStream;
}

module.exports.streams = { 
  convert: getStreamHandler(module.exports.convert),
  identify: getStreamHandler(module.exports.identify, true)
};
