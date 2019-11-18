project('csxp', 'cpp',
    version: '1.0.0',
    default_options: [
        'cpp_std=c++17',
        'b_lto=true',
        'b_ndebug=if-release'
    ])

librw_proj = subproject('librw')
librw_dep = librw_proj.get_variable('librw_dep')

libcsxp_inc = include_directories('include')

subdir('libcsxp')
subdir('csxp')
