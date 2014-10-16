{
  'variables': {
    'v8_use_snapshot%': 'true',
    'node_use_dtrace%': 'false',
    'node_use_etw%': 'false',
    'node_use_perfctr%': 'false',
    'node_has_winsdk%': 'false',
    'node_shared_v8%': 'false',
    'node_shared_zlib%': 'false',
    'node_shared_http_parser%': 'false',
    'node_shared_cares%': 'false',
    'node_shared_libuv%': 'false',
    'node_use_openssl%': 'true',
    'node_shared_openssl%': 'false',
    'node_use_mdb%': 'false',
    'node_v8_options%': '',
    'v8_postmortem_support': 'false'
  },

  'targets': [
    {
      'target_name': 'test_bin',
      'type': 'none',
      'dependencies': [
        '../third_party/mordor-base/gyp/mordor.gyp:tests_base',
#        '../third_party/v8/src/d8.gyp:d8',
#        '../third_party/v8/samples/samples.gyp:*',
      ],
    },
    {
      'target_name': 'mordor_shell',
      'type': 'none',
      'dependencies': [
        '../test/test.gyp:shell',
      ],
    },
  ],
}

