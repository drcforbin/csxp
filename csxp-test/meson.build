testexe = executable(
    'csxp-test', [
        'env.cpp',
        'integration.cpp',
        'lib-core.cpp',
        'lib-fn.cpp',
        'lib-math.cpp',
        'lib-op.cpp',
        'main.cpp',
        'map.cpp',
        'reader.cpp',
        'run-helpers.cpp',
        'test-data.cpp'
    ],
    dependencies: [libcsxp_dep]
)

test('csxp test', testexe)

