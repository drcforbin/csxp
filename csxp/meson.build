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
    dependencies: [librw_dep, libcsxp_dep],
    include_directories: inc,
    install: true
)
