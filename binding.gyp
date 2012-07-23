{
  "targets": [
    {
      "target_name": "imagemagick",
      "sources": [ "src/imagemagick.cc" ],
      "link_settings": {
        "libraries": [
          "-lMagick++",
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
