"""Keep standalone native regressions on the combined game's graphics ABI."""

import os
from pathlib import Path
import shlex
import shutil
import sys
import tempfile


COMPILER_NAMES = ("cc", "c++", "gcc", "g++", "clang", "clang++")
ABI_FLAGS = ("-DGBI_FLOAT_MTX", "-DGBI_S32_VTX")


def _resolve_command(value, search_path):
    # Some fixtures consume CC/CXX as one literal executable path; others allow
    # a shell-style command plus flags. Accept a real path before splitting it.
    executable = shutil.which(value, path=search_path)
    if executable is not None:
        return [os.path.abspath(executable)]
    command = shlex.split(value)
    if not command:
        raise ValueError("A native compiler command cannot be empty")
    executable = shutil.which(command[0], path=search_path)
    if executable is None:
        return None
    return [os.path.abspath(executable), *command[1:]]


def _write_adapter(path, command):
    path.write_text(
        f"#!{sys.executable}\n"
        "import os\nimport sys\n"
        f"command = {command!r}\n"
        "if os.environ.get('COMBO_NATIVE_TEST_ABI') != 'explicit':\n"
        f"    command += {list(ABI_FLAGS)!r}\n"
        "os.execv(command[0], [*command, *sys.argv[1:]])\n"
    )
    path.chmod(0o755)


class NativeTestCompilers:
    """Own temporary adapters for one runner invocation without mutating its env."""

    def __init__(self, environment):
        self.environment = dict(environment)
        self.temporary = None
        self.task_count = 0

    def __enter__(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="combo-native-abi-")
        return self

    def __exit__(self, *exception):
        self.temporary.cleanup()

    def environment_for_task(self, task):
        environment = {**self.environment, **task.get("env", {})}
        native_abi = task.get("native_abi", environment.get("COMBO_NATIVE_TEST_ABI", "combined"))
        if native_abi not in ("combined", "explicit"):
            raise ValueError(f"Unknown native_abi mode: {native_abi!r}")
        environment["COMBO_NATIVE_TEST_ABI"] = native_abi
        search_path = environment.get("PATH", os.defpath)

        # Resolve every target before placing any adapters on this task's PATH.
        # Absolute exec targets prevent a named adapter from finding itself.
        commands = {}
        for name in COMPILER_NAMES:
            command = _resolve_command(name, search_path)
            if command is not None:
                commands[name] = command
        configured = {}
        for variable, fallback in (("CC", "cc"), ("CXX", "c++")):
            command = _resolve_command(environment.get(variable, fallback), search_path)
            if command is not None:
                configured[variable] = command

        self.task_count += 1
        directory = Path(self.temporary.name) / str(self.task_count)
        directory.mkdir()
        for name, command in commands.items():
            _write_adapter(directory / name, command)
        for variable, command in configured.items():
            # Avoid case-folded CC/cc collisions on case-insensitive filesystems.
            adapter = directory / ("configured-" + variable.lower())
            _write_adapter(adapter, command)
            environment[variable] = str(adapter)
        environment["PATH"] = str(directory) + os.pathsep + search_path
        return environment
