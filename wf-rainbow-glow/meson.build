project(
    'wayfire-glow-decoration',
    'cpp',
    version: '0.1.0',
    license: 'MIT',
    meson_version: '>=0.51.0',
    default_options: [
        'cpp_std=c++17',
        'warning_level=2',
        'werror=false',
    ],
)

wayfire = dependency('wayfire')
wlroots = dependency('wlroots')
wfconfig = dependency('wf-config')
glm = dependency('glm')

glesv2 = dependency('glesv2')
egl = dependency('egl')

add_project_arguments(
    ['-DWLR_USE_UNSTABLE', '-DWAYFIRE_PLUGIN'],
    language: 'cpp'
)

shared_module(
    'glow-decoration',
    'glow-decoration.cpp',
    dependencies: [wayfire, wlroots, wfconfig, glm, glesv2, egl],
    install: true,
    install_dir: wayfire.get_variable(pkgconfig: 'plugindir'),
)

# Install XML configuration
install_data(
    'glow-decoration.xml',
    install_dir: wayfire.get_variable(pkgconfig: 'metadatadir'),
)
