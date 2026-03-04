project('queue', 'c', 
        version : '1.1.2',
        default_options : ['c_std=c11'])

v_arr = meson.project_version().split('.')
major = v_arr[0]
minor = v_arr[1]
patch = v_arr[2]

conf_data = configuration_data()
conf_data.set('VERSION', meson.project_version())
conf_data.set('MAJOR', major)
conf_data.set('MINOR', minor)
conf_data.set('PATCH', patch)

configure_file(input : 'src/queue_version.h.in',
               output : 'queue_version.h',
               configuration : conf_data)

inc = include_directories('src', '.')

queue_lib = both_libraries('queue',
                        'src/queue.c',
                        version: meson.project_version(),
                        soversion: major,
                        include_directories: inc)

subdir('test')
