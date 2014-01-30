{
  'target_defaults': {
      'default_configuration': 'Release',
      'configurations': {
          'Debug': {
              'defines!': [ 'NDEBUG' ],
              'cflags_cc!': ['-O3', '-Os', '-DNDEBUG'],
              'xcode_settings': {
                'OTHER_CPLUSPLUSFLAGS!':['-O3', '-Os', '-DDEBUG'],
                'GCC_OPTIMIZATION_LEVEL': '0',
                'GCC_GENERATE_DEBUGGING_SYMBOLS': 'YES'
              }
          },
          'Release': {
              'defines': [
                'NDEBUG'
              ],
              'xcode_settings': {
                'OTHER_CPLUSPLUSFLAGS!':['-Os', '-O2'],
                'OTHER_CPLUSPLUSFLAGS':[],
                'GCC_OPTIMIZATION_LEVEL': '3',
                'GCC_GENERATE_DEBUGGING_SYMBOLS': 'NO',
                'DEAD_CODE_STRIPPING':'YES',
                'GCC_INLINES_ARE_PRIVATE_EXTERN':'YES',
                'OTHER_LDFLAGS': [
                    '-s' # warns 'option -s is obsolete and being ignored' but actually works
                ]
              },
              'ldflags': [
                    '-Wl,-s'
              ]
          }
      }
  }
}