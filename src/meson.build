#version_file = configure_file(
#    input: 'version.cpp.in',
#    output: 'version.cpp',
#    configuration: {
#      'version': meson.project_version()
#    }
#)

executable(
    'csxp', [
        'csxp-main.cpp',
        'fake-lib.cpp',
        common_sources,
        #version_file
    ],
    dependencies: [],
    include_directories: inc
)

