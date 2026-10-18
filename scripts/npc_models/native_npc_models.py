"""Compile Tripo GLBs to optional native NPC rigs. Inputs remain read-only.

Uses the recovered Fado resource writer and rigid native matrix convention.
No game animations, actor logic, placements, or public model resources are replaced.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import io
import json
from pathlib import Path
import struct
import zipfile

import numpy as np
from PIL import Image
from scipy.spatial.transform import Rotation
from scipy.spatial import cKDTree

from native_assets import o2r as asset, skeleton

PROFILES = {
    "saria": dict(object="object_sa", skeleton="gSariaSkel", animation="gSariaWaitArmsToSideAnim",
                  hips=1, chest=8, head=15, left=[2, 3, 4, 9, 10, 11], right=[5, 6, 7, 12, 13, 14]),
    "twins": dict(object="object_kw1", skeleton="gKw1Skel", animation="gKokiriIdleAnim",
                  animation_object="object_os_anime", hips=0, chest=7, head=14,
                  left=[1, 2, 3, 8, 9, 10], right=[4, 5, 6, 11, 12, 13], native_actor_types=[1, 5, 6, 9, 10]),
    "young-malon": dict(object="object_ma1", skeleton="gMalonChildSkel", animation="gMalonChildIdleAnim",
                        hips=0, chest=7, head=14, left=[1, 2, 3, 8, 9, 10], right=[4, 5, 6, 11, 12, 13]),
    "malon": dict(object="object_ma2", skeleton="gMalonAdultSkel", animation="gMalonAdultIdleAnim",
                  hips=8, chest=10, head=17, left=[2, 3, 4, 11, 12, 13], right=[5, 6, 7, 14, 15, 16]),
}
VARIANTS = {
    "saria-anime": dict(profile="saria", file="Saria+Anime+custom+npc+++player+model.glb",
                        archive="Saria_Anime_Native_NPC_POC1.o2r"),
    "saria-render": dict(profile="saria", file="Saria+Render+GLB.glb",
                         archive="Saria_Render_Native_NPC_POC1.o2r"),
    "twins": dict(profile="twins", file="fantasy+character+3d+model.glb",
                  archive="Kokiri_Twins_Render_Native_NPC_POC1.o2r"),
    "young-malon": dict(profile="young-malon", file="young+malon+oot+remake+style.glb",
                        archive="Young_Malon_Native_NPC_POC1.o2r"),
    "malon": dict(profile="malon", file="adult+malon+render+art.glb",
                  archive="Adult_Malon_Render_Native_NPC_POC1.o2r"),
}


def unit(a):
    a = np.asarray(a, dtype=float)
    return a / np.maximum(np.linalg.norm(a, axis=-1, keepdims=True), 1e-12)


def align(a, b):
    a, b = unit(a), unit(b)
    cross = np.cross(a, b)
    cosine = np.clip(np.dot(a, b), -1, 1)
    if cosine < -0.999999:
        axis = np.eye(3)[np.argmin(abs(a))]
        return Rotation.from_rotvec(unit(np.cross(a, axis)) * np.pi).as_matrix()
    k = np.array([[0, -cross[2], cross[1]], [cross[2], 0, -cross[0]], [-cross[1], cross[0], 0]])
    return np.eye(3) + k + k @ k / max(1 + cosine, 1e-12)


class SourceModel:
    def __init__(self, path):
        self.raw = Path(path).read_bytes()
        if len(self.raw) < 28 or struct.unpack_from("<II", self.raw)[0:2] != (0x46546C67, 2):
            raise ValueError("Expected a GLB 2.0 file")
        length = struct.unpack_from("<I", self.raw, 12)[0]
        self.g = json.loads(self.raw[20:20+length])
        self.binary = self.raw[28+length:]
        self.world, self.parent = {}, {}

        def walk(index, parent=None):
            node = self.g["nodes"][index]
            matrix = np.eye(4)
            if "matrix" in node:
                matrix = np.asarray(node["matrix"]).reshape(4, 4).T
            else:
                matrix[:3, :3] = Rotation.from_quat(node.get("rotation", [0, 0, 0, 1])).as_matrix() @ np.diag(node.get("scale", [1, 1, 1]))
                matrix[:3, 3] = node.get("translation", [0, 0, 0])
            self.parent[index] = parent
            self.world[index] = matrix if parent is None else self.world[parent] @ matrix
            for child in node.get("children", []):
                walk(child, index)

        for index in self.g["scenes"][self.g.get("scene", 0)]["nodes"]:
            walk(index)
        skins = self.g.get("skins", [])
        self.joints = skins[0]["joints"] if skins else []
        self.names = [self.g["nodes"][i]["name"].removeprefix("mixamorig:") for i in self.joints]
        self.inverse = self.accessor(skins[0]["inverseBindMatrices"]).reshape(-1, 4, 4).transpose(0, 2, 1) if skins else []
        self.meshes = []
        material_ids = set()
        for index, node in enumerate(self.g["nodes"]):
            if "mesh" not in node:
                continue
            for primitive in self.g["meshes"][node["mesh"]]["primitives"]:
                if primitive.get("mode", 4) != 4 or node.get("skin") != 0:
                    raise ValueError("Expected triangle primitives using the source skin")
                a = primitive["attributes"]
                material_ids.add(primitive.get("material", 0))
                self.meshes.append(dict(name=node.get("name", str(index)),
                                        positions=self.accessor(a["POSITION"]), normals=self.accessor(a["NORMAL"]),
                                        uv=self.accessor(a["TEXCOORD_0"]), joints=self.accessor(a["JOINTS_0"]).astype(int),
                                        weights=self.accessor(a["WEIGHTS_0"]),
                                        faces=self.accessor(primitive["indices"]).reshape(-1, 3).astype(int)))
        if len(material_ids) > 1:
            raise ValueError("This candidate requires one diffuse atlas")
        material = self.g["materials"][next(iter(material_ids), 0)]
        texture = material["pbrMetallicRoughness"]["baseColorTexture"]["index"]
        image = self.g["images"][self.g["textures"][texture]["source"]]
        view = self.g["bufferViews"][image["bufferView"]]
        offset = view.get("byteOffset", 0)
        self.image_bytes = self.binary[offset:offset+view["byteLength"]]
        self.image = Image.open(io.BytesIO(self.image_bytes)).convert("RGBA")

    def accessor(self, index):
        a = self.g["accessors"][index]
        view = self.g["bufferViews"][a["bufferView"]]
        dtype = np.dtype({5120: "i1", 5121: "u1", 5122: "<i2", 5123: "<u2", 5125: "<u4", 5126: "<f4"}[a["componentType"]])
        width = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}[a["type"]]
        out = np.ndarray((a["count"], width), dtype=dtype, buffer=self.binary,
                         offset=view.get("byteOffset", 0)+a.get("byteOffset", 0),
                         strides=(view.get("byteStride", width*dtype.itemsize), dtype.itemsize)).copy()
        if a.get("normalized"):
            out = np.maximum(out.astype(float) / np.iinfo(dtype).max, -1)
        return out


def read_string(raw, offset):
    size = struct.unpack_from("<I", raw, offset)[0]
    return raw[offset+4:offset+4+size].decode().removeprefix("__OTR__"), offset+4+size


def read_limb(raw):
    offset = 66
    _, offset = read_string(raw, offset)
    offset += 6
    _, offset = read_string(raw, offset)
    offset += 18
    for _ in range(2):
        _, offset = read_string(raw, offset)
    near, offset = read_string(raw, offset)
    far, offset = read_string(raw, offset)
    return struct.unpack_from("<hhhBB", raw, offset), near, far


def animation_frames(raw):
    try:
        if raw[0] != 0 or raw[4:8] != b"MNAO" or struct.unpack_from("<I", raw, 64)[0] != 0:
            raise ValueError("Expected a native normal animation")
        count, size = struct.unpack_from("<hI", raw, 68)
        if count <= 0 or size > (len(raw)-74)//2:
            raise ValueError("Invalid native animation dimensions")
        values = np.asarray(struct.unpack_from(f"<{size}h", raw, 74))
        offset = 74+size*2
        joints = struct.unpack_from("<I", raw, offset)[0]
        indices = np.asarray([struct.unpack_from("<HHH", raw, offset+4+i*6) for i in range(joints)])
        limit = struct.unpack_from("<H", raw, offset+4+joints*6)[0]
        sample = indices[None, :, :] + np.where(indices[None, :, :] < limit, 0, np.arange(count)[:, None, None])
        if sample.max() >= len(values):
            raise ValueError("Native animation channel exceeds its value buffer")
        return values[sample]
    except (IndexError, struct.error) as error:
        raise ValueError("Truncated native animation") from error


class NativeRig:
    def __init__(self, native, profile, private=False):
        self.profile = PROFILES[profile]
        self.name = profile
        self.archive = native
        self.ns = f"objects/{self.profile['object']}/NativeNpcV1"
        base = f"objects/{self.profile['object']}"
        raw = native.read(self.ns+"/Skel" if private else base+"/"+self.profile["skeleton"])
        self.skeleton_raw = raw
        count, self.dlist_count = struct.unpack_from("<II", raw, 66)
        self.limb_type = raw[65]
        offset = 79
        self.limbs, self.near = [], []
        for i in range(count):
            path, offset = read_string(raw, offset)
            values, near, _ = read_limb(native.read(path))
            x, y, z, child, sibling = values
            self.limbs.append(skeleton.Limb(i, x, y, z, child, sibling, int(bool(near))))
            self.near.append(near)
        self.skel = skeleton.Skeleton(self.limbs, count, self.dlist_count)
        animation_object = self.profile.get("animation_object", self.profile["object"])
        animation = f"objects/{animation_object}/{self.profile['animation']}"
        self.frames = animation_frames(native.read(animation))
        self.pose(self.frames[0])

    def pose(self, frame):
        skeleton._pose(self.skel, np.asarray(frame).ravel().tolist())
        return self.skel


def fit_model(source, rig):
    profile = rig.profile
    pos = {name: source.world[index][:3, 3] for name, index in zip(source.names, source.joints)}
    required = ["Hips", "Neck", "Head", "LeftArm", "LeftForeArm", "LeftHand", "LeftUpLeg", "LeftLeg", "LeftFoot",
                "RightArm", "RightForeArm", "RightHand", "RightUpLeg", "RightLeg", "RightFoot"]
    if any(name not in pos for name in required):
        raise ValueError("Source is missing required humanoid joints")
    T = lambda index: rig.skel.limbs[index].T
    hips, head = profile["hips"], profile["head"]
    left = profile["left"]
    source_leg = np.linalg.norm(pos["LeftLeg"]-pos["LeftUpLeg"]) + np.linalg.norm(pos["LeftFoot"]-pos["LeftLeg"])
    scale = (np.linalg.norm(T(left[1])-T(left[0]))+np.linalg.norm(T(left[2])-T(left[1]))) / source_leg
    src_x = unit(pos["LeftArm"]-pos["RightArm"])
    src_y = unit(pos["Neck"]-pos["Hips"])
    src_z = unit(np.cross(src_x, src_y))
    src_y = unit(np.cross(src_z, src_x))
    target_x = unit(T(profile["left"][3])-T(profile["right"][3]))
    target_y = unit(T(head)-T(hips))
    target_z = unit(np.cross(target_x, target_y))
    target_y = unit(np.cross(target_z, target_x))
    basis = np.column_stack([target_x, target_y, target_z]) @ np.column_stack([src_x, src_y, src_z]).T
    height_scale = np.linalg.norm(T(head)-T(hips))/np.linalg.norm(pos["Neck"]-pos["Hips"])
    width_scale = np.linalg.norm(T(profile["left"][3])-T(profile["right"][3])) / np.linalg.norm(pos["LeftArm"]-pos["RightArm"])
    torso = np.column_stack([target_x*width_scale, target_y*height_scale, target_z*scale]) @ np.column_stack([src_x, src_y, src_z]).T
    owner = {name: profile["chest"] for name in source.names}
    owner.update(Hips=hips, Neck=head, Head=head, HeadTop_End=head)
    transforms = {name: (torso, T(hips)-torso@pos["Hips"]) for name in source.names}
    for name in ["Neck", "Head", "HeadTop_End"]:
        a = basis*scale
        transforms[name] = a, T(head)-a@pos["Neck"]
    for side, indices in [("Left", profile["left"]), ("Right", profile["right"])]:
        up, leg, foot, arm, forearm, hand = indices
        for first, second, first_target, second_target in [("UpLeg", "Leg", up, leg), ("Leg", "Foot", leg, foot),
                                                         ("Arm", "ForeArm", arm, forearm), ("ForeArm", "Hand", forearm, hand)]:
            start, end = side+first, side+second
            u = unit(pos[end]-pos[start])
            ratio = np.linalg.norm(T(second_target)-T(first_target))/np.linalg.norm(pos[end]-pos[start])
            a = align(u, T(second_target)-T(first_target)) @ (scale*np.eye(3)+(ratio-scale)*np.outer(u, u))
            transforms[start] = a, T(first_target)-a@pos[start]
            owner[start] = first_target
        palm = pos.get(side+"HandMiddle1", pos[side+"Hand"]+(pos[side+"Hand"]-pos[side+"ForeArm"]))-pos[side+"Hand"]
        a = align(palm, rig.skel.limbs[hand].M[:, 0])*scale
        for name in source.names:
            if name.startswith(side+"Hand"):
                transforms[name] = a, T(hand)-a@pos[side+"Hand"]
                owner[name] = hand
        # Ankle-to-toe rig vectors descend inside the source boots. Aligning
        # them to a horizontal vector rotates the entire sole upward.
        a = basis*scale
        sole = min(mesh["positions"][np.isin(mesh["joints"], [j for j, name in enumerate(source.names)
                    if name in [side+"Foot", side+"ToeBase", side+"Toe_End"]]).any(1), 1].min()
                   for mesh in source.meshes if np.isin(mesh["joints"], [j for j, name in enumerate(source.names)
                    if name in [side+"Foot", side+"ToeBase", side+"Toe_End"]]).any())
        vertical = np.array([0., -(T(foot)[1]+scale*(sole-pos[side+"Foot"][1])), 0.])
        for name in [side+"Foot", side+"ToeBase", side+"Toe_End"]:
            if name in owner:
                transforms[name] = a, T(foot)-a@pos[side+"Foot"]+vertical
                owner[name] = foot
    skin = [source.world[i]@inv for i, inv in zip(source.joints, source.inverse)]
    parts = []
    for mesh in source.meshes:
        p, n, joints, weights = [mesh[k] for k in ["positions", "normals", "joints", "weights"]]
        if not np.isfinite(weights).all() or not np.allclose(weights.sum(1), 1, atol=0.0003):
            raise ValueError("Invalid source skin weights")
        result, normals = np.zeros_like(p, float), np.zeros_like(n, float)
        bone_weights = np.zeros((len(p), len(rig.limbs)))
        for slot in range(joints.shape[1]):
            for joint in np.unique(joints[:, slot]):
                mask = np.flatnonzero(joints[:, slot] == joint)
                name = source.names[joint]
                a, t = transforms[name]
                posed = p[mask]@skin[joint][:3, :3].T+skin[joint][:3, 3]
                result[mask] += (posed@a.T+t)*weights[mask, slot, None]
                normals[mask] += (n[mask]@skin[joint][:3, :3].T@np.linalg.inv(a))*weights[mask, slot, None]
                bone_weights[mask, owner[name]] += weights[mask, slot]
        low, high = p.min(0), p.max(0)
        head_piece = high[1] > pos["Neck"][1]+0.055 and low[1] > pos["Hips"][1]+0.02 and high[0]-low[0] < 0.4
        dress_piece = (rig.name in ("malon", "young-malon") and low[1] < pos["Hips"][1]-0.15 and
                       high[1] > pos["Hips"][1]+0.015 and high[1] < pos["Neck"][1]-0.1)
        if head_piece or dress_piece:
            # Tripo's loose hair and continuous skirt inherit stray arm/leg
            # weights. Their native counterparts have dedicated rigid limbs.
            a, t = transforms["Head" if head_piece else "Hips"]
            mask = np.ones(len(p), bool)
            if head_piece:
                tex = np.asarray(source.image)
                st = mesh["uv"]
                rgb = tex[np.clip((st[:, 1]*tex.shape[0]).astype(int), 0, tex.shape[0]-1),
                          np.clip((st[:, 0]*tex.shape[1]).astype(int), 0, tex.shape[1]-1), :3].astype(float)
                hair = ((rgb[:, 0] > rgb[:, 1]*1.3) & (rgb[:, 0] > rgb[:, 2]*1.5) |
                        (rgb[:, 1] > rgb[:, 0]*1.2) & (rgb[:, 1] > rgb[:, 2]*1.2))
                mask = (p[:, 1] > pos["Neck"][1]-0.015) | hair
            result[mask], normals[mask] = p[mask]@a.T+t, n[mask]@np.linalg.inv(a)
            bone_weights[mask] = 0
            bone_weights[mask, head if head_piece else (16 if rig.name == "young-malon" else 9)] = 1
        edges = np.sort(np.vstack([mesh["faces"][:, [0, 1]], mesh["faces"][:, [1, 2]], mesh["faces"][:, [2, 0]]]), axis=1)
        edge, count = np.unique(edges, axis=0, return_counts=True)
        boundary = np.zeros(len(p), bool)
        boundary[np.unique(edge[count == 1])] = True
        parts.append(dict(positions=result, normals=unit(normals), uv=mesh["uv"], weights=bone_weights,
                          source_positions=p, boundary=boundary, mesh_ids=np.full(len(p), len(parts)), faces=mesh["faces"]))
    model = {key: np.concatenate([p[key] for p in parts]) for key in ["positions", "normals", "uv", "weights", "source_positions", "boundary", "mesh_ids"]}
    offset, faces = 0, []
    for part in parts:
        faces.append(part["faces"]+offset)
        offset += len(part["positions"])
    model["faces"] = np.concatenate(faces)
    model["bones"] = model["weights"].argmax(1)
    _, inverse = np.unique(np.round(model["source_positions"], 6), axis=0, return_inverse=True)
    for group in np.flatnonzero(np.bincount(inverse) > 1):
        ids = np.flatnonzero(inverse == group)
        model["positions"][ids] = model["positions"][ids].mean(0)
        model["bones"][ids] = model["weights"][ids].sum(0).argmax()
    join_split_surfaces(model, scale, head)
    hand_bones = [profile["left"][-1], profile["right"][-1]]
    hosts = []
    for f in model["faces"]:
        bones = model["bones"][f]
        hands = [b for b in hand_bones if b in bones]
        hosts.append(hands[0] if hands else Counter(bones).most_common(1)[0][0])
    model["hosts"] = np.asarray(hosts)
    model["scale"] = scale
    model["head_transform"] = np.column_stack(transforms["Head"])
    return model


def join_split_surfaces(model, scale, head):
    """Join nearby split body boundaries before quantization and native motion.

    Source segmentation leaves tiny gaps with inconsistent joint influences.
    Match only nearby cross-mesh boundary vertices, keeping head artwork out.
    """
    ids = np.flatnonzero(model["boundary"] & (model["bones"] != head))
    if len(ids) < 2:
        return
    points = model["source_positions"][ids]
    pairs = cKDTree(points).query_pairs(0.004, output_type="ndarray")
    if not len(pairs):
        return
    a, b = ids[pairs[:, 0]], ids[pairs[:, 1]]
    distance = np.linalg.norm(model["source_positions"][a]-model["source_positions"][b], axis=1)
    delta = np.linalg.norm(model["positions"][a]-model["positions"][b], axis=1)
    keep = (model["mesh_ids"][a] != model["mesh_ids"][b]) & (delta > np.maximum(6, distance*scale*2.5))
    pairs = np.column_stack([a[keep], b[keep]])
    parent = np.arange(len(model["positions"]))

    def root(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    members = {int(i): {int(i)} for i in np.unique(pairs)}
    for first, second in pairs:
        first, second = root(first), root(second)
        if first == second:
            continue
        group = members[first] | members[second]
        if np.ptp(model["source_positions"][list(group)], axis=0).max() > 0.005:
            continue
        parent[second] = first
        members[first] = group
        del members[second]
    old = model["positions"].copy()
    repaired = []
    for group in members.values():
        if len(group) < 2:
            continue
        group = np.asarray(sorted(group))
        model["positions"][group] = old[group].mean(0)
        model["bones"][group] = model["weights"][group].sum(0).argmax()
        repaired.extend(group)
    if repaired:
        repaired = np.asarray(repaired)
        distance, nearest = cKDTree(model["source_positions"][repaired]).query(model["source_positions"])
        weight = np.clip(1-distance/0.018, 0, 1)**2
        weight[repaired] = 0
        weight[model["bones"] == head] = 0
        correction = model["positions"][repaired]-old[repaired]
        model["positions"] += correction[nearest]*weight[:, None]


def emit_mesh(archive, path, model, rig, faces, texpath, restore):
    """Fado's 32-vertex cache batching and native matrix ownership convention."""
    d = asset.DLBuilder()
    d.marker(path)
    for a, b in [(0xE7000000, 0), (0xE3001001, 0), (0xD7000002, 0xFFFFFFFF),
                 (0xFC127E03, 0xFFFFFBFD), (0xD9F1F9FF, 0), (0xD9FFFFFF, 0x00830005),
                 (0xFA000000, 0xFFFFFFFF)]:
        d.raw(a, b)
    d.settimg_hash(0, 3, 512, texpath)
    for a, b in [(0xF5180000, 0x07080200), (0xE6000000, 0), (0xF4000000, 0x077FC7FC),
                 (0xE7000000, 0), (0xF5190000, 0x00080200), (0xF2000000, 0x007FC7FC)]:
        d.raw(a, b)
    records = []
    for p, n, uv, bone in zip(model["positions"], model["normals"], model["uv"], model["bones"]):
        limb = rig.skel.limbs[bone]
        xyz = np.rint((p-limb.T)@limb.M).astype(int)
        normal = np.rint(unit(n@limb.M)*127).astype(int)
        st = np.rint(uv*512*32).astype(int)
        if max(abs(xyz)) >= 32768 or max(abs(st)) >= 32768:
            raise ValueError("Vertex coordinate exceeds the native representation")
        records.append((int(bone), *map(int, xyz), *map(int, st), *map(int, normal)))
    slots = rig.skel.slot_to_limb
    groups = defaultdict(list)
    for f in faces:
        groups[tuple(sorted(set(int(b) for b in model["bones"][f])))].append(f)
    vertices, current = [], None

    def flush(batch):
        nonlocal current
        unique = sorted({records[int(i)] for f in batch for i in f})
        lookup = {record: i for i, record in enumerate(unique)}
        for bone in sorted(set(record[0] for record in unique)):
            subset = [(i, record) for i, record in enumerate(unique) if record[0] == bone]
            slot = slots.index(bone)
            if current != slot:
                d.raw(0xDA380003, 0x0D000001+slot*64)
                current = slot
            offset = len(vertices)*16
            vertices.extend([(*r[1:4], slot << 8, *r[4:], 255) for _, r in subset])
            d.vtx_hash(len(subset), subset[0][0], offset, path+"/V")
        triangles = [[lookup[records[int(i)]] for i in f] for f in batch]
        for i in range(0, len(triangles), 2):
            a, b, c = [x*2 for x in triangles[i]]
            first = (a << 16) | (b << 8) | c
            if i+1 < len(triangles):
                a, b, c = [x*2 for x in triangles[i+1]]
                d.raw(0x06000000 | first, (a << 16) | (b << 8) | c)
            else:
                d.raw(0x05000000 | first, 0)

    for _, fs in sorted(groups.items()):
        batch, used = [], set()
        for f in fs:
            more = {records[int(i)] for i in f}
            if len(used | more) > 32:
                flush(batch)
                batch, used = [], set()
            batch.append(f)
            used |= more
        if batch:
            flush(batch)
    d.raw(0xDA380003, 0x0D000001+slots.index(restore)*64)
    d.end()
    archive.add(path, d.to_resource())
    if vertices:
        archive.add(path+"/V", asset.build_vertex_array(vertices))
    return len(faces)


