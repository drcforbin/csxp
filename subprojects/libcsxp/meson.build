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

librw_dep = dependency('librw', fallback: ['librw', 'librw_dep'])

inc = include_directories('include')

libcsxp = static_library(
    'csxp', [
        'csxp.cpp',
        'atom.cpp',
        'env.cpp',
        'lib/detail-core.cpp',
        'lib/detail-env.cpp',
        'lib/detail-fn.cpp',
        'lib/detail-lazy.cpp',
        'lib/detail-math.cpp',
        'lib/detail-op.cpp',
        'lib/detail-util.cpp',
        'lib/lib.cpp',
        'reader.cpp',
        version_file
    ],
    dependencies: [librw_dep],
    include_directories : inc,
    install : true
)

libcsxp_dep = declare_dependency(
    include_directories : inc,
    link_with : libcsxp
)

executable(
    'libcsxp-bench', [
        'bench/main.cpp',
        'bench/integration.cpp',
        'bench/reader.cpp',
        'test/test-data.cpp'
    ],
    dependencies: [librw_dep, libcsxp_dep],
    include_directories : inc
)

testexe = executable(
    'libcsxp-test', [
        'test/env.cpp',
        'test/integration.cpp',
        'test/lib-core.cpp',
        'test/lib-fn.cpp',
        'test/lib-math.cpp',
        'test/lib-op.cpp',
        'test/main.cpp',
        'test/reader.cpp',
        'test/run-helpers.cpp',
        'test/test-data.cpp'
    ],
    dependencies: [librw_dep, libcsxp_dep],
    include_directories : inc
)

test('libcsxp test', testexe)

