/* Rito-only post-native foot contact. Header-local C code; no new build unit.
 * Uses the authored offsets and support bounds in rito_feet_support.h.
 * All trial math is private. The caller retains the native pose on any failure.
 */
#ifndef MODS_FORMS_RITO_FOOT_CONTACT_H
#define MODS_FORMS_RITO_FOOT_CONTACT_H

#include <float.h>
#include <math.h>
#include <string.h>
#include "mods/forms/rito_feet_support.h"

typedef struct {
    f64 x, y, z;
} RitoFeetVector;

typedef struct {
    f64 values[3][3];
} RitoFeetMatrix;

typedef struct {
    Vec3s joints[3];
    MtxF segments[3];
} RitoFeetPose;

static RitoFeetVector RitoFeet_Add(RitoFeetVector a, RitoFeetVector b) {
    RitoFeetVector result = { a.x + b.x, a.y + b.y, a.z + b.z };
    return result;
}

static RitoFeetVector RitoFeet_Subtract(RitoFeetVector a, RitoFeetVector b) {
    RitoFeetVector result = { a.x - b.x, a.y - b.y, a.z - b.z };
    return result;
}

static RitoFeetVector RitoFeet_Scale(RitoFeetVector vector, f64 scale) {
    RitoFeetVector result = { vector.x * scale, vector.y * scale, vector.z * scale };
    return result;
}

static f64 RitoFeet_Dot(RitoFeetVector a, RitoFeetVector b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static RitoFeetVector RitoFeet_Cross(RitoFeetVector a, RitoFeetVector b) {
    RitoFeetVector result = { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
    return result;
}

static f64 RitoFeet_Length(RitoFeetVector vector) {
    return sqrt(RitoFeet_Dot(vector, vector));
}

static RitoFeetMatrix RitoFeet_Linear(const MtxF* matrix) {
    RitoFeetMatrix result;
    s32 row, column;
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            result.values[row][column] = matrix->mf[column][row];
        }
    }
    return result;
}

static RitoFeetVector RitoFeet_Origin(const MtxF* matrix) {
    RitoFeetVector result = { matrix->xw, matrix->yw, matrix->zw };
    return result;
}

static RitoFeetVector RitoFeet_Column(const RitoFeetMatrix* matrix, s32 column) {
    RitoFeetVector result = { matrix->values[0][column], matrix->values[1][column], matrix->values[2][column] };
    return result;
}

static RitoFeetMatrix RitoFeet_Multiply(const RitoFeetMatrix* a, const RitoFeetMatrix* b) {
    RitoFeetMatrix result = { 0 };
    s32 row, column, inner;
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            for (inner = 0; inner < 3; inner++) {
                result.values[row][column] += a->values[row][inner] * b->values[inner][column];
            }
        }
    }
    return result;
}

static RitoFeetVector RitoFeet_Transform(const RitoFeetMatrix* matrix, RitoFeetVector vector) {
    RitoFeetVector result = {
        matrix->values[0][0] * vector.x + matrix->values[0][1] * vector.y + matrix->values[0][2] * vector.z,
        matrix->values[1][0] * vector.x + matrix->values[1][1] * vector.y + matrix->values[1][2] * vector.z,
        matrix->values[2][0] * vector.x + matrix->values[2][1] * vector.y + matrix->values[2][2] * vector.z,
    };
    return result;
}

static f64 RitoFeet_Determinant(const RitoFeetMatrix* matrix) {
    return RitoFeet_Dot(RitoFeet_Column(matrix, 0),
                        RitoFeet_Cross(RitoFeet_Column(matrix, 1), RitoFeet_Column(matrix, 2)));
}

