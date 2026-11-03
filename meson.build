# Copyright © 2021 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

project(
  'crucible',
  ['c'],
  license : 'MIT',
  meson_version : '>= 1.1',
  default_options : ['buildtype=debugoptimized', 'b_ndebug=if-release', 'c_std=c11']
)

cc = meson.get_compiler('c')

add_project_arguments(
  [
    '-D_DEFAULT_SOURCE',
    '-D_GNU_SOURCE',
    '-D_POSIX_C_SOURCE=200809L',
    '-D_XOPEN_SOURCE=700',
  ],
  language : ['c']
)

# Generator to run *_gen.py to create *_gen.h

src_root = meson.project_source_root()
prog_python = import('python').find_installation('python3', modules : ['mako'])
if not prog_python.language_version().version_compare('>= 3.10')
  error('Python 3.10 or newer is required')
endif
mako_version = run_command(
  prog_python, '-c', 'import mako; print(mako.__version__)',
  check : true
).stdout().strip()
if not mako_version.version_compare('>= 0.8')
  error('Mako 0.8 or newer is required')
endif

gen_py_to_gen_h = generator(
  prog_python,
  output : '@BASENAME@.h',
  arguments : ['@INPUT@', '@OUTPUT@'],
)

feature_sources = custom_target(
  'feature-descriptions',
  input : 'include/vulkan/vk.xml',
  output : ['t_features_gen.h', 't_features_gen.c'],
  command : [prog_python, files('misc/gen_features.py'),
             '@INPUT@', '@OUTPUT0@', '@OUTPUT1@'],
)

test_sources = []

# Generator to create *-spirv.h.

prog_glslang = find_program('glslangValidator')
prog_spirv_as = find_program('spirv-as')

glslang_version = run_command(prog_glslang, '--version', capture: true, check: true).stdout().split()[2].split(':')[1]
message('glslangValidator version: ' + glslang_version)

if glslang_version.version_compare('>=11.12.0')
  with_VK_EXT_mesh_shader = true
else
  with_VK_EXT_mesh_shader = false
  warning('Disabling VK_EXT_mesh_shader because GLSLang version ' + glslang_version + ' is too old')
endif

c_to_spirv_h = generator(
  prog_python,
  output : '@BASENAME@-spirv.h',
  arguments : [meson.project_source_root() / 'misc/shader_scraper.py',
               '--mode=glsl',
               '--with-glslang=' + prog_glslang.full_path(),
               '-o', '@OUTPUT@', '@INPUT@'],
)

c_to_spvasm_h = generator(
  prog_python,
  output : '@BASENAME@-spvasm.h',
  arguments : [meson.project_source_root() / 'misc/shader_scraper.py',
               '--mode=spvasm',
               '--with-spirv-as=' + prog_spirv_as.full_path(),
               '-o', '@OUTPUT@', '@INPUT@'],
)

fs = import('fs')

subdir('data')
subdir('doc')
subdir('src/cmd')
subdir('src/framework')
subdir('src/qonos')
subdir('src/tests')
subdir('src/util')

inc_include = include_directories('include')

dep_libpng16 = dependency('libpng16')
dep_libxml2 = dependency('libxml-2.0')
dep_m = cc.find_library('m', required : false)
dep_thread = dependency('threads')
dep_vulkan = dependency('vulkan')

executable(
  'crucible',
  [command_sources, data_outputs, feature_sources, framework_sources, qonos_sources,
   test_sources, util_sources],
  include_directories : [inc_include],
  dependencies: [dep_libpng16, dep_libxml2, dep_m, dep_thread, dep_vulkan],
)
