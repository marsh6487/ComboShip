#!/usr/bin/env python3
"""Compile/link generated shader variants in a surfaceless OpenGL ES 3 context.

Usage: python3 compile_glsl.py /path/to/run_shader_tests_output
This validates shader compilation only. It does not render a game or validate driver output.
"""
from ctypes import CDLL, POINTER, byref, c_char_p, c_int, c_uint, c_void_p, create_string_buffer
from pathlib import Path
import sys

EGL_NONE = 0x3038
EGL_PLATFORM_SURFACELESS_MESA = 0x31DD
egl = CDLL('libEGL.so.1')
gl = CDLL('libGLESv2.so.2')


def bind(library, name, result, *arguments):
    method = getattr(library, name)
    method.restype = result
    method.argtypes = list(arguments)
    return method


def ints(*values):
    return (c_int * len(values))(*values)


get_display = bind(egl, 'eglGetPlatformDisplay', c_void_p, c_uint, c_void_p, POINTER(c_int))
initialize = bind(egl, 'eglInitialize', c_uint, c_void_p, POINTER(c_int), POINTER(c_int))
bind_api = bind(egl, 'eglBindAPI', c_uint, c_uint)
choose_config = bind(egl, 'eglChooseConfig', c_uint, c_void_p, POINTER(c_int), POINTER(c_void_p), c_int, POINTER(c_int))
create_surface = bind(egl, 'eglCreatePbufferSurface', c_void_p, c_void_p, c_void_p, POINTER(c_int))
create_context = bind(egl, 'eglCreateContext', c_void_p, c_void_p, c_void_p, c_void_p, POINTER(c_int))
make_current = bind(egl, 'eglMakeCurrent', c_uint, c_void_p, c_void_p, c_void_p, c_void_p)
destroy_context = bind(egl, 'eglDestroyContext', c_uint, c_void_p, c_void_p)
destroy_surface = bind(egl, 'eglDestroySurface', c_uint, c_void_p, c_void_p)
terminate = bind(egl, 'eglTerminate', c_uint, c_void_p)
get_error = bind(egl, 'eglGetError', c_uint)
new_shader = bind(gl, 'glCreateShader', c_uint, c_uint)
shader_source = bind(gl, 'glShaderSource', None, c_uint, c_int, POINTER(c_char_p), POINTER(c_int))
compile_shader = bind(gl, 'glCompileShader', None, c_uint)
shader_info = bind(gl, 'glGetShaderiv', None, c_uint, c_uint, POINTER(c_int))
shader_log = bind(gl, 'glGetShaderInfoLog', None, c_uint, c_int, POINTER(c_int), c_void_p)
delete_shader = bind(gl, 'glDeleteShader', None, c_uint)
new_program = bind(gl, 'glCreateProgram', c_uint)
attach_shader = bind(gl, 'glAttachShader', None, c_uint, c_uint)
link_program = bind(gl, 'glLinkProgram', None, c_uint)
program_info = bind(gl, 'glGetProgramiv', None, c_uint, c_uint, POINTER(c_int))
program_log = bind(gl, 'glGetProgramInfoLog', None, c_uint, c_int, POINTER(c_int), c_void_p)
delete_program = bind(gl, 'glDeleteProgram', None, c_uint)
get_string = bind(gl, 'glGetString', c_char_p, c_uint)


def require(result, message):
    if not result:
        raise RuntimeError(f'{message}; EGL error {get_error():#x}')


def compile(path, stage):
    shader = new_shader(stage)
    text = path.read_bytes()
    sources = (c_char_p * 1)(text)
    lengths = (c_int * 1)(len(text))
    shader_source(shader, 1, sources, lengths)
    compile_shader(shader)
    status = c_int()
    shader_info(shader, 0x8B81, byref(status))
    if not status.value:
        log = create_string_buffer(16384)
        shader_log(shader, len(log), None, log)
        delete_shader(shader)
        raise RuntimeError(f'{path.name}: {log.value.decode()}')
    return shader


display = get_display(EGL_PLATFORM_SURFACELESS_MESA, None, None)
require(display, 'No surfaceless EGL display')
require(initialize(display, None, None), 'Cannot initialize EGL')
context = surface = None
try:
    require(bind_api(0x30A0), 'Cannot bind OpenGL ES API')
    config, count = c_void_p(), c_int()
    require(choose_config(display, ints(0x3033, 1, 0x3040, 0x40, 0x3025, 24, 0x3026, 8, EGL_NONE),
                          byref(config), 1, byref(count)), 'Cannot choose config')
    require(count.value, 'No depth/stencil ES 3 config')
    surface = create_surface(display, config, ints(0x3057, 1, 0x3056, 1, EGL_NONE))
    require(surface, 'Cannot create pbuffer')
    context = create_context(display, config, None, ints(0x3098, 3, EGL_NONE))
    require(context, 'Cannot create ES 3 context')
    require(make_current(display, surface, surface, context), 'Cannot activate ES 3 context')
    variants = sorted(Path(sys.argv[1]).glob('*.vert'))
    if not variants:
        raise RuntimeError('No generated .vert shaders')
    for vertex in variants:
        shaders = []
        program = None
        try:
            shaders.append(compile(vertex, 0x8B31))
            shaders.append(compile(vertex.with_suffix('.frag'), 0x8B30))
            program = new_program()
            for shader in shaders:
                attach_shader(program, shader)
            link_program(program)
            status = c_int()
            program_info(program, 0x8B82, byref(status))
            if not status.value:
                log = create_string_buffer(16384)
                program_log(program, len(log), None, log)
                raise RuntimeError(f'{vertex.stem}: link {log.value.decode()}')
        finally:
            if program:
                delete_program(program)
            for shader in shaders:
                delete_shader(shader)
    print(f'PASS {len(variants)} OpenGL ES 3 shader programs compiled and linked ({get_string(0x1F01).decode()})')
finally:
    make_current(display, None, None, None)
    if context:
        destroy_context(display, context)
    if surface:
        destroy_surface(display, surface)
    terminate(display)