static u8 RitoFeet_Inverse(const RitoFeetMatrix* matrix, RitoFeetMatrix* result) {
    f64 determinant = RitoFeet_Determinant(matrix);
    RitoFeetVector rows[3];
    s32 row;
    if (!isfinite(determinant) || fabs(determinant) < DBL_EPSILON) {
        return false;
    }
    rows[0] = RitoFeet_Scale(RitoFeet_Cross(RitoFeet_Column(matrix, 1), RitoFeet_Column(matrix, 2)), 1.0 / determinant);
    rows[1] = RitoFeet_Scale(RitoFeet_Cross(RitoFeet_Column(matrix, 2), RitoFeet_Column(matrix, 0)), 1.0 / determinant);
    rows[2] = RitoFeet_Scale(RitoFeet_Cross(RitoFeet_Column(matrix, 0), RitoFeet_Column(matrix, 1)), 1.0 / determinant);
    for (row = 0; row < 3; row++) {
        result->values[row][0] = rows[row].x;
        result->values[row][1] = rows[row].y;
        result->values[row][2] = rows[row].z;
    }
    return true;
}

/* Shortest-arc transport retains the current knee side without an Euler-axis assumption. */
static u8 RitoFeet_Arc(RitoFeetVector from, RitoFeetVector to, RitoFeetMatrix* result) {
    f64 fromLength = RitoFeet_Length(from), toLength = RitoFeet_Length(to), cosine;
    RitoFeetVector axis;
    RitoFeetMatrix crossMatrix = { 0 }, square;
    s32 row, column;
    if (fromLength <= DBL_EPSILON || toLength <= DBL_EPSILON) {
        return false;
    }
    from = RitoFeet_Scale(from, 1.0 / fromLength);
    to = RitoFeet_Scale(to, 1.0 / toLength);
    cosine = RitoFeet_Dot(from, to);
    if (!isfinite(cosine) || cosine <= -1.0 + FLT_EPSILON) {
        return false;
    }
    axis = RitoFeet_Cross(from, to);
    crossMatrix.values[0][1] = -axis.z;
    crossMatrix.values[0][2] = axis.y;
    crossMatrix.values[1][0] = axis.z;
    crossMatrix.values[1][2] = -axis.x;
    crossMatrix.values[2][0] = -axis.y;
    crossMatrix.values[2][1] = axis.x;
    square = RitoFeet_Multiply(&crossMatrix, &crossMatrix);
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            result->values[row][column] = (row == column ? 1.0 : 0.0) + crossMatrix.values[row][column] +
                                          square.values[row][column] / (1.0 + cosine);
        }
    }
    return true;
}

static Vec3s RitoFeet_NativeEuler(const RitoFeetMatrix* rotation) {
    MtxF matrix = { 0 };
    Vec3s angles;
    s32 row, column;
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            matrix.mf[column][row] = rotation->values[row][column];
        }
    }
    matrix.ww = 1.0f;
    Matrix_MtxFToZYXRot(&matrix, &angles, true);
    return angles;
}

static RitoFeetPose RitoFeet_Compose(const MtxF* parent, const Vec3f* position, const Vec3s joints[3], s32 side) {
    RitoFeetPose pose;
    Vec3f offset;
    s32 segment;
    memcpy(pose.joints, joints, sizeof(pose.joints));
    Matrix_Push();
    Matrix_Put((MtxF*)parent);
    Matrix_TranslateRotateZYX((Vec3f*)position, &pose.joints[0]);
    Matrix_Get(&pose.segments[0]);
    for (segment = 1; segment < 3; segment++) {
        offset.x = sRitoFeetBoneOffsets[side][segment - 1].x;
        offset.y = sRitoFeetBoneOffsets[side][segment - 1].y;
        offset.z = sRitoFeetBoneOffsets[side][segment - 1].z;
        Matrix_TranslateRotateZYX(&offset, &pose.joints[segment]);
        Matrix_Get(&pose.segments[segment]);
    }
    Matrix_Pop();
    return pose;
}