def wrapper(archive, path, parts):
    d = asset.DLBuilder()
    d.marker(path)
    for part in parts:
        d.dl_hash(part)
    d.end()
    archive.add(path, d.to_resource())


def export_face_parts(archive, source, model, rig, variant, texpath):
    """Native facial states on the source surface, with an exact neutral head.

    GLBs have no expression morphs. Retexture source eye/mouth fragments using
    their own skin/line atlas pixels; keep their original surface and normals.
    No floating eyelid caps, diffuse repainting or replacement facial artwork.
    """
    landmarks = {
        "saria-anime": (0.031, 0.853, 0.025, 0.022, 0.815, 0.023),
        "saria-render": (0.031, 0.864, 0.025, 0.024, 0.826, 0.022),
        "twins": (0.040, 0.840, 0.028, 0.027, 0.798, 0.027),
        "young-malon": (0.031, 0.855, 0.027, 0.025, 0.803, 0.023),
        "malon": (0.030, 0.861, 0.027, 0.025, 0.829, 0.024),
    }
    eye_x, eye_y, eye_rx, eye_ry, mouth_y, mouth_rx = landmarks[variant]
    p, uv = model["source_positions"], model["uv"]
    rgba = np.asarray(source.image)
    rgb = rgba[np.clip((uv[:, 1]*rgba.shape[0]).astype(int), 0, rgba.shape[0]-1),
               np.clip((uv[:, 0]*rgba.shape[1]).astype(int), 0, rgba.shape[1]-1), :3].astype(float)
    front = (abs(p[:, 0]) < 0.12) & (p[:, 1] > mouth_y-0.04) & (p[:, 1] < eye_y+0.07) & (p[:, 2] > 0.015)
    skin = (front & (rgb[:, 0] > 140) & (rgb[:, 1] > 115) & (rgb[:, 2] > 70) &
            (rgb[:, 1] > rgb[:, 0]*0.72) & (rgb[:, 1] < rgb[:, 0]*1.16) &
            (rgb[:, 2] > rgb[:, 0]*0.45) & (rgb[:, 2] < rgb[:, 0]*0.9))
    dark = front & (rgb.mean(1) < 70)
    if not skin.any() or not dark.any():
        raise ValueError("Source face lacks clean skin/line samples")

    def sample(mask, target):
        ids = np.flatnonzero(mask)
        return uv[ids[np.linalg.norm(p[ids, :2]-np.asarray(target), axis=1).argmin()]]

    head = rig.profile["head"]
    faces = model["faces"][model["hosts"] == head]
    remaining = np.ones(len(faces), bool)
    regions = {}
    for key, cx, cy, rx, ry in [("L", eye_x, eye_y, eye_rx, eye_ry),
                                ("R", -eye_x, eye_y, eye_rx, eye_ry),
                                ("M", 0, mouth_y, mouth_rx*1.25, 0.014)]:
        point = p[faces]
        inside = (((point[:, :, 0]-cx)/rx)**2+((point[:, :, 1]-cy)/ry)**2 < 1.25**2) & (point[:, :, 2] > 0.015)
        selected = inside.any(1) & remaining
        regions[key] = (faces[selected], cx, cy, rx, ry)
        remaining[selected] = False
    base_path = rig.ns+"/FaceParts/Base"
    emit_mesh(archive, "alt/"+base_path, model, rig, faces[remaining], texpath, head)
    parts = {}
    for key, (region, cx, cy, rx, ry) in regions.items():
        # Subdivide only expression regions; each state has the exact same
        # surface. Triangle-local UV choices cannot interpolate across islands.
        bary = np.broadcast_to(np.eye(3), (len(region), 3, 3)).copy()
        source_faces = region.copy()
        for _ in range(2):
            u, v, w = bary[:, 0], bary[:, 1], bary[:, 2]
            uv_mid, vw_mid, wu_mid = (u+v)/2, (v+w)/2, (w+u)/2
            bary = np.concatenate([np.stack(x, 1) for x in
                                   [(u, uv_mid, wu_mid), (uv_mid, v, vw_mid),
                                    (wu_mid, vw_mid, w), (uv_mid, vw_mid, wu_mid)]])
            source_faces = np.tile(source_faces, (4, 1))
        positions = np.einsum("fij,fjk->fik", bary, model["positions"][source_faces])
        normals = unit(np.einsum("fij,fjk->fik", bary, model["normals"][source_faces]))
        st = np.einsum("fij,fjk->fik", bary, model["uv"][source_faces])
        source_p = np.einsum("fij,fjk->fik", bary, p[source_faces])
        center = source_p.mean(1)
        ellipse = ((center[:, 0]-cx)/rx)**2+((center[:, 1]-cy)/ry)**2 < 1
        skin_uv = sample(skin, [cx, cy+ry+0.004 if key != "M" else cy-0.018])
        dark_uv = sample(dark, [cx, cy])
        for state in range(5):
            out_uv = st.copy()
            if state and key != "M":
                bottom = cy-ry*2 if state == 2 else cy+(-0.2*ry if state == 4 else 0)
                cover = ellipse & (center[:, 1] > bottom) if state != 3 else np.zeros(len(center), bool)
                out_uv[cover] = skin_uv
                # A soft closed lash line is placed on the same surface.
                line_y = cy-0.003+0.004*((center[:, 0]-cx)/rx)**2
                line = ellipse & (abs(center[:, 1]-line_y) < 0.0009) if state == 2 else np.zeros(len(center), bool)
                out_uv[line] = dark_uv
            elif state:
                out_uv[ellipse] = skin_uv
                x = (center[:, 0]-cx)/rx
                if state in (1, 3):
                    line = x*x+(center[:, 1]-cy)**2/(0.007 if state == 1 else 0.010)**2 < (0.7 if state == 1 else 0.4)**2
                else:
                    line_y = cy+(-0.005 if state == 2 else 0.001)*x*x
                    line = (abs(x) < 0.85) & (abs(center[:, 1]-line_y) < 0.001)
                out_uv[line & ellipse] = dark_uv
            output = dict(positions=positions.reshape(-1, 3), normals=normals.reshape(-1, 3),
                          uv=out_uv.reshape(-1, 2), bones=np.full(positions.size//3, head),
                          faces=np.arange(positions.size//3).reshape(-1, 3))
            path = rig.ns+f"/FaceParts/{key}{state}"
            emit_mesh(archive, "alt/"+path, output, rig, output["faces"], texpath, head)
            parts[key+str(state)] = path
    for left in range(5):
        for right in range(5):
            for mouth in range(5):
                selected = ([rig.ns+f"/Body/Limb{head:02d}"] if (left, right, mouth) == (0, 0, 0) else
                            [base_path, parts[f"L{left}"], parts[f"R{right}"], parts[f"M{mouth}"]])
                wrapper(archive, "alt/"+rig.ns+f"/Faces/L{left}R{right}M{mouth}", selected)


def build(source_path, native_path, variant, output):
    profile = VARIANTS[variant]["profile"]
    source = SourceModel(source_path)
    with zipfile.ZipFile(native_path) as native:
        rig = NativeRig(native, profile)
        model = fit_model(source, rig)
        archive = asset.O2R()
        texpath = rig.ns+"/Textures/Diffuse"
        pixels = np.asarray(source.image).tobytes()
        w, h = source.image.size
        archive.add(texpath, asset._header(asset.T_OTEX, 1)+struct.pack("<IIIIffI", 1, w, h, 1, w/512, h/512, len(pixels))+pixels)
        paths = []
        body = {}
        for i, limb in enumerate(rig.limbs):
            limb_path = rig.ns+f"/Rig/Limb{i:02d}"
            paths.append(limb_path)
            target = rig.ns+f"/Body/Limb{i:02d}" if limb.dlist else ""
            xyz = (limb.x, limb.y, limb.z)
            archive.add(limb_path, asset.build_limb(xyz, limb.child, limb.sibling, rig.near[i], "", rig.limb_type))
            archive.add("alt/"+limb_path, asset.build_limb(xyz, limb.child, limb.sibling, target, "", rig.limb_type))
            if target:
                body[str(i)] = emit_mesh(archive, "alt/"+target, model, rig,
                                         model["faces"][model["hosts"] == i], texpath, i)
        raw = asset.build_skeleton(paths, rig.limb_type, asset.SKEL_FLEX, rig.dlist_count)
        archive.add(rig.ns+"/Skel", raw)
        archive.add("alt/"+rig.ns+"/Skel", raw)
        export_face_parts(archive, source, model, rig, variant, texpath)
        if profile == "saria":
            # The native list starts with the ocarina batch, then changes to
            # hand skin. Keep only the prop batch; the custom hand stays visible.
            prop = native.read("objects/object_sa/gSariaRightHandAndOcarinaDL")
            if len(prop) != 696 or prop[384:392] != struct.pack("<II", 0xE7000000, 0):
                raise ValueError("Native Saria prop layout differs from the audited source")
            d = asset.DLBuilder()
            d.marker(rig.ns+"/Props/Ocarina")
            vertex_path = rig.ns+"/Props/OcarinaVertices"
            records = asset.read_vertex_array(native.read("objects/object_sa/object_saVtx_006948"))[:20]
            records = [(*[round(v*0.6) for v in row[:3]], *row[3:]) for row in records]
            archive.add("alt/"+vertex_path, asset.build_vertex_array(records))
            offset = 88
            while offset < 384:
                first, second = struct.unpack_from("<II", prop, offset)
                size = 16 if first >> 24 in (0x20, 0x31, 0x32, 0x33) else 8
                if first >> 24 == 0x32:
                    hashed = asset.crc64(vertex_path)
                    d.words.extend([first, second, hashed >> 32, hashed & 0xFFFFFFFF])
                else:
                    d.words.extend(struct.unpack_from(f"<{size//4}I", prop, offset))
                offset += size
            d.end()
            archive.add("alt/"+rig.ns+"/Props/Ocarina", d.to_resource())
            wrapper(archive, "alt/"+rig.ns+"/Props/RightHandOcarina",
                    [rig.ns+"/Body/Limb14", rig.ns+"/Props/Ocarina"])
        report = dict(variant=variant, source_sha256=hashlib.sha256(source.raw).hexdigest(),
                      native_sha256=hashlib.sha256(Path(native_path).read_bytes()).hexdigest(),
                      source_triangles=len(model["faces"]), body_triangles=body,
                      source_joints=len(source.joints), target_limbs=len(rig.limbs),
                      native_skeleton=rig.profile["skeleton"], namespace=rig.ns,
                      diffuse_pixel_sha256=hashlib.sha256(pixels).hexdigest(),
                      original_diffuse_size=[w, h], native_actor_types=rig.profile.get("native_actor_types"),
                      expression_method="native-indexed source-surface eye/mouth UV states; exact neutral artwork; expression art requires review",
                      runtime_tested=False, status="IMPLEMENTED_RUNTIME_UNTESTED")
        archive.add(variant.replace("-", "_")+"_NativeNpcModelInfo.json", json.dumps(report, indent=2).encode())
        output = Path(output)
        if output.resolve() in (Path(source_path).resolve(), Path(native_path).resolve()):
            raise ValueError("The candidate must be separate from both input files")
        output.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
            for path, raw in sorted(archive.files.items()):
                info = zipfile.ZipInfo(path, (2026, 10, 18, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                z.writestr(info, raw)
        np.savez_compressed(output.with_suffix(".npz"), **model,
                            bind_matrices=np.asarray([l.M for l in rig.limbs]),
                            bind_positions=np.asarray([l.T for l in rig.limbs]))
        return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native", type=Path)
    parser.add_argument("sources", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--variant", choices=list(VARIANTS))
    args = parser.parse_args()
    for name, variant in VARIANTS.items():
        if args.variant and args.variant != name:
            continue
        report = build(args.sources/variant["file"], args.native, name, args.output/variant["archive"])
        print(json.dumps(report), flush=True)


if __name__ == "__main__":
    main()
