"""Rito identity, native pool/grant, live widget, snapshot and combined-fill checks.

Provide nlohmann's include directory in CPATH, as in the other native test runners.
"""
from pathlib import Path
import os,subprocess,tempfile,sys
ROOT=Path(__file__).resolve().parents[2]
HERE=ROOT/'tests/rito_mask_rando'
for script in ['test_registry.py','test_routes.py','run_behavior_tests.py','run_oot_descriptor_tests.py','run_pool_tests.py']:
 subprocess.run([sys.executable,'-B',str(HERE/script)],check=True)
with tempfile.TemporaryDirectory(prefix='rito-rando-') as td:
 tmp=Path(td)
 source=(ROOT/'combo/gui/ComboWidgetRender.cpp').read_text();start=source.index('void RenderWidget(');end=source.index('{',start)+1;depth=1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 (tmp/'native_widget.inc').write_text(source[start:end])
 source=(ROOT/'combo/tests/combo_shared_test.cpp').read_text();(tmp/'shared_pool_fixture.inc').write_text(source[:source.index('int main()')])
 for name in ['test_settings','test_widget','shared_pool_test']:
  binary=tmp/name
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-O1','-I'+str(ROOT),'-I'+str(ROOT/'soh'),'-I'+str(ROOT/'combo'),'-I'+str(ROOT/'combo/menu'),'-I'+str(ROOT/'libultraship/include'),'-I'+td,str(HERE/(name+'.cpp')),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
