test_sources = files(
    'main.c',
    'allocators.c',
    'arrays.c',
    'environment.c',
    'errors.c',
    'hashes.c',
    'hashmaps.c',
    'modules.c',
    'mutexes.c',
    'queues.c',
    'random.c',
    'readers.c',
    'strings.c',
    'utils.c',
    'waitobjects.c',
    'threads.c'
)

executable('hammer-test', test_sources, link_with: hammer_lib, include_directories: inc)