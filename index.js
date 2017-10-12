module.exports = require(__dirname + '/build/Release/imagemagick.node');

// Idea for stream conversions from
// http://codewinds.com/blog/2013-08-20-nodejs-transform-streams.html
var stream = require('stream');
var util = require('util');

function Convert(options) {
  // allow use without new
  if (!(this instanceof Convert)) {
    return new Convert(options);
  }

  this._options = options;
  this._bufs = [];

  // init Transform
  stream.Transform.call(this, options);
}
util.inherits(Convert, stream.Transform);

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

function promisify(func) {
  return function(options) {
    return new Promise((resolve, reject) => {
      func(options, function(err, buff) {
        if (err) {
          return reject(err);
        }
        resolve(buff);
      });
    });
  };
}

module.exports.promises = {
  convert: promisify(module.exports.convert),
  identify: promisify(module.exports.identify),
  composite: promisify(module.exports.composite),
};
