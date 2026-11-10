"""Read ZIP/O2R or MPQ/OTR inputs without modifying or repackaging them."""
import ctypes
from pathlib import Path
import zipfile


class ReadOnlyArchive:
    def __init__(self, path, stormlib=None):
        self.path = Path(path)
        self.zip = zipfile.ZipFile(path) if zipfile.is_zipfile(path) else None
        self.handle = None
        if self.zip:
            self.names = {n for n in self.zip.namelist() if not n.endswith('/')}
            return
        if not stormlib:
            raise ValueError('MPQ/OTR validation requires --stormlib pointing to the built native StormLib shared library')
        self.lib = ctypes.CDLL(str(stormlib))
        handle, word, pointer = ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER
        signatures = {
            'SFileOpenArchive': ([ctypes.c_char_p, word, word, pointer(handle)], ctypes.c_bool),
            'SFileOpenFileEx': ([handle, ctypes.c_char_p, word, pointer(handle)], ctypes.c_bool),
            'SFileGetFileSize': ([handle, pointer(word)], word),
            'SFileReadFile': ([handle, ctypes.c_void_p, word, pointer(word), ctypes.c_void_p], ctypes.c_bool),
            'SFileCloseFile': ([handle], ctypes.c_bool),
            'SFileCloseArchive': ([handle], ctypes.c_bool),
        }
        for name, (args, result) in signatures.items():
            try:
                function = getattr(self.lib, name)
            except AttributeError:
                function = getattr(self.lib, 'Soul_' + name)
                setattr(self.lib, name, function)
            function.argtypes, function.restype = args, result
        self.handle = handle()
        if not self.lib.SFileOpenArchive(str(self.path).encode(), 0, 0x100, ctypes.byref(self.handle)):
            raise ValueError('StormLib could not open archive read-only: ' + self.path.name)
        self.names = {n.replace('\\', '/') for n in self.read('(listfile)').decode('utf-8').splitlines() if n}

    def read(self, name):
        if self.zip:
            return self.zip.read(name)
        file_handle = ctypes.c_void_p()
        if not self.lib.SFileOpenFileEx(self.handle, name.encode(), 0, ctypes.byref(file_handle)):
            raise KeyError(name)
        try:
            high = ctypes.c_uint32()
            size = self.lib.SFileGetFileSize(file_handle, ctypes.byref(high))
            if high.value or size == 0xFFFFFFFF:
                raise ValueError('unsupported archive entry size: ' + name)
            data = ctypes.create_string_buffer(size)
            read = ctypes.c_uint32()
            if not self.lib.SFileReadFile(file_handle, data, size, ctypes.byref(read), None) or read.value != size:
                raise ValueError('short archive entry read: ' + name)
            return data.raw
        finally:
            self.lib.SFileCloseFile(file_handle)

    def close(self):
        if self.zip:
            self.zip.close()
        elif self.handle:
            self.lib.SFileCloseArchive(self.handle)
            self.handle = None
