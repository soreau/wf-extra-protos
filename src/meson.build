sources = ['main.cpp', 'wayfire-desktop/wayfire-desktop.cpp']

extra_protos = shared_module('extra-protos', sources,
    dependencies: [wayfire, wlroots, wf_server_protos],
    install: true, install_dir: join_paths(get_option('libdir'), 'wayfire'))

subdir('wf-menu')