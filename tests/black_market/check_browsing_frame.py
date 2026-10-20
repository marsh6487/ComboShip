#!/usr/bin/env python3
"""Project reserved item/seal bounds through the production shop camera poses."""
from pathlib import Path
import argparse,hashlib,itertools,json,os,subprocess,tempfile
import numpy as np

ROOT=Path(__file__).resolve().parents[2]
def camera_views():
    with tempfile.TemporaryDirectory(prefix='market-camera-') as temporary:
        binary=Path(temporary)/'camera'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),
            str(ROOT/'tools/black_market/browsing_preview.cpp'),'-o',str(binary)],check=True)
        return json.loads(subprocess.check_output([str(binary)],text=True))

def project(point,view):
    eye,at=np.array(view['eye'],float),np.array(view['at'],float)
    forward=at-eye;forward/=np.linalg.norm(forward)
    right=np.cross(forward,[0,1,0]);right/=np.linalg.norm(right)
    up=np.cross(right,forward);delta=np.array(point)-eye;depth=delta@forward
    assert depth>0
    half=np.tan(np.deg2rad(view['fov'])/2)
    return [160+(delta@right)/depth/half*120,120-(delta@up)/depth/half*120]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--layout',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();layout=json.loads(a.layout.read_text());views=camera_views();proof={}
    for name,slots in [('right',range(4)),('left',range(4,8))]:
        bounds=[]
        for slot in slots:
            points=np.array([project(point,views[name]) for point in itertools.product(*zip(*layout['reserved_slot_envelopes'][slot]))])
            low,high=points.min(0),points.max(0)
            assert low[0]>8 and high[0]<312 and low[1]>8 and high[1]<138,(name,slot,'item/seal clips screen or native Y142 textbox',low,high)
            anchor=np.array(project(layout['shelves'][slot],views[name]))
            assert anchor[1]+20<142 and anchor[1]-20>4,(name,slot,'native cursor clips textbox or top')
            bounds.append({'slot':slot,'screen_min':low.tolist(),'screen_max':high.tolist()})
        proof[name]=bounds
    a.output.write_text(json.dumps(dict(views=views,bounds=proof,layout_sha256=hashlib.sha256(a.layout.read_bytes()).hexdigest(),
        source_sha256=hashlib.sha256((ROOT/'combo/BlackMarketBrowsing.h').read_bytes()).hexdigest(),
        native_textbox_y=142,aspect='4:3',runtime_proven=False,
        scope='Conditional full authored FrameFit item/seal envelope including selected scale and bob; arbitrary native/Alt models not covered.'),indent=2)+'\n')
    print('PASS browsing framing: eight full item/seal envelopes and native cursor clear 4:3 screen and Y142 textbox')

if __name__=='__main__':main()
