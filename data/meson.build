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

data_outputs = [
  'grass-2048x1024.jpg',
  'grass-grayscale-2048x1024.png',
  'grass-grayscale-1024x1024.png',
  'grass-grayscale-1024x512.png',
  'grass-grayscale-512x512.png',
  'grass-grayscale-512x256.png',
  'grass-grayscale-256x256.png',
  'grass-grayscale-256x128.png',
  'grass-grayscale-128x128.png',
  'grass-grayscale-128x64.png',
  'grass-grayscale-64x64.png',
  'grass-grayscale-64x32.png',
  'grass-grayscale-32x32.png',
  'grass-grayscale-32x16.png',
  'grass-grayscale-16x16.png',
  'grass-grayscale-16x8.png',
  'grass-grayscale-8x8.png',
  'grass-grayscale-8x4.png',
  'grass-grayscale-4x4.png',
  'grass-grayscale-4x2.png',
  'grass-grayscale-2x2.png',
  'grass-grayscale-2x1.png',
  'grass-grayscale-1x1.png',
  'pink-leaves-2048x1024.jpg',
  'pink-leaves-grayscale-2048x1024.png',
  'pink-leaves-grayscale-1024x1024.png',
  'pink-leaves-grayscale-1024x512.png',
  'pink-leaves-grayscale-512x512.png',
  'pink-leaves-grayscale-512x256.png',
  'pink-leaves-grayscale-256x256.png',
  'pink-leaves-grayscale-256x128.png',
  'pink-leaves-grayscale-128x128.png',
  'pink-leaves-grayscale-128x64.png',
  'pink-leaves-grayscale-64x64.png',
  'pink-leaves-grayscale-64x32.png',
  'pink-leaves-grayscale-32x32.png',
  'pink-leaves-grayscale-32x16.png',
  'pink-leaves-grayscale-16x16.png',
  'pink-leaves-grayscale-16x8.png',
  'pink-leaves-grayscale-8x8.png',
  'pink-leaves-grayscale-8x4.png',
  'pink-leaves-grayscale-4x4.png',
  'pink-leaves-grayscale-4x2.png',
  'pink-leaves-grayscale-2x2.png',
  'pink-leaves-grayscale-2x1.png',
  'pink-leaves-grayscale-1x1.png',
]

gen_image = [prog_python, src_root + '/misc/gen_image']

data_outputs = []

dst = 'grass-2048x1024.jpg'
grass_2048x1024 = custom_target(
  dst,
  input : 'grass-2014x1536.jpg',
  output : dst,
  command : gen_image + ['crop', '@INPUT@', '@OUTPUT@'],
)

data_outputs += grass_2048x1024

gen_sizes = [
  '2048x1024', '1024x1024', '1024x512', '512x512', '512x256', '256x256',
  '256x128', '128x128', '128x64', '64x64', '64x32', '32x32', '32x16', '16x16',
  '16x8', '8x8', '8x4', '4x4', '4x2', '2x2', '2x1', '1x1',
]

foreach s : gen_sizes
  dst = 'grass-grayscale-' + s + '.png'
  img = custom_target(
    dst,
    input : grass_2048x1024,
    output : dst,
    command : gen_image + ['scale', '@INPUT@', '@OUTPUT@'],
  )

  data_outputs += img
endforeach

dst = 'pink-leaves-2048x1024.jpg'
pink_leaves_2048x1024 = custom_target(
  dst,
  input : 'pink-leaves-3264x2448.jpg',
  output : dst,
  command : gen_image + ['crop', '@INPUT@', '@OUTPUT@'],
)

data_outputs += pink_leaves_2048x1024

foreach s : gen_sizes
  dst = 'pink-leaves-grayscale-' + s + '.png'
  img = custom_target(
    dst,
    input : pink_leaves_2048x1024,
    output : dst,
    command : gen_image + ['scale', '@INPUT@', '@OUTPUT@'],
  )

  data_outputs += img
endforeach

