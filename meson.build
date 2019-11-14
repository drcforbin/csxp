project('csxp', 'cpp',
    version: '1.0.0',
    default_options: [
        'cpp_std=c++17',
        'b_lto=true',
        'b_ndebug=if-release'
    ])

libcsxp_proj = subproject('libcsxp')
libcsxp_dep = libcsxp_proj.get_variable('libcsxp_dep')

librw_proj = subproject('librw')
librw_dep = libcsxp_proj.get_variable('librw_dep')

subdir('csxp')
subdir('csxp-test')
subdir('csxp-bench')
