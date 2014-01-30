{
  'includes': [ 'common.gypi' ],
  'conditions': [
    ['OS=="win"', {
      'variables': {
        'MAGICK_ROOT%': '<!(python get_regvalue.py)',
        # download the dll binary and check off for libraries and includes
      }
    }, { # 'OS!="win"'
      'variables': {
        'std%':'ansi',
        'OSX_VER%': "<!(sw_vers | grep 'ProductVersion:' | grep -o '[0-9]*\.[0-9]*\.[0-9]$*' | awk '{print substr($1,0,4)}')",
      }
    }]
  ],
  "targets": [
    {
      "target_name": "imagemagick",
      "sources": [ "src/imagemagick.cc" ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      "conditions": [
        ['OSX_VER == "10.9"', {
          'OTHER_CPLUSPLUSFLAGS' : [
            '<!@(Magick++-config --cflags)',
            '-std=c++11',
            '-stdlib=libc++',
          ],
          'cflags_cc' : [
              '-std=c++11',
          ],
          'xcode_settings': {
            #'OTHER_CPLUSPLUSFLAGS':['-std=c++11','-stdlib=libc++'],
            'OTHER_CPLUSPLUSFLAGS':['-std=c++11','-stdlib=libc++','<!@(Magick++-config --cflags)'],
            'OTHER_LDFLAGS':['-stdlib=libc++'],
            'CLANG_CXX_LANGUAGE_STANDARD':'c++11',
            'MACOSX_DEPLOYMENT_TARGET':'10.7'
          }
        }],
        
        ['OS=="win"', {
          "libraries": [
            '-l<(MAGICK_ROOT)/lib/CORE_RL_magick_.lib',
            '-l<(MAGICK_ROOT)/lib/CORE_RL_Magick++_.lib',
            '-l<(MAGICK_ROOT)/lib/CORE_RL_wand_.lib',
            '-l<(MAGICK_ROOT)/lib/X11.lib'
          ],        
          'include_dirs': [
            '<(MAGICK_ROOT)/include',
          ]
        }], ['OS=="win" and target_arch!="x64"', {
          'defines': [
            '_SSIZE_T_',
          ]
        }], ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'OTHER_CFLAGS': [
              '<!@(Magick++-config --cflags)'
            ],
          },
          "libraries": [
             '<!@(Magick++-config --ldflags --libs)',
          ],
          'cflags': [
            '<!@(Magick++-config --cflags --cppflags)'
          ],
        }], ['OS=="linux"', { # not windows not mac
          "libraries": [
            '<!@(Magick++-config --ldflags --libs)',
          ],
          'cflags': [
            '<!@(Magick++-config --cflags --cppflags)'
          ],
        }]
      ]
    }]
  }
