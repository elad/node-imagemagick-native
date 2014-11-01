module.exports = require(__dirname + '/build/Release/imagemagick.node');

// Idea for stream conversions from
// http://codewinds.com/blog/2013-08-20-nodejs-transform-streams.html
var stream = require('stream');
var util = require('util');

// node v0.10+ use native Transform, else polyfill
var Transform = stream.Transform ||
  require('readable-stream').Transform;

function Convert(options) {
  // allow use without new
  if (!(this instanceof Convert)) {
    return new Convert(options);
  }

  this._options = options;
  this._bufs = [];

  // init Transform
  Transform.call(this, options);
}
util.inherits(Convert, Transform);

Convert.prototype._transform = function (chunk, enc, done) {
  this._bufs.push(chunk);
  done();
};

Convert.prototype._flush = function(done) {
  this._options.srcData = Buffer.concat(this._bufs);
  var self = this;
  module.exports.convert(this._options,function(err,data){
    if(err) {
      return done(err);
    }
    self.push(data);
    done();
  });
}

module.exports.streams = { convert : Convert };
