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
