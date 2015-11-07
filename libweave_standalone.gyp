# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'includes': [
    'libweave.gypi',
  ],
  'target_defaults': {
    'libraries': [
      '-lcrypto',
      '-lgtest',
      '-lgmock',
      '-lexpat',
      '-lpthread',
    ],
  },
  'targets': [
    {
      'target_name': 'libweave_common',
      'type': 'static_library',
      'include_dirs': [
        '<@(DEPTH)/external',
      ],
      'sources': [
        '<@(weave_sources)',
        '<@(base_sources)',
      ],
    },
    {
      'target_name': 'libweave',
      'type': 'shared_library',
      'include_dirs': [
        '../libweave/external',
      ],
      'dependencies': [
        'libweave_common',
      ],
      'sources': [
        'src/empty.cc'
      ],
    },
    {
      'target_name': 'libweave-test',
      'type': 'static_library',
      'standalone_static_library': 1,
      'include_dirs': [
        '<@(DEPTH)/external',
      ],
      'sources': [
        '<@(weave_test_sources)',
      ],
    },
    {
      'target_name': 'libweave_testrunner',
      'type': 'executable',
      'include_dirs': [
        '<@(DEPTH)/external',
      ],
      'dependencies': [
        'libweave_common',
        'libweave-test',
      ],
      'sources': [
        '<@(weave_unittest_sources)',
        '<@(base_unittests)',
      ],
    },
    {
      'target_name': 'libweave_exports_testrunner',
      'type': 'executable',
      'include_dirs': [
        '<@(DEPTH)/external',
      ],
      'dependencies': [
        'libweave',
        'libweave-test',
      ],
      'sources': [
        '<@(weave_exports_unittest_sources)',
      ],
    },
  ],
}