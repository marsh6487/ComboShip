"""Namespace the accepted animation-only POC7 without changing its motion data."""
from pathlib import Path
import argparse, hashlib, json, struct, zipfile

EXPECTED = '0dd723fc6064d0d9855f2ac55e6e32c768700b839fd76385c9a4853761d73f7e'
DATA = 'objects/optional_zelda_run/poc7/gRunData'
HEADER = 'objects/optional_zelda_run/poc7/gRun'

def build(source, output):
    assert hashlib.sha256(source.read_bytes()).hexdigest() == EXPECTED
    with zipfile.ZipFile(source) as z:
        data = z.read('objects/din_zelda_run_poc7_current_build_20261115/gDinAdultRunData')
        old = z.read('objects/gameplay_keep/gPlayerAnim_link_normal_run_free')
        assert z.read('alt/objects/din_zelda_run_poc7_current_build_20261115/gDinAdultRunData') == data
        assert z.read('alt/objects/gameplay_keep/gPlayerAnim_link_normal_run_free') == old
    assert data[4:8] == b'MAPO' and struct.unpack_from('<I', data, 64)[0] == 20 * 67
    assert old[4:8] == b'MNAO' and struct.unpack_from('<IH', old, 64) == (1, 20)
    path = ('__OTR__' + DATA).encode()
    header = old[:70] + struct.pack('<I', len(path)) + path
    files = {DATA:data, HEADER:header, 'alt/'+DATA:data, 'alt/'+HEADER:header}
    report = {'source_sha256':EXPECTED, 'frames':20, 'words_per_frame':67,
              'motion_payload_byte_identical':True, 'model_resources':0,
              'private_header':HEADER, 'no_shared_animation_overrides':True,
              'runtime_tested':False}
    files['Zelda_Run_Private_POC7_Info.json'] = json.dumps(report, indent=2).encode()
    files['README_Zelda_Run_Private_POC7.txt'] = (
        'OPTIONAL ZELDA RUN POC7 — PRIVATE ANIMATION ONLY\n'
        'Requires the matching Zelda Run checkbox source. Install in mods/soh and mods/2ship\n'
        'for Combo, or the active standalone game mods directory. Remove the old fixed-path\n'
        'POC7 run override and older run archives. Enable Zelda Run and choose Child, Adult\n'
        'or Both (default Both). Off, missing assets or an excluded age keeps your current\n'
        'run. Only the accepted 20-frame empty-handed run is supplied; no held-run, walk,\n'
        'jump-slash, model, skeleton or equipment changes. Independent of outfits.\n'
        'Changes apply when the native locomotion code next selects its run. Child use\n'
        'is a user-authorized selection of the unchanged clip, not a newly fitted pose.\n'
        'Native and Alt aliases are included. Gameplay acceptance remains pending.\n').encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as z:
        for name, blob in sorted(files.items()):
            entry=zipfile.ZipInfo(name,(2026,11,18,0,0,0));entry.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(entry,blob)
    with zipfile.ZipFile(output) as z:
        assert z.read(DATA) == data and z.read('alt/'+DATA) == data
        assert not any('objects/gameplay_keep/' in n for n in z.namelist())
    report['output_sha256']=hashlib.sha256(output.read_bytes()).hexdigest()
    output.with_suffix('.validation.json').write_text(json.dumps(report,indent=2))
    print(json.dumps(report))

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();build(args.source,args.output)
