subdir('collections')
subdir('core')
subdir('io')
subdir('net')
subdir('runtime')
subdir('threading')

test_sources = files('main.c') + test_collections_sources + test_core_sources + test_io_sources + test_net_sources + test_runtime_sources + test_threading_sources

executable('hammer-tests', test_sources, link_with: hammer_lib, include_directories: inc)