subdir('collections')
subdir('core')
subdir('io')
subdir('runtime')
subdir('vendor')

sources = collections_sources + core_sources + io_sources + runtime_sources + sqlite3_sources

thread_dep = dependency('threads')
dl_dep = meson.get_compiler('c').find_library('dl')
deps = [thread_dep, dl_dep]

hammer_lib = static_library('hammer', sources, include_directories: inc, dependencies: deps)