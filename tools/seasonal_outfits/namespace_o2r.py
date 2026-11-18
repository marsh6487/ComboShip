"""Route accepted adult Din POC3 models into private seasonal resource roots.

No mesh, rig, material, texture or animation authoring occurs here. The only
binary edits are resource references; shared gear is extracted once unchanged.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import zipfile

ODLT=0x4F444C54
OSKL=0x4F534B4C
OSLB=0x4F534C42
OARR=0x4F415252
OTEX=0x4F544558
OMTX=0x4F4D5458
OANM=0x4F414E4D
WIDE_OPS=(0x20,0x31,0x32,0x33,0x35,0x36,0x37)
EXPECTED={
    'spring':'b6f76c303cd1ecd489410fcdf60c488b72d1262fee377a4381623514f0a7a945',
    'autumn':'7c431a2d788bccce29b8fc044c706d8c417c41ec55be0ad6083166aa84bf2ac6',
    'winter':'40def18796b4269a5a8e21988dedd75fcf7227c87648a330d5300dedd5b4f498',
}
CRC_POLY=0x42F0E1EBA9EA3693


def digest(data):
    return hashlib.sha256(data).hexdigest()


def crc_table():
    table=[]
    for value in range(256):
        crc=value<<56
        for _ in range(8):
            crc=((crc<<1)^CRC_POLY)&0xffffffffffffffff if crc>>63 else (crc<<1)&0xffffffffffffffff
        table.append(crc)
    return table


CRC_TABLE=crc_table()


def crc64(path):
    crc=0xffffffffffffffff
    for value in path.encode('ascii'):
        crc=CRC_TABLE[((crc>>56)^value)&255]^((crc<<8)&0xffffffffffffffff)
    return crc


def resource_type(data):
    if len(data)>=64 and data[0] in (0,1):
        assert data[0]==0, 'These accepted source packs use little endian only'
        return struct.unpack_from('<I',data,4)[0]
    return None


def dl_commands(data):
    assert resource_type(data)==ODLT
    offset=72
    while offset<len(data):
        start=offset
        w0,w1=struct.unpack_from('<II',data,offset);offset+=8
        op=w0>>24;reference=None
        if op in WIDE_OPS:
            hi,lo=struct.unpack_from('<II',data,offset);reference=hi<<32|lo;offset+=8
        yield start,offset,op,reference
        if op==0xdf:
            assert offset==len(data), 'Unparsed trailing display-list bytes'
            return
    raise AssertionError('Unterminated display list')


class StringCursor:
    def __init__(self,data,offset):
        self.data=data;self.offset=offset;self.output=bytearray(data[:offset]);self.references=[]

    def skip(self,count):
        self.output.extend(self.data[self.offset:self.offset+count]);self.offset+=count

    def string(self,mapping):
        size=struct.unpack_from('<I',self.data,self.offset)[0];self.offset+=4
        text=self.data[self.offset:self.offset+size].decode('ascii');self.offset+=size
        if text:self.references.append(text)
        replacement=mapping.get(text,text).encode('ascii')
        self.output.extend(struct.pack('<I',len(replacement)));self.output.extend(replacement)

    def finish(self):
        self.output.extend(self.data[self.offset:]);return bytes(self.output)


def rewrite_strings(data,mapping):
    kind=resource_type(data)
    if kind==OSKL:
        count=struct.unpack_from('<I',data,75)[0]
        cursor=StringCursor(data,79)
        for _ in range(count):cursor.string(mapping)
        assert cursor.offset==len(data), 'Unexpected skeleton suffix'
    elif kind==OSLB:
        cursor=StringCursor(data,66);cursor.string(mapping)
        cursor.skip(2)
        modifications=struct.unpack_from('<I',data,cursor.offset)[0]
        assert modifications==0, 'Weighted limb encoding requires a separate converter'
        cursor.skip(4);cursor.string(mapping);cursor.skip(18)
        for _ in range(4):cursor.string(mapping)
        assert cursor.offset+8==len(data), 'Unexpected rigid limb suffix'
    else:
        return data,[]
    return cursor.finish(),cursor.references


def references(data):
    kind=resource_type(data)
    if kind==ODLT:
        return {h for _,_,op,h in dl_commands(data) if h is not None and op!=0x33},set()
    if kind in (OSKL,OSLB):
        _,strings=rewrite_strings(data,{})
        return set(),set(strings)
    return set(),set()


def rewrite(data,mapping):
    kind=resource_type(data)
    if kind==ODLT:
        hash_mapping={crc64(old):crc64(new) for old,new in mapping.items()}
        out=bytearray(data)
        for start,end,op,h in dl_commands(data):
            if h is not None and h in hash_mapping:
                value=hash_mapping[h]
                struct.pack_into('<II',out,start+8,value>>32,value&0xffffffff)
        return bytes(out)
    if kind in (OSKL,OSLB):return rewrite_strings(data,mapping)[0]
    # Accepted XML lists are color-only; path-bearing XML would need a schema-aware rewrite.
    if data.lstrip().startswith(b'<?xml'):
        assert b'objects/' not in data and b'__OTR__' not in data, 'Review XML resource references'
    # Texture/vertex/matrix resources and XML color-only material lists are exact.
    return data


def decoded_payload_summary(source):
    vertex_arrays=vertices=textures=texels=0
    for data in source.values():
        kind=resource_type(data)
        if kind==OARR:
            array_kind,count=struct.unpack_from('<II',data,64)
            assert array_kind==25 and len(data)==72+count*16
            assert len(list(struct.iter_unpack('<hhhHhhBBBB',data[72:])))==count
            vertex_arrays+=1;vertices+=count
        elif kind==OTEX:
            version=struct.unpack_from('<I',data,8)[0]
            texture_kind,width,height=struct.unpack_from('<III',data,64)
            assert version in (0,1) and texture_kind in (1,2)
            size_offset=76 if version==0 else 88
            size=struct.unpack_from('<I',data,size_offset)[0]
            pixels=data[size_offset+4:size_offset+4+size]
            assert len(pixels)==size==width*height*(4 if texture_kind==1 else 2)
            textures+=1;texels+=width*height
    return {'vertex_arrays':vertex_arrays,'decoded_vertices':vertices,
            'native_rgba_texture_arrays':textures,'native_rgba_texels':texels}


def normalise(path):
    return path[4:] if path.startswith('alt/') else path


def shared_gear_seed(path):
    path=normalise(path)
    if '/EquipmentFallback/' in path:return True
    if path.startswith('objects/DinHandsPOC1/NativeEquipment/'):return True
    if path.startswith('objects/object_link_child/YoungDinEyesHandsPOC4_OOT/'):return True
    if path.startswith('objects/object_custom_equip/'):
        return path.rsplit('/',1)[-1]!='gCustomAdultFPSHandDL'
    if path.startswith('objects/object_link_boy/') and path.count('/')==2:
        symbol=path.rsplit('/',1)[-1]
        return any(token in symbol for token in (
            'ShieldAndSheath','ShieldSwordAndSheath','MasterSwordAndSheath',
            'GauntletPlate','IronBoot','HoverBoot')) or symbol in (
            'gLinkAdultBottleDL','gLinkAdultBowStringDL','gLinkAdultBrokenGiantsKnifeBladeDL',
            'gLinkAdultHookshotChainDL','gLinkAdultHookshotTipDL','gLinkAdultSheathNearDL','gLinkAdultSheathFarDL')
    return False


def read_pack(path,expected_hash):
    assert digest(path.read_bytes())==expected_hash, f'Unexpected source: {path.name}'
    with zipfile.ZipFile(path) as archive:
        names=archive.namelist();assert len(names)==len(set(names)), 'Duplicate resource paths'
        return {name:archive.read(name) for name in names}


def private_path(path,season):
    alias='alt/' if path.startswith('alt/') else ''
    path=normalise(path);prefix=f'objects/seasonal_outfits/din/adult/{season}/'
    if path.startswith('objects/object_link_boy/') and path.count('/')==2:
        suffix=path.rsplit('/',1)[-1]
    elif path=='objects/object_custom_equip/gCustomAdultFPSHandDL':
        suffix='gCustomAdultFPSHandDL'
    else:
        suffix='resources/'+path
    return alias+prefix+suffix


def write_pack(path,resources):
    path.parent.mkdir(parents=True,exist_ok=True)
    # Source POC3 archives also use DEFLATE. Keep resource bytes unchanged while
    # avoiding an unnecessary fourfold increase in installed archive size.
    with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=6,allowZip64=True) as archive:
        for name in sorted(resources):
            info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
            info._compresslevel=6
            info.external_attr=0o100644<<16;archive.writestr(info,resources[name])


def marker_resource(path):
    header=bytearray(64);header[1]=1
    struct.pack_into('<I',header,4,ODLT)
    struct.pack_into('<Q',header,12,0xDEADBEEFDEADBEEF)
    reference=crc64(path)
    return bytes(header)+b'\x04'+b'\xff'*7+struct.pack('<IIIIII',0x33000000,0xBEEFBEEF,
        reference>>32,reference&0xffffffff,0xdf000000,0)


def validate_closure(resources,original_external_hashes,original_external_strings):
    hashes={crc64(name):name for name in resources}
    assert len(hashes)==len(resources), 'Resource hash collision'
    missing_hashes=set();missing_strings=set()
    for data in resources.values():
        hash_refs,string_refs=references(data)
        missing_hashes.update(h for h in hash_refs if h not in hashes)
        missing_strings.update(s for s in string_refs if s not in resources)
    assert missing_hashes<=original_external_hashes, f'New unresolved hashes: {missing_hashes-original_external_hashes}'
    assert missing_strings<=original_external_strings, f'New unresolved strings: {missing_strings-original_external_strings}'
    return sorted(f'{h:016x}' for h in missing_hashes),sorted(missing_strings)


def convert(paths,output):
    packs={season:read_pack(paths[season],EXPECTED[season]) for season in EXPECTED}
    first=packs['spring'];all_names=set(first)
    hashes={crc64(name):name for name in first}
    assert len(hashes)==len(first)
    identical={name for name in first if all(pack.get(name)==first[name] for pack in packs.values())}
    seeds={name for name in first if shared_gear_seed(name)}
    shared=seeds&identical
    # Keep every body-dependent wrapper private, even if its name looks like gear.
    while True:
        invalid=set()
        for name in shared:
            hash_refs,string_refs=references(first[name])
            internal={hashes[h] for h in hash_refs if h in hashes}|{s for s in string_refs if s in all_names}
            if any(target not in shared for target in internal):invalid.add(name)
        if not invalid:break
        shared-=invalid
    assert len(shared)==500, 'Review a changed source schema before repackaging'
    shared_resources={name:first[name] for name in shared}
    assert not any(name.rsplit('/',1)[-1]=='gLinkAdultSkel' for name in shared_resources)
    for alias in ('','alt/'):
        marker=alias+'objects/seasonal_din/shared/Enabled'
        shared_resources[marker]=marker_resource(marker)
    shared_path=output/'Adult_Din_Shared_Equipment_POC1.o2r'
    write_pack(shared_path,shared_resources)
    report={'format':1,'scope':'Namespacing and reference routing only; no rig/geometry/texture/animation changes',
            'shared_equipment':{'file':shared_path.name,'sha256':digest(shared_path.read_bytes()),
                                'resource_count':len(shared_resources),'original_resource_count':len(shared),
                                'added_marker_resources':2,'all_original_resources_exact_across_three_sources':True,
                                'paths':sorted(shared)},'seasons':{}}
    for season,source in packs.items():
        mapping={old:private_path(old,season) for old in source if old not in shared}
        assert len(set(mapping.values()))==len(mapping), 'Private path collision'
        candidate={mapping[old]:rewrite(data,mapping) for old,data in source.items() if old in mapping}
        assert len(candidate)==334
        prefix=f'objects/seasonal_outfits/din/adult/{season}/'
        assert all(name.startswith(prefix) or name.startswith('alt/'+prefix) for name in candidate)
        canonical=prefix+'gLinkAdultSkel'
        assert canonical in candidate and 'alt/'+canonical in candidate
        # The accepted source has this body-hand wrapper only in Alt. A private
        # native alias lets the same hand route resolve with Alt Assets disabled.
        compatibility_aliases=[]
        fps_hand=prefix+'gCustomAdultFPSHandDL'
        if fps_hand not in candidate and 'alt/'+fps_hand in candidate:
            candidate[fps_hand]=rewrite(candidate['alt/'+fps_hand],{'alt/'+fps_hand:fps_hand})
            compatibility_aliases.append(fps_hand)
        assert all(resource_type(data)!=OANM for data in candidate.values()), 'Animations must remain separate'
        # Reverse the reference remap and demand every resource byte back exactly.
        reverse={new:old for old,new in mapping.items()}
        for old,new in mapping.items():
            assert rewrite(candidate[new],reverse)==source[old], f'Unintended data edit: {old}'
        old_private_hashes={crc64(old) for old in mapping}
        for data in candidate.values():
            hash_refs,string_refs=references(data)
            assert not (hash_refs&old_private_hashes), 'Original body hash alias leaked'
            assert not (string_refs&mapping.keys()), 'Original body string alias leaked'
        original_hashes={crc64(name) for name in source}
        original_external_hashes=set();original_external_strings=set()
        for data in source.values():
            hash_refs,string_refs=references(data)
            original_external_hashes.update(h for h in hash_refs if h not in original_hashes)
            original_external_strings.update(s for s in string_refs if s not in source)
        external_hashes,external_strings=validate_closure(candidate|shared_resources,original_external_hashes,original_external_strings)
        label=season.capitalize()
        path=output/f'Adult_Din_{label}_Seasonal_POC1.private.o2r'
        write_pack(path,candidate)
        unchanged_types=Counter()
        for old,new in mapping.items():
            kind=resource_type(source[old])
            if kind in (OARR,OTEX,OMTX):
                assert candidate[new]==source[old]
                unchanged_types[f'{kind:08x}']+=1
        report['seasons'][season]={'file':path.name,'sha256':digest(path.read_bytes()),
            'source_file':paths[season].name,'source_sha256':EXPECTED[season],
            'private_resource_count':len(candidate),'canonical_skeleton':'__OTR__'+canonical,
            'added_private_native_hand_aliases':compatibility_aliases,
            'all_resources_reverse_reference_rewrite_exact':True,'immutable_binary_resource_counts':dict(unchanged_types),
            'combined_unchanged_decoded_payloads':decoded_payload_summary(source),
            'original_external_hashes_preserved':external_hashes,'original_external_strings_preserved':external_strings,
            'animation_resources':0,'path_map':mapping,'runtime':'UNTESTED'}
    output.mkdir(parents=True,exist_ok=True)
    (output/'Seasonal_Conversion_Validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({season:{k:v for k,v in entry.items() if k not in ('path_map',)} for season,entry in report['seasons'].items()},indent=2))
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--spring',type=Path,required=True)
    parser.add_argument('--autumn',type=Path,required=True,help='Accepted Fall POC3 input')
    parser.add_argument('--winter',type=Path,required=True)
    parser.add_argument('--output-dir',type=Path,required=True)
    args=parser.parse_args()
    convert({season:getattr(args,season) for season in EXPECTED},args.output_dir)