data_ref_files = [
  '32x32-green.ref.png',
  '64x64-green.ref.png',
  '128x128-green.ref.png',
  '32x32-smile.ref.png',
  'example.image.copy-ref-image.ref.png',
  'example.image.map-ref-image.ref.png',
  'func.4-vertex-buffers.ref.png',
  'func.depthstencil.basic-depth.clear-0.0.op-greater.ref.png',
  'func.depthstencil.basic-depth.clear-0.0.op-less.ref.png',
  'func.depthstencil.basic-depth.clear-0.5.op-greater-equal.ref.png',
  'func.depthstencil.basic-depth.clear-1.0.op-greater.ref.png',
  'func.desc.dynamic.uniform-buffer.ref.png',
  'func.draw-index16-restart.ref.png',
  'func.draw-index16.ref.png',
  'func.draw-index32-restart.ref.png',
  'func.first.ref.png',
  'func.gs.basic.ref.png',
  'func.interleaved-cmd-buffers.ref.png',
  'func.mesh.basic.ref.png',
  'func.mesh.clipdistance.ref.png',
  'func.mesh.clipdistance_culldistance.ref.png',
  'func.mesh.clipdistance.fs.ref.png',
  'func.mesh.culldistance.ref.png',
  'func.mesh.layer.ref.0.png',
  'func.mesh.layer.ref.1.png',
  'func.mesh.layer.ref.2.png',
  'func.mesh.layer.ref.3.png',
  'func.mesh.multiview.1.1.ref.0.png',
  'func.mesh.multiview.2.10.ref.1.png',
  'func.mesh.multiview.2.11.ref.0.png',
  'func.mesh.multiview.2.11.ref.1.png',
  'func.mesh.multiview.3.110.ref.1.png',
  'func.mesh.multiview.3.110.ref.2.png',
  'func.mesh.multiview.3.111.ref.0.png',
  'func.mesh.multiview.3.111.ref.1.png',
  'func.mesh.multiview.3.111.ref.2.png',
  'func.mesh.multiview.ref.empty.png',
  'func.mesh.multiview.perview.block.2.11.ref.0.png',
  'func.mesh.multiview.perview.block.2.11.ref.1.png',
  'func.mesh.multiview.perview.nonblock.2.11.ref.0.png',
  'func.mesh.multiview.perview.nonblock.2.11.ref.1.png',
  'func.mesh.multiview.clipdistance.2.11.ref.0.png',
  'func.mesh.multiview.clipdistance.2.11.ref.1.png',
  'func.mesh.multiview.culldistance.2.11.ref.0.png',
  'func.mesh.multiview.culldistance.2.11.ref.1.png',
  'func.mesh.primitive_id.fs.ref.png',
  'func.mesh.viewport_index.ref.png',
  'func.mesh.viewport_index.fs.ref.png',
  'func.mesh.viewport_mask.simple.ref.png',
  'func.mesh.viewport_mask.mixed.ref.png',
  'func.multiview.ref.0.png',
  'func.multiview.ref.1.png',
  'func.multiview.ref.2.png',
  'func.multiview.ref.3.png',
  'func.multiview.ref.4.png',
  'func.multiview.ref.5.png',
  'func.multiview.ref.empty.png',
  'func.push-constants.basic.ref.png',
  'func.renderpass.clear.color-render-area.ref.png',
  'grass-2014x1536.jpg',
  'mandrill-128x128.png',
  'mandrill-16384x32.png',
  'mandrill-16x16.png',
  'mandrill-16x8192.png',
  'mandrill-1x1.png',
  'mandrill-256x256.png',
  'mandrill-2x2.png',
  'mandrill-32x16384.png',
  'mandrill-32x32.png',
  'mandrill-4x4.png',
  'mandrill-512x512.png',
  'mandrill-64x64.png',
  'mandrill-8192x16.png',
  'mandrill-8x8.png',
  'mandrill-dxt5-512x512.ktx',
  'pink-leaves-3264x2448.jpg',
]

foreach a : data_ref_files
  data_outputs += custom_target(
    a,
    input : a,
    output : a,
    command : [prog_cp, '@INPUT@', '@OUTPUT@'],
  )
endforeach

subdir('func.depthstencil.stencil-triangles')
