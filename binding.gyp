{
  "targets": [
    {
      "target_name": "imagemagick",
      "sources": [ "src/imagemagick.cc" ],
      "link_settings": {
        "libraries": [
          '<!@(Magick-config --ldflags)',
          "-lMagick++"
        ],
      },
      "conditions": [
        ['OS=="mac"', {
          'xcode_settings': {
            'OTHER_CFLAGS': [
              '<!@(Magick-config --cflags)'
            ]
          }
        }, {
          'cflags': [
            '<!@(Magick-config --cflags)'
          ],
        }]
      ]
    }
  ]
}
