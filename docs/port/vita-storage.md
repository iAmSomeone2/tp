# Vita storage and decompression: measurements

Measured on 2026-11-11 with `tools/vita-bench/` on the developer's Vita (ARM 444 MHz, bus 222 MHz), homebrew installed with VitaShell. `ux0:` is an SD2Vita adapter, `uma0:` an official 8 GiB memory card, `app0:` the installed app (physically on `ux0:`, not PFS-encrypted). The corpus is 28 archives (11.52 MiB) with all members decompressed: the logo scene's ARAM set, the Ordon stage (`F_SP103`) and six common object archives. These numbers decide the pack layer in [bake.md](bake.md#pack-files-and-the-index) and the cache tier in [memory-design.md](memory-design.md#resident-set-and-cache-tier).

## File operations

Medians, in ms. Reads are of whole entries; random reads go through an already open handle.

| | `app0:` | `ux0:` (SD2Vita) | `uma0:` (memory card) |
|---|---|---|---|
| `sceIoGetstat` | 0.36–2.05 | 0.39 | 0.39 |
| open (read-only) | 0.66 | 0.68 | 0.68 |
| close | 0.16 | 0.13 | 0.13 |
| random 4 KiB read | 2.89 | 4.00 | 3.28 |
| random 64 KiB read | 5.92 | 8.14 | 6.64 |
| sequential read (64 KiB to 1 MiB chunks) | 10.2–10.4 MiB/s | 8.1–8.2 MiB/s | 9.3–9.6 MiB/s |
| 28 entries as loose files (open, read, close each) | 1,474 ms | 1,712 ms | 1,590 ms |
| 28 entries from one open pack (`sceIoPread`) | 1,106 ms | 1,397 ms | 1,220 ms |

Writes on `ux0:` (64 KiB files): create 3.5 ms, write 11 ms, close 3.7 ms; `sceIoSyncByFd` adds 1.7 ms.

**Findings:**

* **Opens are cheap:** under 1 ms including close. An earlier note of "about 100 ms per open" came from a misremembered measurement (WiFi wake-up in another project), not from storage.
* **Throughput is the limit:** 8–10 MiB/s sequential on every device, with chunk size making no difference. The official card and the SD2Vita are within 15% of each other, so the I/O stack limits this rather than the medium.
* **Loose files still cost extra:** about 11–13 ms per file over reading the same bytes from an open pack. Only 1–3 ms of that is getstat, open and close; the rest is first-read latency on a newly opened file. Over the 28 entries, the pack is 19–25% faster.
* **No OS read cache:** every second pass is as slow as the first. Anything read again comes from storage again unless the engine keeps it.
* **Per-request latency** is about 3–4 ms, so many small reads are expensive; whole-entry reads are the right granularity.

## Decompression

One core, best of 3, throughput of decompressed output. Every entry is compressed separately, as it would be in a pack. "Estimated load" is reading the stored bytes at the measured `app0:` throughput (10.6 MiB/s) and then decompressing.

| Codec | Ratio | Decompression | Estimated load of the corpus |
|---|---|---|---|
| none | 1.00 | (`memcpy` 216 MiB/s) | 1,086 ms |
| LZ4 | 1.53 | 58 MiB/s | 910 ms |
| LZ4HC level 9 | 1.70 | 62 MiB/s | 826 ms |
| LZ4HC level 12 | 1.70 | 62 MiB/s | 825 ms |
| zstd level 3 | 1.93 | 24 MiB/s | 1,049 ms |
| zstd level 9 | 1.96 | 24 MiB/s | 1,034 ms |

**Findings:**

* **LZ4HC wins:** about 24% faster loads than uncompressed and 1.7× the capacity per cached MiB. Level 12 gains nothing over level 9.
* **zstd compresses better but decompresses too slowly** on this CPU to beat reading uncompressed data by much.
* **`memcpy` reaches only 216 MiB/s,** so this CPU is limited by memory bandwidth. Compiler flags for the codec are a possible later gain.
* Textures in the corpus are still GX formats. BC1 compresses about as poorly as CMPR, so the ratio should hold after the bake.

## Memory

Measured with `memcheck` (`tools/vita-bench/src/memcheck.cpp`), installed through VitaShell, with a 1 MiB newlib heap so the numbers are close to the app's whole budget:

| | Default | Extended memory mode (`ATTRIBUTE2=12`) |
|---|---|---|
| User (`*_USER_RW`) | 249 MiB | **358 MiB** |
| CDRAM | 112 MiB | 112 MiB |
| Physically contiguous | 26 MiB | 26 MiB |

* **The extended mode adds 109 MiB of user memory** and changes nothing else. The engine's Vita build sets it (`-d ATTRIBUTE2=12` in `VITA_MKSFOEX_FLAGS`).
* **It is read from the installed `param.sfo`.** The storage benchmark above had its `param.sfo` replaced over FTP and got the default budget (153 MiB free beside its 96 MiB heap, 249 MiB in total), so a changed SFO needs a reinstall.
* CDRAM (video memory) is separate from the user budget: textures, vertex buffers and render targets can live there without counting against the 358 MiB.

## Not measured

* **`savedata0:`:** saves are rare, and the game already makes the player choose when to save and wait for it to finish.
* **PFS-encrypted `app0:`** (a retail-style install), and `imc0:` (internal storage on later models).
