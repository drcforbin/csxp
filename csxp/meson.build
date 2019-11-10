version_file = configure_file(
    input: 'version.cpp.in',
    output: 'version.cpp',
    configuration: {
      'version': meson.project_version()
    }
)

inc = include_directories('include')

executable(
    'csxp', [
        'main.cpp',
        version_file
    ],
    dependencies: [libcsxp_dep],
    include_directories: inc,
    install: true
)