static f64 RitoFeet_Contact(const RitoFeetPose* pose, s32 side) {
    f64 lowest = DBL_MAX;
    Vec3f point;
    s32 vertex;
    for (vertex = 0; vertex < 104; vertex++) {
        Matrix_MultVec3fExt((Vec3f*)&sRitoFeetSupport[side][vertex], &point, (MtxF*)&pose->segments[2]);
        if (!isfinite(point.y)) {
            return NAN;
        }
        lowest = fmin(lowest, point.y);
    }
    return lowest;
}

/* The six seam vertices follow the shin, not the foot. Validate the complete
 * rendered foot after solving the foot-owned contact hull. */
static f64 RitoFeet_RenderedContact(const RitoFeetPose* pose, s32 side) {
    f64 lowest = RitoFeet_Contact(pose, side);
    Vec3f point;
    s32 vertex;
    if (!isfinite(lowest)) {
        return NAN;
    }
    for (vertex = 0; vertex < 6; vertex++) {
        Matrix_MultVec3fExt((Vec3f*)&sRitoFeetShinSeam[side][vertex], &point, (MtxF*)&pose->segments[1]);
        if (!isfinite(point.y)) {
            return NAN;
        }
        lowest = fmin(lowest, point.y);
    }
    return lowest;
}

/* This function returns a private candidate, never a partially updated live pose. */
static u8 RitoFeet_Solve(const MtxF* parent, const Vec3f* position, const RitoFeetPose* initial, s32 side,
                         f64 floorBound, RitoFeetPose* result) {
    /* atan ratio table <1/1024 rad; sine table drops4 binang bits. Include an
     * extraction binang and two signed16 amplitude units. Three Euler factors
     * per local rotation yield eta; three local joints bound the foot frame. */
    const f64 gamma = 1.0 / 1024.0 + 17.0 * (2.0 * (f64)M_PIf / 65536.0) + 2.0 / 32767.0;
    const f64 eta = 3.0 * gamma;
    RitoFeetMatrix parentLinear = RitoFeet_Linear(parent), parentInverse;
    RitoFeetMatrix thighLinear = RitoFeet_Linear(&initial->segments[0]);
    RitoFeetMatrix shinLinear = RitoFeet_Linear(&initial->segments[1]);
    RitoFeetMatrix footLinear = RitoFeet_Linear(&initial->segments[2]);
    RitoFeetMatrix axisArc, upperArc, lowerArc, desiredThigh, desiredShin, thighInverse, shinInverse, local;
    RitoFeetVector hip, knee, ankle, upper, lower, oldAxis, pole, target, axis, targetKnee, actualKnee;
    f64 scale = 0.0, deformation = 0.0, contact, length1, length2, oldDistance, radius = 0.0;
    f64 roundoff, minimumLift, margin, distance, poleLength, along, heightSquared, kneeSide, candidateContact;
    Vec3s rotations[3];
    RitoFeetPose candidate;
    s32 column, other, vertex;

    contact = RitoFeet_Contact(initial, side);
    if (!isfinite(contact) || !isfinite(floorBound) || contact >= floorBound) {
        return false;
    }
    for (column = 0; column < 3; column++) {
        scale += RitoFeet_Length(RitoFeet_Column(&parentLinear, column)) / 3.0;
    }
    if (!isfinite(scale) || scale <= FLT_EPSILON || RitoFeet_Determinant(&parentLinear) <= 0.0 ||
        !RitoFeet_Inverse(&parentLinear, &parentInverse)) {
        return false;
    }
    for (column = 0; column < 3; column++) {
        f64 norm = RitoFeet_Length(RitoFeet_Column(&parentLinear, column));
        deformation = fmax(deformation, fabs(norm / scale - 1.0));
        for (other = 0; other < column; other++) {
            deformation =
                fmax(deformation,
                     fabs(RitoFeet_Dot(RitoFeet_Column(&parentLinear, column), RitoFeet_Column(&parentLinear, other)) /
                          (norm * RitoFeet_Length(RitoFeet_Column(&parentLinear, other)))));
        }
    }
    /* Permit only the native ancestor quantization budget, not arbitrary shear. */
    if (!isfinite(deformation) || deformation > 12.0 * gamma) {
        return false;
    }
    hip = RitoFeet_Origin(&initial->segments[0]);
    knee = RitoFeet_Origin(&initial->segments[1]);
    ankle = RitoFeet_Origin(&initial->segments[2]);
    upper = RitoFeet_Subtract(knee, hip);
    lower = RitoFeet_Subtract(ankle, knee);
    length1 = RitoFeet_Length(upper);
    length2 = RitoFeet_Length(lower);
    oldAxis = RitoFeet_Subtract(ankle, hip);
    oldDistance = RitoFeet_Length(oldAxis);
    if (length1 <= FLT_EPSILON || length2 <= FLT_EPSILON || oldDistance <= FLT_EPSILON) {
        return false;
    }
    for (vertex = 0; vertex < 104; vertex++) {
        RitoFeetVector point = { sRitoFeetSupport[side][vertex].x, sRitoFeetSupport[side][vertex].y,
                                 sRitoFeetSupport[side][vertex].z };
        radius = fmax(radius, RitoFeet_Length(RitoFeet_Transform(&footLinear, point)));
    }
    roundoff = 64.0 * FLT_EPSILON * (RitoFeet_Length(hip) + length1 + length2 + radius);
    minimumLift = floorBound - contact;
    /* Upper/shin/foot rotational errors have these exact geometric lever arms. */
    margin = (length1 + 2.0 * length2 + 3.0 * radius) * eta + (length1 + length2 + radius) * deformation + roundoff;
    target = ankle;
    target.y += minimumLift + margin;
    axis = RitoFeet_Subtract(target, hip);
    distance = RitoFeet_Length(axis);
    if (!isfinite(distance) || distance <= fabs(length1 - length2) + roundoff ||
        distance >= length1 + length2 - roundoff) {
        return false;
    }
    oldAxis = RitoFeet_Scale(oldAxis, 1.0 / oldDistance);
    pole = RitoFeet_Subtract(upper, RitoFeet_Scale(oldAxis, RitoFeet_Dot(upper, oldAxis)));
    poleLength = RitoFeet_Length(pole);
    if (!isfinite(poleLength) || poleLength <= roundoff || !RitoFeet_Arc(oldAxis, axis, &axisArc)) {
        return false;
    }
    pole = RitoFeet_Transform(&axisArc, RitoFeet_Scale(pole, 1.0 / poleLength));
    axis = RitoFeet_Scale(axis, 1.0 / distance);
    pole = RitoFeet_Subtract(pole, RitoFeet_Scale(axis, RitoFeet_Dot(pole, axis)));
    poleLength = RitoFeet_Length(pole);
    if (poleLength <= FLT_EPSILON) {
        return false;
    }
    pole = RitoFeet_Scale(pole, 1.0 / poleLength);
    along = (length1 * length1 - length2 * length2 + distance * distance) / (2.0 * distance);
    heightSquared = length1 * length1 - along * along;
    if (heightSquared <= roundoff * roundoff) {
        return false;
    }
    targetKnee =
        RitoFeet_Add(hip, RitoFeet_Add(RitoFeet_Scale(axis, along), RitoFeet_Scale(pole, sqrt(heightSquared))));
    if (!RitoFeet_Arc(upper, RitoFeet_Subtract(targetKnee, hip), &upperArc) ||
        !RitoFeet_Arc(lower, RitoFeet_Subtract(target, targetKnee), &lowerArc)) {
        return false;
    }
    desiredThigh = RitoFeet_Multiply(&upperArc, &thighLinear);
    desiredShin = RitoFeet_Multiply(&lowerArc, &shinLinear);
    if (!RitoFeet_Inverse(&desiredThigh, &thighInverse) || !RitoFeet_Inverse(&desiredShin, &shinInverse)) {
        return false;
    }
    local = RitoFeet_Multiply(&parentInverse, &desiredThigh);
    rotations[0] = RitoFeet_NativeEuler(&local);
    local = RitoFeet_Multiply(&thighInverse, &desiredShin);
    rotations[1] = RitoFeet_NativeEuler(&local);
    local = RitoFeet_Multiply(&shinInverse, &footLinear);
    rotations[2] = RitoFeet_NativeEuler(&local);
    candidate = RitoFeet_Compose(parent, position, rotations, side);

    candidateContact = RitoFeet_RenderedContact(&candidate, side);

    /* Native binang recomposition is the acceptance gate, not ideal float math. */
    if (RitoFeet_Length(RitoFeet_Subtract(RitoFeet_Origin(&candidate.segments[2]), target)) >
            (length1 + 2.0 * length2) * eta + (length1 + length2) * deformation + roundoff ||
        RitoFeet_Length(RitoFeet_Subtract(RitoFeet_Origin(&candidate.segments[1]), targetKnee)) >
            length1 * eta + (length1 + length2) * deformation + roundoff ||
        !isfinite(candidateContact) || candidateContact < floorBound) {
        return false;
    }
    for (column = 0; column < 3; column++) {
        RitoFeetMatrix recomposedFoot = RitoFeet_Linear(&candidate.segments[2]);
        RitoFeetVector before = RitoFeet_Column(&footLinear, column);
        RitoFeetVector after = RitoFeet_Column(&recomposedFoot, column);
        f64 cosine = RitoFeet_Dot(before, after) / (RitoFeet_Length(before) * RitoFeet_Length(after));
        if (!isfinite(cosine) || acos(fmin(1.0, fmax(-1.0, cosine))) > 9.0 * gamma + deformation) {
            return false;
        }
    }
    actualKnee = RitoFeet_Subtract(RitoFeet_Origin(&candidate.segments[1]), hip);
    kneeSide = RitoFeet_Dot(RitoFeet_Subtract(actualKnee, RitoFeet_Scale(axis, RitoFeet_Dot(actualKnee, axis))), pole);
    if (!isfinite(kneeSide) || kneeSide <= 0.0) {
        return false;
    }
    *result = candidate;
    return true;
}

