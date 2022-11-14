test_sources = files(
    'main.c',
    'allocators.c',
    'arrays.c',
    'errors.c',
    'hashes.c',
    'hashmaps.c',
    'modules.c',
    'mutexes.c',
    'queues.c',
    'readers.c',
    'strings.c',
    'utils.c',
    'waitobjects.c'
)

executable('hammer-test', test_sources, link_with: hammer_lib, include_directories: inc)