project('hammer', 'c',
    version: '1.0',
    meson_version: '>= 0.50.0'
)

add_global_arguments(['-Werror', '-Wall', '-pedantic'], language: 'c')

inc = [include_directories('pkg')]

subdir('pkg')
subdir('cmd')