"""Offline renders read back from the exported native archives, never game captures."""
import argparse
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

from native_npc_models import VARIANTS, NativeRig, animation_frames, unit
from verify_native_npc_models import Archive
from native_assets import n64texture
from native_assets.gbi import Triangle, TileState, Vertex
from native_assets.render import Tex, render


def preview(task):
    native, directory, variant, mode = task
    directory = Path(directory)
    settings = VARIANTS[variant]
    archive = Archive(native, directory/settings["archive"])
    rig = NativeRig(archive, settings["profile"], private=True)
    animations = {
        "saria": ["gSariaWaitArmsToSideAnim", "gSariaPlayingOcarinaAnim", "gSariaSitting3Anim"],
        "twins": ["gKokiriIdleAnim", "gKokiriStandingArmsBehindBackAnim", "gKokiriSittingCrossedArmsLegsAnim"],
        "young-malon": ["gMalonChildIdleAnim", "gMalonChildRaiseHandsAnim", "gMalonChildSingAnim"],
        "malon": ["gMalonAdultIdleAnim", "gMalonAdultStandStillAnim", "gMalonAdultSingAnim"],
    }
    expression = mode.startswith("face")
    if expression:
        pose, eye, mouth = 0, int(mode[-1]), 0
    else:
        pose, eye, mouth = int(mode[-1]), 0, 0
    anim_object = rig.profile.get("animation_object", rig.profile["object"])
    animation = f"objects/{anim_object}/{animations[rig.name][pose]}"
    pose_key = rig.ns+"/PoseAnimations/"+animation
    clip = "alt/"+pose_key
    frames = animation_frames(archive.candidate.read(clip) if clip in archive.candidate.namelist() else archive.native.read(animation))
    rig.pose(frames[len(frames)//2 if pose else 0])
    textures, records = {}, []

    def texture(path):
        if path not in textures:
            kind, w, h, raw = archive.texture(path)
            fmt, size = {1: (0, 3), 2: (0, 2), 5: (4, 0), 6: (4, 1), 7: (3, 0), 8: (3, 1), 9: (3, 2)}[kind]
            image = n64texture.decode(raw, fmt, size, w, h)
            scale = w/512 if "NativeNpcV1" in path else 1
            textures[path] = Tex(image, fmt, size)
            textures[path].scale_s = textures[path].scale_t = scale
        return textures[path]

    entries = []
    for i, limb in enumerate(rig.limbs):
        if not limb.dlist:
            continue
        path = rig.ns+f"/Body/Limb{i:02d}"
        if i == rig.profile["head"]:
            path = rig.ns+f"/Faces/L{eye}R{eye}M{mouth}"
        if rig.name == "saria" and i == 14 and pose == 1:
            path = rig.ns+"/Props/RightHandOcarina"
        if "alt/"+pose_key+f"/Limbs/Limb{i:02d}" in archive.candidate.namelist():
            path = pose_key+f"/Limbs/Limb{i:02d}"
        entries.append((path, i))
    if rig.name == "malon" and pose == 1 and not expression:
        basket = pose_key+"/Props/Basket"
        entries.append((basket if "alt/"+basket in archive.candidate.namelist() else
                        "objects/object_ma2/gMalonAdultBasketDL", rig.profile["left"][-1]))
    for path, i in entries:
        data = archive.geometry(path, rig, i)
        p, n = np.zeros_like(data["local"]), np.zeros_like(data["normals"])
        for bone in np.unique(data["bones"]):
            mask = data["bones"] == bone
            matrix = rig.skel.limbs[bone]
            p[mask] = data["local"][mask]@matrix.M.T+matrix.T
            n[mask] = data["normals"][mask]@matrix.M.T
        shade = 0.45+0.55*np.maximum(unit(n)@unit([0.3, 0.8, 0.5]), 0)
        tile = TileState(cms=2, cmt=2, cull_mode=0)
        st = data["uv"]*512*32
        for face, tex in zip(data["faces"], data["textures"]):
            vertices = tuple(Vertex(0, 0, 0, int(st[j, 0]), int(st[j, 1]), 0, 0, 0, 255) for j in face)
            records.append((Triangle(vertices, tile), p[face], [(float(shade[j]),)*3 for j in face], texture(tex)))
    bounds = None
    if expression:
        head = rig.skel.limbs[rig.profile["head"]].T
        span = 1400 if rig.name != "malon" else 1800
        bounds = (head[0]-span/2, head[1]-span/10, head[0]+span/2, head[1]+span*0.9)
    out = directory/"previews"
    out.mkdir(exist_ok=True)
    render(records, {}, size=480, supersample=1, bg=(28, 33, 40, 255), view_bounds=bounds).save(out/f"{variant}-{mode}.png")
    return f"{variant}-{mode}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native")
    parser.add_argument("candidates", type=Path)
    args = parser.parse_args()
    modes = ["pose0", "pose1", "pose2", "face0", "face1", "face2"]
    tasks = [(args.native, str(args.candidates), name, mode) for name in VARIANTS for mode in modes]
    with ProcessPoolExecutor(max_workers=2) as pool:
        for result in pool.map(preview, tasks):
            print("Rendered", result, flush=True)
    for title, selection in [("Native_Pose_Review", modes[:3]), ("Native_Blink_Review", modes[3:])]:
        sheet = Image.new("RGB", (480*3, 510*len(VARIANTS)+40), (28, 33, 40))
        draw = ImageDraw.Draw(sheet)
        draw.text((14, 12), "Exported archive geometry + native animation | OFFLINE PREVIEW | Runtime untested", fill="white")
        for row, variant in enumerate(VARIANTS):
            for col, mode in enumerate(selection):
                x, y = col*480, row*510+40
                sheet.paste(Image.open(args.candidates/"previews"/f"{variant}-{mode}.png"), (x, y))
                draw.text((x+12, y+483), f"{variant} / {mode}", fill="white")
        sheet.save(args.candidates/"previews"/f"{title}.png")


if __name__ == "__main__":
    main()
