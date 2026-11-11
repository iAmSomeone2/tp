# vita-bench

A Vita homebrew app that measures what the pack-file and cache-tier designs depend on ([bake.md](../../docs/port/bake.md#pack-files-and-the-index), [memory-design.md](../../docs/port/memory-design.md#resident-set-and-cache-tier)):

* **Per-file cost:** `sceIoGetstat`, open, read and close timed separately, for one file per archive.
* **Pack pattern:** one open handle, `sceIoPread` per entry; sequential throughput at 64 KiB / 256 KiB / 1 MiB chunks; random 4 KiB and 64 KiB reads.
* **Writes:** create, write 64 KiB, optional `sceIoSyncByFd`, close.
* **Decompression on one core at 444 MHz:** LZ4, LZ4HC 9/12 and zstd 3/9 against `memcpy`, compressed per entry (entries are compressed one at a time, as in the pack), plus the estimated load time of the corpus for each.

Each test runs on every corpus location found: `app0:corpus` (bundled into the VPK), `ux0:`, `uma0:` and `imc0:` under `data/nfbench/corpus`. Loose-file and `pread` tests run twice to show the OS cache; the first sequential read of the pack runs before anything else touches it.

## Build and run

```sh
python3 tools/vita-bench/make_corpus.py          # build/vita-bench/corpus from orig/GZ2E01/GZ2E01.iso
export VITASDK=~/.local/vitasdk PATH=$VITASDK/bin:$PATH
cmake -S tools/vita-bench -B build/vita-bench/app -DNFBENCH_CORPUS=$PWD/build/vita-bench/corpus
cmake --build build/vita-bench/app               # build/vita-bench/app/nfbench.vpk
```

Leaving out `-DNFBENCH_CORPUS` gives a VPK without the `app0:` corpus. For the other partitions, copy `build/vita-bench/corpus/` to `<partition>:data/nfbench/corpus/`. LZ4 and zstd are fetched by CMake at configure time.

Install the VPK, reboot the Vita (so the first reads are not cached), and run it. Results are shown on screen and written to `ux0:data/nfbench/results.txt`; START exits.

The corpus is 28 archives (11.5 MiB): the logo scene's ARAM set, the Ordon stage and common object archives, with all members decompressed. It contains game data from your disc, so it and the VPK stay under `build/`.

## memcheck

Two tiny VPKs from the same build, `memcheck_default.vpk` (title `NFBN00002`, default memory budget) and `memcheck_extended.vpk` (`NFBN00003`, `ATTRIBUTE2=12` for the extended memory mode). Each writes its free user, CDRAM and physically contiguous memory to `ux0:data/nfbench/memory-<title>.txt` and exits at once, with newlib's heap kept to 1 MiB so the numbers are close to the full budget. Install both through VitaShell rather than copying files over FTP, so the system is sure to read each `param.sfo`. Run each once, and compare the two files.

## abiprobe

`abiprobe.vpk` (title `NFBN00007`) checks two things step 0 of the memory design depends on ([memory-design.md](../../docs/port/memory-design.md#order-of-work)):

* **`thread_local`**, which the current heap becomes. GCC implements it with emutls on this target (`__emutls_get_address` per access, backed by pthread keys), so it is checked on `std::thread` and on raw `sceKernelCreateThread` threads: each thread starts at the initial value and sees only its own. The cost per read is timed against a plain global.
* **Alignment faults**, to choose the heaps' minimum alignment: `ldr`/`ldrh`/`ldm`/`ldrd`/`vldr` and `vld1` without and with `:64`/`:128` hints at misaligned offsets, then three `-O3` functions over 8-aligned types (`uint64_t`, a `double`/`uint64_t` struct) called through 4-aligned pointers. GCC vectorises the `uint64_t` sum with `vld1.64 … [r0 :64]`. It also reports malloc's alignment.

A faulting load kills the app, so each case is logged to `ux0:data/nfbench/abiprobe.txt` before it runs. Launch the app again after a crash: the case that started without finishing is marked `FAULT` and the probe continues with the next one. It is finished when the file ends with `done` (the next launch starts over).

```sh
cmake --build build/vita-bench/app --target abiprobe.vpk-vpk   # build/vita-bench/app/abiprobe.vpk
```

## Stream benchmark

Measures what decoding the music streams as Opus would cost, against the disc's own AFC ADPCM ([bake.md](../../docs/port/bake.md#audio-streams)). `make_stream_samples.py` takes `demo01_01.ast` (1:59, stereo) from the disc and writes two samples in the same packet container:

* **Opus:** ffmpeg resamples to 48 kHz and encodes with libopus in 20 ms frames, VBR with a 96 kbit/s target, the bake's choice (this track comes out at 106 kbit/s, or 140 with `--bitrate 128k`; the app prints the actual rate). The raw packets are stored with a size table and no Ogg, as the bake would store them.
* **AFC:** the stream's own data without its block headers, cut into packets of 960 samples (21.8 ms at 44.1 kHz). The decoder (`src/afc.h`) is checked by the script against ffmpeg's decode of the whole track, and must match bit for bit.

Each app (`src/streambench.cpp`) plays its sample through SDL3's audio stream as soon as it starts, decoding on SDL's audio thread, and times every decode call. Then it decodes the whole track again flat out on the main thread, as a cross-check, and exits.

| VPK | Title | Decoder | Into SDL | Results |
|---|---|---|---|---|
| `opusbench_float.vpk` | `NFBN00004` | libopus, float | F32 | `opus-float.txt` |
| `opusbench_fixed.vpk` | `NFBN00005` | libopus, fixed point | S16 | `opus-fixed.txt` |
| `afcbench.vpk` | `NFBN00006` | AFC ADPCM, `afc.h` | S16 | `afc.txt` |

Both libopus builds presume NEON, since the Vita has no run-time CPU detection. The fixed build replaces one assembly-only routine with libopus's C version (`src/opus_fixed_xcorr.c`); the decoder uses that routine only for packet-loss concealment.

```sh
python3 tools/vita-bench/make_stream_samples.py  # build/vita-bench/stream/{opus,afc}.bin [--track X.ast] [--bitrate 128k]
cmake -S tools/vita-bench -B build/vita-bench/app -DNFBENCH_SAMPLES=$PWD/build/vita-bench/stream
cmake --build build/vita-bench/app               # opusbench_float.vpk, opusbench_fixed.vpk, afcbench.vpk
```

Listen for glitches while it plays. Results, at 444 MHz: the share of one core spent decoding, mean and worst time per packet, and the longest audio callback. They are shown on screen and written to `ux0:data/nfbench/<results file>`. SDL3 and libopus are fetched by CMake at configure time. The samples are disc audio, so they stay under `build/`.
