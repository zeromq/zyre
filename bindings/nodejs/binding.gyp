{
  'targets': [
    {
      'target_name': 'zyre',
      'sources': [
          'binding.cc'
      ],
      'include_dirs': [
          "<!(node -e \"require('nan')\")",
          '../../include'
      ],
      'dependencies': [
          '../../../zyre/builds/gyp/project.gyp:libzyre'
      ]
    }
  ]
}