static void RitoFeet_ApplyPostNative(const MtxF* parent, const Vec3f* position, Vec3s* rotation, SkelAnime* skelAnime,
                                     s32 thigh, s32 shin, s32 foot, f32 floorY) {
    s32 side;
    Vec3s joints[3];
    RitoFeetPose initial, corrected;
    if ((thigh == PLAYER_LIMB_RIGHT_THIGH) && (shin == PLAYER_LIMB_RIGHT_SHIN) && (foot == PLAYER_LIMB_RIGHT_FOOT)) {
        side = 0;
    } else if ((thigh == PLAYER_LIMB_LEFT_THIGH) && (shin == PLAYER_LIMB_LEFT_SHIN) &&
               (foot == PLAYER_LIMB_LEFT_FOOT)) {
        side = 1;
    } else {
        return;
    }
    joints[0] = *rotation;
    joints[1] = skelAnime->jointTable[shin];
    joints[2] = skelAnime->jointTable[foot];
    initial = RitoFeet_Compose(parent, position, joints, side);
    if (!RitoFeet_Solve(parent, position, &initial, side, floorY + sRitoFeetIdleFloorOffset[side], &corrected)) {
        return;
    }
    /* No live pose is touched until contact, orientation, reach and bend pass. */
    *rotation = corrected.joints[0];
    skelAnime->jointTable[thigh] = corrected.joints[0];
    skelAnime->jointTable[shin] = corrected.joints[1];
    skelAnime->jointTable[foot] = corrected.joints[2];
}

#endif
