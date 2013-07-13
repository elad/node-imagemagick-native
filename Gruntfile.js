module.exports = function(grunt) {
  grunt.initConfig({
    pkg: grunt.file.readJSON('package.json')
  });
  grunt.loadNpmTasks('grunt-release');
  grunt.registerTask('default', ['test']);

  grunt.registerTask('test', 'run tests', function() {
    var done = this.async();
    require('child_process').exec(
      'npm test',
      function(error, stdout, stderr) {
        grunt.log.verbose.writeln(stdout);
        grunt.log.verbose.writeln(stderr);
        if (error) {
          grunt.log.error(error);
          done(false);
        }
        else {
          // detect "# tests 12"
          //        "# fail  1"
          var testsLine = stdout.match(/# tests\s+([0-9]+)/),
              failLine  = stdout.match(/# fail\s+([0-9]+)/);
          if (failLine) {
            grunt.fail.warn( failLine[1] + " of " + testsLine[1] + " tests failed." );
            done(false);
            return;
          }
          if (testsLine) {
            grunt.log.oklns( "All " + testsLine[1] + " tests successful." );
          }
          done(true);
        }
      }
    );
  });
};
