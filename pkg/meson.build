subdir('collections')
subdir('core')
subdir('io')
subdir('net')
subdir('platform')
subdir('runtime')
subdir('threading')
subdir('vendor')

sources = collections_sources + core_sources + io_sources + net_sources + platform_sources + runtime_sources + threading_sources + sqlite3_sources

thread_dep = dependency('threads')
dl_dep = meson.get_compiler('c').find_library('dl')
deps = [thread_dep, dl_dep]

hammer_lib = static_library('hammer', sources, include_directories: inc, dependencies: deps)