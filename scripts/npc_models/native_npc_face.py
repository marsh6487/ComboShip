"""UV baking and seam-safe expression textures for the supplied source meshes.

The projection follows the recovered MIT Fado face baker. Artwork is authored
separately; this module only transfers pixels between source and native UVs.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image
from scipy.ndimage import binary_erosion, binary_fill_holes, binary_propagation, distance_transform_edt, map_coordinates

LANDMARKS = {
    "saria-anime": (.031, .853, .025, .022, .815, .023),
    "saria-render": (.031, .864, .025, .024, .826, .022),
    "twins": (.040, .840, .028, .027, .798, .027),
    "young-malon": (.031, .855, .027, .025, .803, .023),
    "malon": (.030, .861, .027, .025, .829, .024),
}
SIZE = 512


def face_bounds(variant):
    _, y, _, _, mouth, _ = LANDMARKS[variant]
    return [-.095, mouth-.035, .095, y+.065]


def bake(source, uv, atlas, faces, bounds, size=SIZE, face_labels=None):
    """Reproject source pixels, optionally returning each visible face's label.

    Labels use the exact same depth test as the pixels. Uncovered pixels keep
    label -1 even though their colors are extended to provide texture padding.
    """
    x0, y0, x1, y1 = bounds
    xy = (source[:, :2]-[x0, y0])/[x1-x0, y1-y0]
    xy[:, 1] = 1-xy[:, 1]
    points = xy*(size-1)
    out, depth = np.zeros((size, size, 4), float), np.full((size, size), -np.inf)
    owners = np.full((size, size), -1, int) if face_labels is not None else None
    for index, f in enumerate(faces):
        q = points[f]
        lo = np.maximum(np.floor(q.min(0)).astype(int), 0)
        hi = np.minimum(np.ceil(q.max(0)).astype(int), size-1)
        if np.any(hi < lo):
            continue
        xx, yy = np.meshgrid(np.arange(lo[0], hi[0]+1), np.arange(lo[1], hi[1]+1))
        matrix = np.column_stack([q[0]-q[2], q[1]-q[2]])
        if abs(np.linalg.det(matrix)) < 1e-10:
            continue
        b = (np.stack([xx, yy], -1)-q[2])@np.linalg.inv(matrix).T
        weights = np.dstack([b, 1-b.sum(-1)])
        z = weights@source[f, 2]
        mask = (weights.min(-1) >= -1e-5) & (z >= depth[yy, xx])
        if not mask.any():
            continue
        st = weights@uv[f]
        coord = [st[..., 1]*(atlas.shape[0]-1), st[..., 0]*(atlas.shape[1]-1)]
        color = np.stack([map_coordinates(atlas[..., c], coord, order=1, mode="nearest")
                          for c in range(4)], -1)
        out[yy[mask], xx[mask]], depth[yy[mask], xx[mask]] = color[mask], z[mask]
        if owners is not None:
            owners[yy[mask], xx[mask]] = face_labels[index]
    valid = np.isfinite(depth)
    if not valid.any():
        raise ValueError("Source projection has no visible face pixels")
    nearest = distance_transform_edt(~valid, return_distances=False, return_indices=True)
    out = np.uint8(np.clip(out[tuple(nearest)], 0, 255))
    out[..., 3] = 255
    return (out, xy, valid, owners) if owners is not None else (out, xy, valid)


def eye_mask(variant, bounds, side=None, size=SIZE):
    x, y, rx, ry, _, _ = LANDMARKS[variant]
    x0, y0, x1, y1 = bounds
    yy, xx = np.indices((size, size))
    sx, sy = x0+xx/(size-1)*(x1-x0), y1-yy/(size-1)*(y1-y0)
    centers = [x] if side == "L" else [-x] if side == "R" else [-x, x]
    return np.logical_or.reduce([((sx-cx)/rx)**4+((sy-y)/ry)**4 < 1 for cx in centers])


def compose_eye_state(base, authored, variant, bounds):
    """Keep source pixels outside the eyelids and feather only inside the edge."""
    mask = eye_mask(variant, bounds, size=base.shape[0])
    weight = np.clip(distance_transform_edt(mask)/5, 0, 1)
    out = np.uint8(np.rint(base.astype(float)*(1-weight[..., None])+authored*weight[..., None]))
    return out


def compose_visible_gaze(base, authored, owners, eyeball_ids, inset=2, aperture_mask=None):
    """Center the gaze inside the visible aperture, protecting skin and lashes.

    The supplied head also paints part of each iris on the underlying face.
    Grow from visible eyeballs only through connected white/blue face pixels;
    filling enclosed pupils keeps the old gaze from surviving as a crescent.
    """
    bounds = np.ones(owners.shape, bool) if aperture_mask is None else aperture_mask
    eyes = np.isin(owners, eyeball_ids) & bounds
    rgb = base[..., :3].astype(float)
    eye_color = (rgb[..., 2] > 45) & (rgb[..., 2] > rgb[..., 1]*.72)
    allowed = (eyes | ((owners == 6) & eye_color)) & bounds
    mask = binary_fill_holes(binary_propagation(eyes, mask=allowed))
    mask &= ((owners == 6) | eyes) & bounds
    art_rgb = authored[..., :3].astype(float)
    art_eye = (art_rgb[..., 2] > 45) & (art_rgb[..., 2] > art_rgb[..., 1]*.72) & mask
    art_eye |= binary_fill_holes(art_eye) & (art_rgb.max(2) < 90) & mask
    if not art_eye.any():
        return base.copy()
    # The gaze repaint made its aperture smaller than the source eye. Extend
    # its existing sclera/iris into the interior skin strip; copying that strip
    # would leave the preserved original lower lash visibly detached.
    nearest = distance_transform_edt(~art_eye, return_distances=False, return_indices=True)
    gaze = authored[tuple(nearest)]
    mask = binary_erosion(mask, iterations=inset)
    weight = np.clip(distance_transform_edt(mask)/2, 0, 1)[..., None]
    return np.uint8(np.rint(base.astype(float)*(1-weight)+gaze.astype(float)*weight))


def visible_skin_vertices(model, faces, skin_mesh, eyeball_meshes):
    """Retain source skin that is outside or in front of the open eyeballs.

    The underlying skin inside an eye aperture may become an eyelid. Visible
    cheek, brow and lash-rim vertices retain their original depth and shading.
    """
    p, mesh_ids = model["source_positions"], model["mesh_ids"]
    ids = np.unique(faces)
    skin = ids[mesh_ids[ids] == skin_mesh]
    eye = faces[np.isin(mesh_ids[faces], eyeball_meshes).all(1)]
    if not len(eye) or not len(skin):
        return skin
    q = p[eye]
    a, b = q[:, 0, :2]-q[:, 2, :2], q[:, 1, :2]-q[:, 2, :2]
    det = a[:, 0]*b[:, 1]-a[:, 1]*b[:, 0]
    good = abs(det) > 1e-12
    q, a, b, det = q[good], a[good], b[good], det[good]
    if not len(q):
        return skin
    delta = p[skin, None, :2]-q[None, :, 2, :2]
    u = (delta[..., 0]*b[:, 1]-delta[..., 1]*b[:, 0])/det
    v = (delta[..., 1]*a[:, 0]-delta[..., 0]*a[:, 1])/det
    covered = (u >= -1e-6) & (v >= -1e-6) & (u+v <= 1+1e-6)
    depth = u*q[:, 0, 2]+v*q[:, 1, 2]+(1-u-v)*q[:, 2, 2]
    depth[~covered] = -np.inf
    return skin[depth.max(1) <= p[skin, 2]+1e-6]


def shared_patch_vertices(positions, all_faces, patch_faces):
    """Only vertices shared with the retained face are expression seams.

    An open eye shell or UV split inside the patch is not a protected seam.
    """
    chosen = set(map(tuple, np.sort(patch_faces, axis=1)))
    outside = np.array([f for f in all_faces if tuple(sorted(f)) not in chosen], dtype=int)
    if not len(outside):
        return np.empty(0, dtype=int)
    _, weld = np.unique(np.round(positions, 6), axis=0, return_inverse=True)
    ids = np.unique(patch_faces)
    return ids[np.isin(weld[ids], np.unique(weld[outside]))]


def expression_subdivision(faces, levels=3):
    """Refine talking detail without inserting vertices on shared edges.

    Native integer rounding moves an edge midpoint off its neighbor's unsplit
    edge. Centroid fans keep every existing perimeter edge exactly intact.
    """
    bary = np.broadcast_to(np.eye(3), (len(faces), 3, 3)).copy()
    source_faces = faces.copy()
    for _ in range(levels):
        a, b, c = bary[:, 0], bary[:, 1], bary[:, 2]
        center = (a+b+c)/3
        bary = np.concatenate([np.stack(triangle, 1) for triangle in
                               [(a, b, center), (b, c, center), (c, a, center)]])
        source_faces = np.tile(source_faces, (3, 1))
    return bary, source_faces


def make_lid_surface(model, faces, coef, center, bounds, head, preserve_vertices=None,
                     preserve_topology=False):
    """One triangulated skin surface replaces overlapping open-eye shells.

    Shared outer vertices retain exact fitted positions and shading. Interior
    vertices follow the skin fitted around the eye, with continuous normals.
    """
    from scipy.spatial import Delaunay
    from native_npc_models import unit
    p = model["source_positions"]
    ids = np.unique(faces)
    xy, inverse = np.unique(np.round(p[ids, :2], 6), axis=0, return_inverse=True)
    cx, cy = center
    x, y = xy[:, 0]-cx, xy[:, 1]-cy
    matrix = np.column_stack([np.ones(len(x)), x, y, x*x, y*y, x*y])
    source_p = np.column_stack([xy, matrix@coef])
    transform = model["head_transform"]
    positions = source_p@transform[:, :3].T+transform[:, 3]
    dx, dy = coef[1]+2*coef[3]*x+coef[5]*y, coef[2]+2*coef[4]*y+coef[5]*x
    normals = unit(np.column_stack([-dx, -dy, np.ones(len(x))])@np.linalg.inv(transform[:, :3]))
    seam = shared_patch_vertices(p, model["faces"], faces)
    if preserve_vertices is not None:
        seam = np.union1d(seam, preserve_vertices)
    for group in np.unique(inverse[np.isin(ids, seam)]):
        match = ids[(inverse == group) & np.isin(ids, seam)]
        # Frontmost boundary is the one visible beside the retained face.
        index = match[np.argmax(p[match, 2])]
        positions[group], normals[group] = model["positions"][index], model["normals"][index]
    if preserve_topology:
        # A centroid test on unconstrained Delaunay triangles can cut tiny
        # corners out of a concave source boundary. Keep the skin's own edges.
        lookup = np.full(len(p), -1, int)
        lookup[ids] = np.arange(len(ids))
        triangles = inverse[lookup[faces]]
        triangles = triangles[(triangles[:, 0] != triangles[:, 1]) &
                              (triangles[:, 1] != triangles[:, 2]) &
                              (triangles[:, 2] != triangles[:, 0])]
    else:
        triangles = Delaunay(xy).simplices
        # Keep the projected union of the original eye patch, rather than filling
        # a concave gap outside its selection boundary.
        q = p[faces, :2]
        a, b = q[:, 0]-q[:, 2], q[:, 1]-q[:, 2]
        det = a[:, 0]*b[:, 1]-a[:, 1]*b[:, 0]
        good = abs(det) > 1e-12
        q, a, b, det = q[good], a[good], b[good], det[good]
        delta = xy[triangles].mean(1)[:, None, :]-q[None, :, 2, :]
        u = (delta[..., 0]*b[:, 1]-delta[..., 1]*b[:, 0])/det
        v = (delta[..., 1]*a[:, 0]-delta[..., 0]*a[:, 1])/det
        triangles = triangles[((u >= -1e-6) & (v >= -1e-6) & (u+v <= 1+1e-6)).any(1)]
    uv = (xy-np.array(bounds[:2]))/np.subtract(bounds[2:], bounds[:2])
    uv[:, 1] = 1-uv[:, 1]
    return dict(positions=positions, normals=normals, uv=uv, bones=np.full(len(xy), head), faces=triangles)


def make_front_lid_surface(model, faces, coef, center, bounds, head):
    """Keep unsupported side-skin triangles on their original source surface.

    The artwork bake only includes triangles wholly in front of source z=.015.
    Joining side/back skin points to that frontal surface stretches a small
    outer lash into a branch in angled views. Keep those original triangles,
    and protect their shared seam when triangulating the supported lid.
    """
    p = model['source_positions']
    skin = (model['mesh_ids'][faces] == 6).all(1)
    side = skin & (p[faces, 2] <= .015).any(1)
    front = faces[~side]
    preserve = visible_skin_vertices(model, front, 6, (14, 15))
    # The skin continues under the eyeballs. Its original topology supplies a
    # joined lid without retaining a second, overlapping eyeball shell.
    front = front[(model['mesh_ids'][front] == 6).all(1)]
    lid = make_lid_surface(model, front, coef, center, bounds, head,
                           preserve_vertices=preserve, preserve_topology=True)
    return lid, faces[side]


def export_eye(archive, source, model, rig, faces, variant, side, texpath):
    """Export original open eye plus authored lids on a seam-matched surface."""
    import struct
    from native_npc_models import emit_mesh, asset, wrapper
    head = rig.profile["head"]
    directory = Path(__file__).with_name("face_art")/variant
    spec = json.loads((directory/"projection.json").read_text())
    if spec["source_sha256"] != hashlib.sha256(source.raw).hexdigest():
        raise ValueError("Eyelid artwork belongs to a different source model")
    x0, y0, x1, y1 = spec["bounds"]
    ids = np.unique(faces)
    lut = np.full(len(model["positions"]), -1, int)
    lut[ids] = np.arange(len(ids))
    source_p = model["source_positions"][ids].copy()
    uv = (source_p[:, :2]-[x0, y0])/[x1-x0, y1-y0]
    uv[:, 1] = 1-uv[:, 1]
    cx, cy, rx, ry, _, _ = LANDMARKS[variant]
    cx *= 1 if side == "L" else -1
    p = model["source_positions"]
    radius = ((p[:, 0]-cx)/rx)**2+((p[:, 1]-cy)/ry)**2
    rgb = np.asarray(source.image)[np.clip((model["uv"][:, 1]*source.image.height).astype(int), 0, source.image.height-1),
                                   np.clip((model["uv"][:, 0]*source.image.width).astype(int), 0, source.image.width-1), :3].astype(float)
    skin = (rgb[:, 0] > rgb[:, 1]*1.02) & (rgb[:, 1] > rgb[:, 2]*1.08) & (rgb.mean(1) > 90)
    ring = (radius > 1.1) & (radius < 3.5) & (p[:, 2] > .015) & skin
    if variant == "saria-render":
        ring &= model["mesh_ids"] == 6
    def polynomial(points):
        x, y = points[:, 0]-cx, points[:, 1]-cy
        return np.column_stack([np.ones(len(points)), x, y, x*x, y*y, x*y])
    coef = np.linalg.lstsq(polynomial(p[ring]), p[ring, 2], rcond=None)[0]
    if variant=='twins':coef=model['twin_eye_curve_'+side]
    side_path = None
    if variant == 'saria-render':
        lid, retained = make_front_lid_surface(model, faces, coef, (cx, cy), spec['bounds'], head)
        if len(retained):
            side_path = rig.ns+f'/FaceParts/{side}Side'
            emit_mesh(archive, 'alt/'+side_path, model, rig, retained, texpath, head)
    else:
        lid = make_lid_surface(model, faces, coef, (cx, cy), spec['bounds'], head,
                               preserve_topology=variant == 'saria-anime')
    parts = {}
    joined_open = variant == 'twins'
    neutral_texture = export_neutral_projection(archive,source,model,rig,variant) if joined_open else None
    for state in range(5):
        path = rig.ns+f"/FaceParts/{side}{state}"
        texture = texpath
        label = ("half" if state == 1 else "closed") if state in (1, 2) else "neutral"
        if joined_open and state not in (1,2):
            emit_mesh(archive,'alt/'+path,lid,rig,lid['faces'],neutral_texture,head)
            parts[side+str(state)]=path
            continue
        if state in (1, 2) or (directory/"neutral.png").exists():
            texture = rig.ns+f"/Textures/Eyes{label.title()}"
            if "alt/"+texture not in archive.files:
                pixels = np.asarray(Image.open(directory/(label+".png")).convert("RGBA"))
                if variant == "saria-render" and label == "neutral":
                    front = model["faces"][(p[model["faces"], 2] > .015).all(1)]
                    base, _, _, owners = bake(p, model["uv"], np.asarray(source.image), front,
                                               spec["bounds"], face_labels=model["mesh_ids"][front[:, 0]])
                    pixels = compose_visible_gaze(base, pixels, owners, (14, 15), inset=2,
                                                   aperture_mask=eye_mask(variant, spec["bounds"]))
                archive.add("alt/"+texture, asset._header(asset.T_OTEX, 1)+
                    struct.pack("<IIIIffI", 1, SIZE, SIZE, 1, 1., 1., pixels.nbytes)+pixels.tobytes())
            # Neutral paint revisions retain the source surface. Blink states
            # use the joined skin patch instead of the open-eye shells.
            output = dict(positions=model["positions"][ids], normals=model["normals"][ids], uv=uv,
                          bones=np.full(len(ids), head), faces=lut[faces])
            if state in (1, 2):
                output = lid
            if state in (1, 2) and side_path:
                emit_mesh(archive, 'alt/'+path+'/Lid', output, rig, output['faces'], texture, head)
                wrapper(archive, 'alt/'+path, [path+'/Lid', side_path])
            else:
                emit_mesh(archive, "alt/"+path, output, rig, output["faces"], texture, head)
        else:
            emit_mesh(archive, "alt/"+path, model, rig, faces, texture, head)
        parts[side+str(state)] = path
    return parts


def export_neutral_projection(archive,source,model,rig,variant):
    """Bake existing source pixels once; never repaint the approved face."""
    import struct
    from native_npc_models import asset
    path=rig.ns+'/Textures/EyesNeutral'
    if 'alt/'+path in archive.files:return path
    directory=Path(__file__).with_name('face_art')/variant
    bounds=json.loads((directory/'projection.json').read_text())['bounds']
    faces=model['faces'][(model['hosts']==rig.profile['head'])&model['twin_face_piece'][model['faces'][:,0]]]
    pixels,_,_=bake(model['source_positions'],model['uv'],np.asarray(source.image),faces,bounds)
    archive.add('alt/'+path,asset._header(asset.T_OTEX,1)+
                struct.pack('<IIIIffI',1,SIZE,SIZE,1,1.,1.,pixels.nbytes)+pixels.tobytes())
    return path


def export_cheek(archive, model, rig, faces, variant):
    """A fixed texture repair on unchanged cheek triangles, shared by all states."""
    from native_npc_models import emit_mesh
    directory = Path(__file__).with_name("face_art")/variant
    bounds = json.loads((directory/"projection.json").read_text())["bounds"]
    x0, y0, x1, y1 = bounds
    ids = np.unique(faces)
    lut = np.full(len(model["positions"]), -1, int)
    lut[ids] = np.arange(len(ids))
    p = model["source_positions"][ids]
    uv = (p[:, :2]-[x0, y0])/[x1-x0, y1-y0]
    uv[:, 1] = 1-uv[:, 1]
    texture = rig.ns+"/Textures/EyesNeutral"
    path = rig.ns+"/FaceParts/Cheek"
    output = {key: model[key][ids] for key in ("positions", "normals", "bones")}
    output.update(uv=uv, faces=lut[faces])
    emit_mesh(archive, "alt/"+path, output, rig, output["faces"], texture, rig.profile["head"])
    return path


def export_mouth(archive, model, rig, faces, variant):
    """A narrow authored mouth edit on the existing face geometry."""
    import struct
    from native_npc_models import emit_mesh, asset
    directory = Path(__file__).with_name("face_art")/variant
    bounds = json.loads((directory/"projection.json").read_text())["bounds"]
    x0, y0, x1, y1 = bounds
    ids = np.unique(faces)
    lut = np.full(len(model["positions"]), -1, int); lut[ids] = np.arange(len(ids))
    p = model["source_positions"][ids]
    uv = (p[:, :2]-[x0, y0])/[x1-x0, y1-y0]; uv[:, 1] = 1-uv[:, 1]
    texture = rig.ns+"/Textures/MouthNeutral"
    pixels = np.asarray(Image.open(directory/"mouth-neutral.png").convert("RGBA"))
    archive.add("alt/"+texture, asset._header(asset.T_OTEX, 1)+
                struct.pack("<IIIIffI", 1, SIZE, SIZE, 1, 1., 1., pixels.nbytes)+pixels.tobytes())
    path = rig.ns+"/FaceParts/M0"
    output = {key: model[key][ids] for key in ("positions", "normals", "bones")}
    output.update(uv=uv, faces=lut[faces])
    emit_mesh(archive, "alt/"+path, output, rig, output["faces"], texture, rig.profile["head"])
    return path


def export_hair_top(archive, model, rig, faces):
    """Transfer the narrow crown cleanup to the unchanged hair surface."""
    import struct
    from native_npc_models import emit_mesh, asset
    directory = Path(__file__).with_name("face_art")/"young-malon"
    bounds = json.loads((directory/"hair-projection.json").read_text())["bounds"]
    ids = np.unique(faces); lut = np.full(len(model["positions"]), -1, int); lut[ids] = np.arange(len(ids))
    p = model["source_positions"][ids]
    uv = (p[:, :2]-bounds[:2])/(np.asarray(bounds[2:])-bounds[:2]); uv[:, 1] = 1-uv[:, 1]
    texture = rig.ns+"/Textures/HairCrown"
    pixels = np.asarray(Image.open(directory/"hair-top.png").convert("RGBA"))
    archive.add("alt/"+texture, asset._header(asset.T_OTEX, 1)+
                struct.pack("<IIIIffI", 1, SIZE, SIZE, 1, 1., 1., pixels.nbytes)+pixels.tobytes())
    path = rig.ns+"/FaceParts/HairCrown"
    output = {key: model[key][ids] for key in ("positions", "normals", "bones")}
    output.update(uv=uv, faces=lut[faces])
    emit_mesh(archive, "alt/"+path, output, rig, output["faces"], texture, rig.profile["head"])
    return path


def prepare(source, variant, directory):
    p = np.concatenate([mesh["positions"] for mesh in source.meshes])
    uv = np.concatenate([mesh["uv"] for mesh in source.meshes])
    fs, offset = [], 0
    for mesh in source.meshes:
        fs.append(mesh["faces"]+offset)
        offset += len(mesh["positions"])
    faces = np.concatenate(fs)
    faces = faces[(p[faces, 2] > .015).all(1)]
    bounds = face_bounds(variant)
    image, _, _ = bake(p, uv, np.asarray(source.image), faces, bounds)
    directory = Path(directory)/variant
    directory.mkdir(parents=True, exist_ok=True)
    Image.fromarray(image).save(directory/"source.png")
    paired = Image.new("RGBA", (SIZE*2, SIZE))
    paired.paste(Image.fromarray(image), (0, 0))
    paired.paste(Image.fromarray(image), (SIZE, 0))
    paired.save(directory/"edit-target.png")
    (directory/"projection.json").write_text(json.dumps(dict(variant=variant, bounds=bounds,
        source_sha256=hashlib.sha256(source.raw).hexdigest(), panels=["half", "closed"]), indent=2)+"\n")


if __name__ == "__main__":
    from native_npc_models import SourceModel, VARIANTS
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sources", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    for variant, settings in VARIANTS.items():
        prepare(SourceModel(args.sources/settings["file"]), variant, args.output)
        print("Baked", variant, flush=True)
