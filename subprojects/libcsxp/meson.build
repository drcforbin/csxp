project('libcsxp', 'cpp',
    version: '1.0.0',
    default_options: [
        'cpp_std=c++17',
        'b_lto=true',
        'b_ndebug=if-release'
    ]
)

version_file = configure_file(
    input: 'version.cpp.in',
    output: 'version.cpp',
    configuration: {
        'version': meson.project_version()
    }
)

inc = include_directories('include')

libcsxp = shared_library(
    'csxp', [
        'csxp.cpp',
        'atom.cpp',
        'env.cpp',
        'fmt/format.cc',
        'fmt/posix.cc',
        'lib/detail-core.cpp',
        'lib/detail-env.cpp',
        'lib/detail-fn.cpp',
        'lib/detail-lazy.cpp',
        'lib/detail-math.cpp',
        'lib/detail-op.cpp',
        'lib/detail-util.cpp',
        'lib/lib.cpp',
        'logging.cpp',
        'reader.cpp',
        version_file
    ],
    include_directories : inc,
    install : true
)

libcsxp_dep = declare_dependency(
    include_directories : inc,
    link_with : libcsxp
)
