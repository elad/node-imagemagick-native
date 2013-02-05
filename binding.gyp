{
  "targets": [
    {
      "target_name": "imagemagick",
      "sources": [ "src/imagemagick.cc" ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      "link_settings": {
        "libraries": [
          '<!@(Magick++-config --ldflags)',
        ],
      },
      "conditions": [
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'OTHER_CFLAGS': [
              '<!@(Magick++-config --cflags)'
            ]
          }
        }, {
          'cflags': [
            '<!@(Magick++-config --cflags)'
          ],
        }]
      ]
    }
  ]
}
