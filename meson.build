project('hammer', 'c',
    version: '1.0',
    meson_version: '>= 0.50.0',
    default_options: ['c_std=c11']
)

add_global_arguments(['-Werror', '-pedantic'], language: 'c')

if host_machine.system() == 'linux'
    add_global_arguments(['-DHM_UNIX'], language: 'c')
endif

inc = [include_directories('pkg')]

subdir('pkg')
subdir('cmd')