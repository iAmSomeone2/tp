# Math library plan (`nf_math`)

> **Status: plan only.** Nothing described here exists yet. Measured with GCC 16.2 on the CMake build (GameCube USA), October 2026.

The engine's vector and matrix math was written for the Gekko's paired-single FPU. On a host compiler most of it is either missing (link errors) or, worse, compiles to an empty function. This page proposes one Nightfall module, `nf_math`, that supplies all of it: first as plain, correct C++, then vectorised for the Vita's NEON where profiling says it matters.

## The problem, in three parts

### 1. The SDK math library is not built

The CMake build omits the Dolphin SDK implementation (see [cmake.md](cmake.md)), and that includes `libs/dolphin/src/mtx/`. When `tp` is linked, 32 of its 366 undefined symbols are math. They have C linkage (`mtx.h` wraps them in `extern "C"`). These are the most-referenced, counting unresolved references in the link of `tp` (roughly one per call site per object file):

| Function | References | Function | References |
|---|---:|---|---:|
| `PSMTXMultVec` | 301 | `PSMTXConcat` | 148 |
| `PSVECSquareMag` | 236 | `PSVECScale` | 95 |
| `PSVECAdd` | 204 | `PSVECSubtract` | 94 |
| `PSMTXCopy` | 172 | `PSMTXMultVecSR` | 81 |
| `PSVECSquareDistance` | 155 | `PSVECDotProduct` | 69 |

