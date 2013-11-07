'use strict';

var os = require('os');
var fs = require('fs');
var path = require('path');
var spawn = require('child_process').spawn;

var install = {};

install.main = function () {
  if (install.isOnWindows()) {
    install.copyAllDllFilesFromLibPathToReleaseFolder(install.done);
  }
};

install.isOnWindows = function () {
  return /^win/.test(os.platform());
};

install.copyAllDllFilesFromLibPathToReleaseFolder = function (cb) {
  this.getLibPath(function (libPath) {
    install.copyDllFiles(libPath, cb);
  });
};

install.getLibPath = function (cb) {
  var getRegVal = spawn('python', ['get_regvalue.py']);
  getRegVal.stdout.on('data', function (libPath) {
    cb(libPath.toString('utf8').replace(/[\r\n]/g, ''));
  });
};

install.copyDllFiles = function (libPath, cb) {
  fs.readdir(libPath, function (err, files) {
    if (err) {
      return cb(err);
    }
	var releaseFolder = path.join(__dirname, 'build/Release');
    var fileLen = files.length;
    var count = 0;
    var done = function (err) {
      if (err) {
        return cb(err);
      }
      count += 1;
      if (count === fileLen) {
        cb();
      }
    };
    files.forEach(function (file) {
      if (install.isDllFile(file)) {
        install.copyFile(path.join(libPath, file), path.join(releaseFolder, file), done);
      } else {
        done();
      }
    });
  });
};

install.isDllFile = function (file) {
  return /\.dll$/.test(file);
};

install.copyFile = function (source, target, cb) {
  cb.__called = false;

  console.log('  %scopy%s: %s => %s', '\x1B[33m', '\x1B[39m', source, target);
  var read = fs.createReadStream(source, {flags: 'r'});
  var write = fs.createWriteStream(target, {flags: 'w'});
  var done = function (err, val) {
    if (!cb.__called) {
      cb.__called = true;
      cb();
    }
  };
  
  read.on('error', done);
  write.on('error', done);
  write.on('close', function () {
    done();
  });
  
  read.pipe(write);
};

install.done = function (err) {
  if (err) {
    console.error(err);
    console.log();
    console.log('  %sFailed%s', '\x1B[31m', '\x1B[39m');
    console.log();
    process.exit(1);
  } else {
    console.log();
  }
};

install.main();
