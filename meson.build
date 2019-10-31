project('csxp', ['c', 'cpp'],
    version: '1.0.0',
    default_options: [
        'cpp_std=c++17',
        'b_lto=true',
        'b_ndebug=if-release'
    ])

inc = include_directories('include')

common_sources = files(
    'src/atom.cpp',
    'src/env.cpp',
    'src/fmt/format.cc',
    'src/fmt/posix.cc',
    'src/lib/detail-core.cpp',
    'src/lib/detail-fn.cpp',
    'src/lib/detail-math.cpp',
    'src/lib/detail-op.cpp',
    'src/lib/lib.cpp',
    'src/logging.cpp',
    'src/reader.cpp'
)


subdir('bench')
subdir('src')
subdir('test')

