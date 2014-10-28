{
  'targets': [
    {
      'target_name': 'shell',
      'product_name': 'mordor_shell',
      'type': 'executable',
      'dependencies': [
        '../third_party/openssl/openssl.gyp:openssl',
        '../third_party/openssl/openssl.gyp:openssl-cli',
        '../third_party/mordor-base/gyp/mordor.gyp:mordor_base',
        '../third_party/v8/mordor_v8_patch/gen/v8.gyp:v8',
        '../third_party/v8/mordor_v8_patch/gen/v8.gyp:v8_libplatform',
      ],
      'include_dirs': [
        '.',
        '..',
        '../third_party',
        '../third_party/mordor-base',
        '../third_party/v8/mordor_v8_patch',
        '../third_party/v8',
        '../third_party/v8/include',
      ],
      'sources': [
        './js_objects/process.cpp',
        './md_runner.cpp',
        './md_readline.cpp',
        './md_shell.cpp',
        './md_env.cpp',
        './md_v8_wrapper.cpp',
        './md_task_queue.cpp',
        './md_worker.cpp',
      ],
      'cflags': [ '-std=c++11' ],
      'cflags_cc!': [ '-fno-rtti', '-fno-exceptions'],
      'link_settings': {
        'libraries': [
          '-L<(PRODUCT_DIR)',
          '-lreadline',
          '-lhistory',
          '-ldl',
          ],
        },
      'xcode_settings': {
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
        'GCC_ENABLE_CPP_RTTI': 'YES',              # -fno-rtti
        'MACOSX_DEPLOYMENT_TARGET': '10.8',        # OS X Deployment Target: 10.8
        'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
        'CLANG_CXX_LIBRARY': 'libc++', # libc++ requires OS X 10.7 or later
        'OTHER_LDFLAGS': [
          '-Wl,-force_load,<(PRODUCT_DIR)/libopenssl.a',
         ],
      },
      'conditions': [
        ['OS in "linux freebsd"', {
          'ldflags': [
            '-Wl,--whole-archive <(PRODUCT_DIR)/libopenssl.a -Wl,--no-whole-archive',
           ],
        }],
       ],
    },
  ],
}

