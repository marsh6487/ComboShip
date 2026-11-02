#!/usr/bin/env python3
"""Compile exact host callbacks and MM creator, with native initialization boundary."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]


def function(source, signature):
    start=source.index(signature)
    begin=source.index('{',start)
    depth,end=1,begin+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}')
        end+=1
    while source[end:].lstrip().startswith('catch'):
        begin=source.index('{',end)
        depth,end=1,begin+1
        while depth:
            depth+=(source[end]=='{')-(source[end]=='}')
            end+=1
    return source[start:end]


host=(ROOT/'combo/ComboShip.cpp').read_text()
functions='\n'.join(function(host,signature) for signature in (
    'static bool WriteContainerDocument(', 'static bool FlushContainer(',
    'static const char* Combo_ReadGameSave(', 'static void Combo_WriteGameSave(',
    'static int Combo_QueryFileMode(', 'static int Combo_VanillaCreation('))
mm=(ROOT/'mm/2s2h/SaveManager/SaveManager.cpp').read_text()
creator=function(mm,'int SaveManager_InitVanillaForCombo(')
marker=function(mm,'void SaveManager_MarkNoSaveLoaded(')
failed=function(mm,'static int SaveManager_LoadFailedForCombo(')
a=mm.index('int SaveManager_LoadSaveFile(');a=mm.index('{',a)+1;b=mm.index('    std::string fileName',a)
identity='int LoadIdentityForTests(int mmFileNum) {\n'+mm[a:b]+'\nreturn 0;\n}'
discard=function(mm,'void SaveManager_DiscardVanillaCreation(') if 'void SaveManager_DiscardVanillaCreation(' in mm else 'void SaveManager_DiscardVanillaCreation() {}'

code=(ROOT/'tests/combo_vanilla/host_creation_test.cpp').read_text()
code=code.replace('/* HOST_FUNCTIONS */',functions).replace('/* MM_CREATOR */',creator)
code=code.replace('/* MM_INVALIDATION */',marker+'\n'+failed+'\n'+discard+'\n'+identity)
flags=['-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT)]
try:
    import nlohmann_json
    flags+=['-I',str(nlohmann_json.get_include())]
except ImportError:
    pass
if '--sanitize' in sys.argv:
    flags+=['-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-fno-pie','-no-pie']
with tempfile.TemporaryDirectory(prefix='native-vanilla-host-') as directory:
    cpp=Path(directory)/'host.cpp';cpp.write_text(code);exe=Path(directory)/'host'
    subprocess.run([*shlex.split(os.environ.get('CXX','c++')),*flags,str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe),directory],check=True)
