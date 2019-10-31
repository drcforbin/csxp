#common_sources = files(
#    'atom.cpp',
#    'env.cpp',
#    'fake-lib.cpp',
#    'fmt/format.cc',
#    'fmt/posix.cc',
#    'lib/detail-core.cpp',
#    'lib/detail-fn.cpp',
#    'lib/detail-math.cpp',
#    'lib/detail-op.cpp',
#    'lib/lib.cpp',
#    'logging.cpp',
#    'parser.cpp',
#    'reader.cpp',
#    'tokenizer.cpp',
#)

testexe = executable(
    'csxp-test', [
        'main.cpp',
        'env.cpp',
        'integration.cpp',
        'map.cpp',
        'lib-math.cpp',
        'lib-op.cpp',
        'run-helpers.cpp',
        'test-data.cpp',
        'reader.cpp',
        common_sources
    ],
    include_directories: inc
)

test('csxp test', testexe)

