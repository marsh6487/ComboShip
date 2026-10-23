#!/usr/bin/env python3
"""Check production shelf geometry against reserved offers and all keeper poses."""
from pathlib import Path
import argparse,json,subprocess,sys,tempfile,zipfile
import numpy as np

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/black_market'))
from presentation_preview import shelf_positions,triangle_box_intersects
from native_preview import NativeKeeper
from keeper_lean import room_faces

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('pack',type=Path);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    assert (ROOT/'combo/BlackMarketShelves.h').exists(),'Physical shop shelves are missing; floating item anchors alone do not create shelves'
    with tempfile.TemporaryDirectory(prefix='market-shelves-') as temporary:
        binary=Path(temporary)/'shelves'
        subprocess.run(['c++','-std=c++20','-I'+str(ROOT),str(ROOT/'tools/black_market/shelves_preview.cpp'),'-o',str(binary)],check=True)
        fixture=json.loads(subprocess.check_output([str(binary)],text=True))
    shelves=np.array(shelf_positions(),float)
    relative=np.array([[-17.368482142857143,-15.162767857142857,-16.96848214285714],
                       [17.368482142857143,27.962767857142858,16.96848214285714]])
    reserved=np.array([relative+position for position in shelves])
    with zipfile.ZipFile(a.pack) as archive:
        keeper=NativeKeeper(archive)
        triangles=np.concatenate([keeper.position_triangles(i) for i in range(keeper.frame_count)])
        room=np.array(room_faces(archive),float)
    boxes=np.array(fixture['boxes'],float)
    for index,box in enumerate(boxes):
        for slot,offer in enumerate(reserved):
            assert not (np.all(box[1]>=offer[0]) and np.all(offer[1]>=box[0])),(index,slot,'shelf intersects selected item/seal envelope')
        candidates=np.all(triangles.max(1)>=box[0],axis=1)&np.all(triangles.min(1)<=box[1],axis=1)
        assert not any(triangle_box_intersects(face,box[0],box[1]) for face in triangles[candidates]),(index,'shelf intersects keeper pose')
        if index%6!=3: # Bottom planks intentionally seat over the raised counter.
            inner=box+[[.01,.01,.01],[-.01,-.01,-.01]]
            candidates=np.all(room.max(1)>=inner[0],axis=1)&np.all(room.min(1)<=inner[1],axis=1)
            assert not any(triangle_box_intersects(face,inner[0],inner[1]) for face in room[candidates]),(index,'shelf intersects retained room props')
    for slot,position in enumerate(shelves):
        boards=[box for box in boxes if box[1,1]<reserved[slot,0,1] and box[0,0]<position[0]<box[1,0] and box[0,2]<position[2]<box[1,2]]
        assert boards,(slot,'item has no shelf beneath it')
        assert reserved[slot,0,1]-max(box[1,1] for box in boards)<5,(slot,'shelf too far below the complete offer envelope')
    a.output.write_text(json.dumps(dict(boxes=fixture['boxes'],shelves=shelves.tolist(),
        reserved_slot_envelopes=reserved.tolist(),keeper_frames=keeper.frame_count,
        physical_shelves_clear_items_and_keeper=True,native_runtime_proven=False),indent=2)+'\n')
    print('PASS physical shelves: every offer has a board; full selected item/seal envelopes and all keeper poses clear shared shelf geometry')

if __name__=='__main__':main()
