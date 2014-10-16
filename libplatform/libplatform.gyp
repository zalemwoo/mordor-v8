{
  'targets': [
    {
      'target_name': 'mordor_libplatform',
      'product_name': 'mordor_platform',
      'type': 'static_library',
      'dependencies': [
        '../third_party/mordor-base/gyp/mordor.gyp:mordor_base',
        '../third_party/v8/mordor_v8_patch/gen/v8.gyp:v8',
      ],
      'include_dirs': [
        '.',
        '../third_party',
        '../third_party/mordor-base',
        '../third_party/v8',
      ],
      'sources': [
        'md_platform.cpp',
      ],
      'cflags': [ '-std=c++11' ],
      'cflags_cc!': [ '-fno-rtti', '-fno-exceptions'],
      'xcode_settings': {
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
        'GCC_ENABLE_CPP_RTTI': 'YES',              # -fno-rtti
        'MACOSX_DEPLOYMENT_TARGET': '10.8',        # OS X Deployment Target: 10.8
        'CLANG_CXX_LANGUAGE_STANDARD': 'c++11',
        'CLANG_CXX_LIBRARY': 'libc++',             # libc++ requires OS X 10.7 or later
      },
    },
  ],
}

