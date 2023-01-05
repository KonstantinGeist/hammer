test_sources = files(
    'main.c',
    'allocators.c',
    'arrays.c',
    'environment.c',
    'errors.c',
    'hashes.c',
    'hashmaps.c',
    'math.c',
    'modules.c',
    'mutexes.c',
    'queues.c',
    'random.c',
    'readers.c',
    'signatures.c',
    'strings.c',
    'stringbuilders.c',
    'threads.c',
    'utils.c',
    'waitableevents.c',
    'workers.c'
)

executable('hammer-test', test_sources, link_with: hammer_lib, include_directories: inc)