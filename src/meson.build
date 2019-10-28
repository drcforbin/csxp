#version_file = configure_file(
#    input: 'version.cpp.in',
#    output: 'version.cpp',
#    configuration: {
#      'version': meson.project_version()
#    }
#)

common_sources = files(
    'atom.cpp',
    'env.cpp',
    #'csxp.cpp',
    'fmt/format.cc',
    'fmt/posix.cc',
    'lib/detail-core.cpp',
    'lib/detail-fn.cpp',
    'lib/detail-math.cpp',
    'lib/detail-op.cpp',
    'lib/lib.cpp',
    'logging.cpp',
    'parser.cpp',
    'tokenizer.cpp',
)

#xcb_deps = [
#    xcb,
#    xcb_util,
#    xcd_xkb,
#    xkbc_x11
#]

executable(
    'csxp', [
        'csxp-main.cpp',
        common_sources,
        #version_file
    ],
    dependencies: [],
    include_directories: inc
)

testexe = executable(
    'csxp-test', [
        'csxp-test.cpp',
        'env-test.cpp',
        'run-test.cpp',
        'parser-test.cpp',
        'test-clj.cpp',
        'tokenizer-test.cpp',
        common_sources
    ],
    #dependencies: csxp_common_deps,
    include_directories: inc,
    cpp_args : '-DCATCH_CONFIG_ENABLE_BENCHMARKING'
)

test('csxp test', testexe)

