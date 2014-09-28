{
  'targets': [
    {
      'target_name': 'shell',
      'product_name': 'mordor_shell',
      'type': 'executable',
      'dependencies': [
        '../third_party/mordor-base/gyp/mordor.gyp:mordor_base',
        '../third_party/v8/tools/gyp/v8.gyp:v8',
      ],
      'include_dirs': [
        '.',
        '../third_party',
        '../third_party/mordor-base',
        '../third_party/v8',
        '../third_party/v8/include',
      ],
      'sources': [
        'shell.cpp',
      ],
      'link_settings': {
        'cflags': [ '-std=c++11' ],
        'cflags_cc!': [ '-fno-rtti', '-fno-exceptions'],
        'libraries': [
          '-L <(PRODUCT_DIR)',
          '-lmordor_base',
          '-lv8_base',
          '-lv8_snapshot',
          '-lv8_libbase',
          '-lv8_libplatform',
          ],
        },
    },
  ],
}

