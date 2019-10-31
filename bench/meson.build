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

executable(
    'csxp-bench', [
        'main.cpp',
        #'env.cpp',
        'integration.cpp',
        'parser.cpp',
        'map.cpp',
        '../test/test-data.cpp',
        'reader.cpp',
        'tokenizer.cpp',
        common_sources
    ],
    include_directories: inc
)