The rest: `PSMTXIdentity`, `PSMTXInverse`, `PSMTXMultVecArray`, `PSMTXQuat`, `PSMTXRotAxisRad`, `PSMTXRotRad`, `PSMTXScale`, `PSMTXScaleApply`, `PSMTXTrans`, `PSMTXTransApply`, `PSVECCrossProduct`, `PSVECDistance`, `PSVECMag`, `PSVECNormalize`, and the C-only `C_MTXLightOrtho`, `C_MTXLightPerspective`, `C_MTXLookAt`, `C_MTXOrtho`, `C_MTXPerspective`, `C_MTXRotAxisRad`, `C_VECHalfAngle`, `C_VECReflect`. `mtx.h` maps the `MTX*`/`VEC*` macros to `C_*` in `DEBUG` builds and to `PS*` otherwise, so the CMake build (no `DEBUG`) references the `PS*` names. [sdk-surface.md](sdk-surface.md#math-library-mtx-vec-quat) counts 1,794 calls to 62 functions in 419 files.

The SDK sources include portable C reference versions of all of these (`C_MTXConcat` and so on, in `libs/dolphin/src/mtx/*.c`). They are what `DEBUG` builds use, which makes them the natural correctness baseline.

### 2. Engine functions that exist only as assembly

`J3DPSMtxArrayConcat` (`libs/JSystem/src/J3DGraphBase/J3DTransform.cpp:441`) is a whole-function `asm` inside `#ifdef __MWERKS__`, so on a host it is simply undefined. It computes `mAB[i] = mA × mB[i]` for `i < count`: `mA` is fixed (the view matrix), while `mB` and `mAB` advance one `Mtx` per iteration. Its callers are in `J3DMtxBuffer::calcDrawMtx` (envelope/skinning matrices).

### 3. Functions that compile to nothing (silent)

Many JSystem functions put the paired-single code *inside* the function body, under `#ifdef __MWERKS__` with no `#else`. On a host they compile, link, and do nothing; the non-`void` ones return garbage. They do not show up as link errors, and the CMake build compiles legacy code with `-w` (`TP_ENABLE_WARNINGS=OFF`), which also hides `-Wreturn-type`.

A scan for `#ifdef __MWERKS__` blocks that contain paired-single instructions and have no `#else` finds these (each should be checked by hand; for example `J3DTransformInfo::operator=` in `J3DTransform.h` is fine, because the whole operator is inside the `#ifdef` and the host gets the implicit one):

| File | Functions with an empty host body |
|---|---|
| `J3DGraphBase/J3DTransform.cpp` | `J3DPSCalcInverseTranspose`, `J3DScaleNrmMtx`, `J3DScaleNrmMtx33`, `J3DMtxProjConcat` |
| `J3DGraphBase/J3DTransform.h` | `J3DPSMtx33Copy`, `J3DPSMtx33CopyFrom34`, `J3DPSMulMtxVec` (all four overloads) |
| `J3DGraphBase/J3DStruct.h` | `J3DTextureSRTInfo::operator=` (an assignment that copies nothing) |
| `J3DGraphBase/J3DStruct.cpp` | `J3DTexMtxInfo::setEffectMtx` |
| `J3DGraphBase/J3DDrawBuffer.h` | `J3DCalcZValue` (returns garbage, used for draw sorting) |
| `J3DGraphAnimator/J3DAnimation.cpp` | `J3DHermiteInterpolation` (returns garbage) |
| `J2DGraph/J2DAnimation.h` | `J2DHermiteInterpolation` (returns garbage) |
| `JMath/JMath.h` | `JMAHermiteInterpolation`; `gekko_ps_copy3/6/12/16`; `JMathInlineVEC::C_VECAdd`, `C_VECSubtract`, `C_VECSquareMag`, `C_VECDotProduct` |
| `JMath/JMath.cpp` | `JMAQuatLerp`, `JMAFastVECNormalize`, `JMAVECScaleAdd`, `JMAMTXApplyScale` |
| `JGeometry.h` | `setTVec3f`, `mulInternal`, `negateInternal` |

Paths are under `libs/JSystem/`. Animation curves, draw-order sorting, normal matrices, texture matrices and vector helpers all depend on these, so **this list is a correctness blocker for phase 6 of the [roadmap](roadmap.md), not an optimisation**. The scan is regex-based; re-run it (or a better one) before relying on the list being complete.

## Design

### Module

A Nightfall module per the repo's conventions: target `nf_math` (alias `nf::math`), headers in `include/nightfall/math/`, sources in `src/nightfall/math/`, tests in `tests/nf_math/`.

* **SDK entry points.** `nf_math` defines the SDK's `PSMTX*`/`PSVEC*`/`PSQUAT*` and the `C_*` functions with C linkage, with the exact signatures in `libs/dolphin/include/dolphin/mtx.h`. Game code keeps calling `MTXConcat` and friends unchanged. Make the `C_*` names aliases of the same kernels, so both spellings behave identically.
* **JSystem entry points.** Each of the asm-only or empty-bodied functions above gets an `#else` branch that calls an `nf::math` kernel, following the precedent of `J3DCalcBBoardMtx` (`J3DTransform.cpp:58`, `#if DEBUG || !defined(__MWERKS__)`). The Metrowerks matching build keeps compiling the original asm, so it is unaffected. Where an inline header function is on the hot path, the kernel should be `inline` in an `nf/math` header, not an out-of-line call.
* **Kernels** live in `namespace nf::math` and operate on the SDK types (`Mtx` = `f32[3][4]`, `Mtx44`, `Mtx33`, `Vec`, `Quaternion`), so there are no conversions at the boundary.

### Three implementation tiers

1. **Scalar C++ (first, for everything).** Port the SDK's own `C_*` reference code from `libs/dolphin/src/mtx/`, which is already portable C. Write kernels in row form (`AB.row[i] = A[i][0]·B.row0 + A[i][1]·B.row1 + A[i][2]·B.row2 + A[i][3]·(0,0,0,1)`), because that is the shape compilers auto-vectorise and that tier 2 implements directly. This tier unblocks the link and fixes every silent no-op.
2. **Portable 128-bit vectors (for measured hot spots).** A `Mtx` row is four floats, exactly one 128-bit vector. GCC/Clang vector extensions (`typedef float v4f __attribute__((vector_size(16)))`) compile the same source to NEON on the Vita and SSE on x86-64, and the VitaSDK's GCC supports them. Check the generated assembly of tier 1 first; GCC and Clang often vectorise the row-form code by themselves at `-O2`/`-O3`.
3. **NEON intrinsics (`arm_neon.h`) only where tier 2's code is measurably poor.** Keep the tier-1 version beside it as the reference and as the non-ARM path.

AVX is not a goal. 256-bit vectors do not help a single 3×4 matrix, and the desktop build is a development host, not a shipping target.

### Rules every kernel follows

* **Unaligned access.** `Mtx` and `Vec` are not guaranteed to be 16-byte aligned, and raising their alignment would change struct layouts the decompiled code depends on. Use unaligned loads (`vld1q_f32` has no alignment requirement; `_mm_loadu_ps` on x86).
* **Aliasing.** Callers routinely pass the same matrix as input and output (`MTXConcat(a, b, a)`). Every kernel must load all of its inputs before storing, or compute into a temporary.
* **No implicit fused multiply-add.** `tp::config` builds with `-ffp-contract=off`. Kernels should keep separate multiply and add unless that policy is changed deliberately. This is also what the Vita's Cortex-A9 does natively: its NEON has `vmla` (multiply then add, two roundings) but not `vfma` (fused), which needs NEONv2.
* **Overreads.** `Vec` is 12 bytes. A 4-wide load of a `Vec` reads 4 bytes past it, which can cross into an unmapped page at the end of a buffer. Load `Vec` as 2+1 floats, or prove the overread is safe.

## Testing

GoogleTest suites in `tests/nf_math/` compare every kernel against the SDK's C reference implementation:

* random inputs, including the identity, singular and near-singular matrices (for `PSMTXInverse`), and zero-length vectors (for `PSVECNormalize`);
* the aliasing cases (output equal to each input in turn);
* the compare uses a small ULP tolerance, because the reference and the vectorised kernels may round differently;
* for the formerly empty JSystem functions, a test that would have failed against the empty body (the assignment copies, the interpolation returns the right value).

A small benchmark target (not part of `ctest`) can time `PSMTXConcat`, `PSMTXMultVec` and `J3DPSMtxArrayConcat` per tier. Only on-device numbers from the Vita should decide whether tier 2 or 3 is worth it.

## Order of work

1. **Find all the silent no-ops.** Re-run the scan, and build the JSystem libraries once with `TP_ENABLE_WARNINGS=ON` (or just `-Wreturn-type`) to catch the non-`void` cases.
2. **Tier 1 for everything**: the SDK entry points, `J3DPSMtxArrayConcat`, and the empty-bodied functions, with tests. This removes 33 undefined symbols from the link of `tp` and makes animation, sorting and normals correct.
3. **Profile on the Vita** once a stage renders (roadmap phase 6).
4. **Tiers 2 and 3** for whatever the profile shows. `PSMTXConcat`, `PSMTXMultVec` and `J3DPSMtxArrayConcat` are the likely candidates.

## Open decisions

* **Skinning on the GPU?** `J3DMtxBuffer::calcDrawMtx` produces envelope skinning matrices. If the GX → GXM work ([graphics.md](graphics.md)) moves skinning into a vertex shader, the CPU-side matrix arrays may shrink or disappear, so settle this before optimising `J3DPSMtxArrayConcat`.
* **FMA policy.** Keep `-ffp-contract=off` everywhere (reproducible, matches the Cortex-A9), or allow fused operations in rendering-only kernels on targets that have them.
* **Where the SDK entry points live.** In `nf_math` (proposed), or in `nf_platform` with `nf_math` holding only the kernels.
