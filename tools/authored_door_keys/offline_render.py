"""Native two-pass preview evaluator. Fixed diffuse light/look-at are approximations.
No PBR/specular shortcut: the moving highlight comes only from the native map.
Rasterization adapted from tools.nei_icons.render; evaluates per-vertex texgen and
coplanar alpha decal order, which the generic opaque-GI preview does not support.
"""
import numpy as np
from PIL import Image
from tools.nei_icons.render import rotation,unit

def sample_native(texture,uv,clamp):
    h,w=texture.shape[:2];xy=uv*[w,h]-.5
    if clamp:xy=np.clip(xy,[0,0],[w-1,h-1])
    else:xy=np.mod(xy,[w,h])
    lo=np.floor(xy).astype(int);fx,fy=(xy-lo).T[:,:,None];x,y=lo.T
    nx=np.minimum(x+1,w-1) if clamp else (x+1)%w
    ny=np.minimum(y+1,h-1) if clamp else (y+1)%h
    return ((texture[y,x]*(1-fx)+texture[y,nx]*fx)*(1-fy)
            +(texture[ny,x]*(1-fx)+texture[ny,nx]*fx)*fy)

def render(model, size=512, azimuth=20, elevation=10, roll=-45, supersample=2, fill=.88):
    """Render straight-alpha RGBA with transparent margins and no background art."""
    width = size * supersample
    rot = rotation("x", elevation) @ rotation("y", azimuth) @ rotation("z", roll)
    points = np.concatenate([p.positions @ rot.T for p in model.parts])
    low, high = points.min(axis=0), points.max(axis=0)
    center = (low + high) / 2
    scale = width * fill / max((high-low)[:2])
    pixels = np.zeros((width, width, 4), dtype=float)  # premultiplied RGBA
    depth_buffer = np.full((width, width), -np.inf)
    light = unit([-.42, .72, .72])
    half = unit(light + [0, 0, 1])
    work = []
    for part in model.parts:
        positions = part.positions @ rot.T - center
        normals = unit(part.normals @ rot.T)
        screen = np.c_[width/2 + positions[:, 0]*scale, width/2 - positions[:, 1]*scale]
        for face in part.triangles:
            # Transparent faces need depth ordering even inside one primitive.
            work.append((part.material.color[3] < .999, float(positions[face, 2].mean()), part, face, positions, normals, screen))
    # Keep actual native submission order: opaque reflection, then coplanar coat.
    for transparent, _, part, face, positions, normals, screen in work:
        mat = part.material
        face_normal = np.cross(positions[face[1]]-positions[face[0]], positions[face[2]]-positions[face[0]])
        if mat.cull and face_normal[2] <= 0:
            continue
        v = screen[face]
        x0, y0 = v[0]; x1, y1 = v[1]; x2, y2 = v[2]
        area = (x1-x0)*(y2-y0)-(x2-x0)*(y1-y0)
        if abs(area) < 1e-8:
            continue
        xmin, ymin = np.maximum(0, np.floor(v.min(axis=0)).astype(int))
        xmax, ymax = np.minimum(width-1, np.ceil(v.max(axis=0)).astype(int))
        if xmax < xmin or ymax < ymin:
            continue
        xx, yy = np.meshgrid(np.arange(xmin, xmax+1)+.5, np.arange(ymin, ymax+1)+.5)
        w0 = ((x1-xx)*(y2-yy)-(x2-xx)*(y1-yy))/area
        w1 = ((x2-xx)*(y0-yy)-(x0-xx)*(y2-yy))/area
        w2 = 1-w0-w1
        z = positions[face, 2]
        depth = w0*z[0] + w1*z[1] + w2*z[2]
        zb = depth_buffer[ymin:ymax+1, xmin:xmax+1]
        depth_pass=(depth >= zb-1e-7) if mat.decal else (depth > zb+1e-9)
        mask = (w0 >= -1e-8) & (w1 >= -1e-8) & (w2 >= -1e-8) & depth_pass
        if not mask.any():
            continue
        weights = np.stack([w0[mask], w1[mask], w2[mask]], axis=1)
        normal = unit(weights @ normals[face])
        diffuse = weights @ np.maximum(normals[face] @ light, 0)
        shade = .46 + .54*diffuse
        base = np.broadcast_to(np.array(mat.color), (len(weights), 4)).copy()
        if mat.texture is not None:
            if mat.texgen:
                # RSP generates coordinates per vertex, before interpolation.
                ns=np.clip(normals[face,:2],-1,1)
                generated=(np.arccos(-ns)/(2*np.pi) if mat.texgen_linear else (ns+1)/4)
                generated=np.trunc(generated*np.array(mat.tex_scale))
                uv=weights @ (generated/(32*np.array(mat.texture.shape[1::-1])))
            else:
                uv = weights @ part.uv[face]
            tex = sample_native(mat.texture, uv,mat.clamp)
            if mat.environment is not None:
                base[:, :3] = np.array(mat.environment[:3]) + (base[:, :3] - mat.environment[:3]) * tex[:, :3]
            else:
                base *= tex
        elif mat.environment is not None and mat.texgen:
            reflection = (.62 + .28 * diffuse)[:, None]
            base[:, :3] = np.array(mat.environment[:3]) + (base[:, :3] - mat.environment[:3]) * reflection
        shade = shade * (1-mat.emissive) + mat.emissive
        rgb = np.clip(base[:, :3]*shade[:, None], 0, 1)
        alpha = base[:, 3:4]
        target = pixels[ymin:ymax+1, xmin:xmax+1]
        target[mask, :3] = rgb*alpha + target[mask, :3]*(1-alpha)
        target[mask, 3:4] = alpha + target[mask, 3:4]*(1-alpha)
        if not mat.decal:
            zb[mask] = depth[mask]
    # Downsample premultiplied colors before unpremultiplying: no dark fringes.
    # Pillow's RGBA resize performs its own premultiplication. Resize the four
    # already-premultiplied channels separately so transparent edges stay exact.
    data = np.stack([np.asarray(Image.fromarray(pixels[:, :, c].astype(np.float32), "F")
                                .resize((size, size), Image.Resampling.LANCZOS))
                     for c in range(4)], axis=-1)
    data = np.clip(data, 0, 1)
    alpha = data[:, :, 3:4]
    data[:, :, :3] = np.divide(data[:, :, :3], alpha, out=np.zeros_like(data[:, :, :3]), where=alpha > 0)
    return Image.fromarray(np.uint8(np.clip(data*255+.5, 0, 255)), "RGBA")
