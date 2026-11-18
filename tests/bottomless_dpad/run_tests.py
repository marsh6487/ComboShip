#!/usr/bin/env python3
"""Execute native bottle updates, production counter hooks and binding/HUD projection."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
def function(source,name):
    match=re.search(r'^(?:extern "C" )?(?:static )?(?:void|u8|uint8_t|s16|int) '+name+r'\([^;{}]*\)\s*\{',source,re.M)
    assert match,name
    end=match.end();depth=1
    while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]
with tempfile.TemporaryDirectory(prefix='bottomless-dpad-') as temp:
  for host in ('soh','mm'):
    source=(ROOT/host/'mods/items/mm_bottle_items.cpp').read_text()
    counter=(ROOT/host/'mods/items/custom_bottles.cpp').read_text()
    native=(ROOT/host/'src/code/z_parameter.c').read_text()
    blocks='\n'.join(re.findall(r'^#define BC_ITEM_.*$',counter,re.M))+'\n'
    blocks+='\n'.join(function(counter,name) for name in ['Bottle_ContentIsReal','Bottle_ContentMaxUses','Bottle_BottomlessOwned','Bottle_BottomlessContent','Bottle_BottomlessCount','Bottle_BottomlessEmpty','Bottle_BottomlessFill','Bottle_BottomlessSetCount','Bottle_BottomlessConsume'])
    if host=='soh':
      driver=source[source.index('REGISTER_VB_SHOULD(VB_UPDATE_BOTTLE_ITEM, {'):]
      start=driver.index('{');end=start+1;depth=1
      while depth:depth+=(driver[end]=='{')-(driver[end]=='}');end+=1
      body=driver[start:end]
      body=re.sub(r'\s*u8 (?:button|item) = \(u8\)va_arg\(args, int32_t\);','',body)
      driver='void BottomlessCounterGate(u8 button,u8 item,bool* should) '+body
      update=function(native,'Inventory_UpdateBottleItem')
    else:
      driver=function(source,'BottleItems_OnBottleUpdate')
      update=function(native,'Inventory_UpdateBottleItem')+'\n'+function(native,'Inventory_Dpad_UpdateBottleItem')
    fixture=(ROOT/'tests/bottomless_dpad/native_test.cpp').read_text().replace('/* COUNTERS */',blocks).replace('/* SYNC */',function(source,'BottleItems_SyncSlot')).replace('/* DRIVER */',driver).replace('/* UPDATE */',update).replace('/* AMMO */',function(native,'Interface_BottomlessAmmo'))
    path=Path(temp)/f'{host}.cpp';path.write_text(fixture)
    includes=[ROOT,ROOT/host,ROOT/host/'include',ROOT/host/'include/PR',ROOT/host/'src',ROOT/host/'assets',ROOT/host/'mods',ROOT/host/('soh' if host=='soh' else '2s2h'),ROOT/'libultraship/include']
    flags=['-DF3DEX_GBI_2','-DCOMBO_BUILD','-DLOG_LEVEL_GAME_PRINTS=0']
    if host=='mm':flags+=['-DBOTTOMLESS_MM']
    target=Path(temp)/host
    subprocess.run(['c++','-std=c++20','-w',*flags,*['-I'+str(p) for p in includes],'-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',str(path),'-o',str(target)],check=True)
    subprocess.run([str(target)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
