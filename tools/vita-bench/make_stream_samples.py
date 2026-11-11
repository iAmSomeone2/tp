#!/usr/bin/env python3
"""Build the stream benchmark's samples from one stream on the user's disc.

Both are cut into packets in the same container, so the benchmark's loop and
timing are the same for each codec. Little-endian:

  u32 magic, u32 sample rate, u32 channels, u32 pre-skip, u32 packet count,
  u16 size[packet count], then the packets back to back.

* opus.bin ('NFOP'): ffmpeg decodes the .ast (AFC ADPCM, 44.1 kHz), resamples it
  to 48 kHz and encodes it with libopus, as bake.md plans for streams. The packets
  are taken out of the Ogg container and stored raw, as the bake would.
* afc.bin ('NFAF'): the .ast's own AFC data, without the BLCK headers, cut into
  packets of 60 frames (960 samples) per channel, channel after channel.

Both go into build/vita-bench/stream/, and the benchmark VPKs bundle them as
app0:sample.bin (configure with -DNFBENCH_SAMPLES=<that directory>). The script
then checks src/afc.h against ffmpeg's decode of the stream (needs a host c++).
"""

import argparse
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
AFC_FRAMES_PER_PACKET = 60


def ogg_packets(data: bytes):
    """Yield the packets of a single-stream Ogg file (RFC 3533)."""
    pos, packet = 0, b""
    while pos < len(data):
        assert data[pos:pos + 4] == b"OggS", f"bad Ogg page at {pos}"
        nseg = data[pos + 26]
        lacing = data[pos + 27:pos + 27 + nseg]
        pos += 27 + nseg
        for n in lacing:
            packet += data[pos:pos + n]
            pos += n
            if n < 255:
                yield packet
                packet = b""


def ast_channels(data: bytes):
    """The per-channel AFC data of an .ast (JASAramStream::Header, BlockHeader)."""
    assert data[:4] == b"STRM" and data[9] == 0, "not an AFC .ast"
    channels, rate = struct.unpack_from(">H", data, 0x0C)[0], struct.unpack_from(">i", data, 0x10)[0]
    out = [bytearray() for _ in range(channels)]
    pos = 0x40
    while pos < len(data):
        assert data[pos:pos + 4] == b"BLCK"
        size = struct.unpack_from(">I", data, pos + 4)[0]
        pos += 0x20
        for c in range(channels):
            out[c] += data[pos:pos + size]
            pos += size
    return rate, out


def write(path: Path, magic: bytes, rate: int, channels: int, pre_skip: int, packets: list) -> None:
    with path.open("wb") as f:
        f.write(struct.pack("<4s4I", magic, rate, channels, pre_skip, len(packets)))
        f.write(struct.pack(f"<{len(packets)}H", *map(len, packets)))
        f.writelines(packets)
    size = sum(map(len, packets))
    print(f"{path}: {len(packets)} packets, {channels} ch, {rate} Hz, {size / 1024:.0f} KiB")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--iso", type=Path, default=ROOT / "orig/GZ2E01/GZ2E01.iso")
    ap.add_argument("--dtk", type=Path, default=ROOT / "build/tools/dtk")
    ap.add_argument("--track", default="demo01_01.ast")
    ap.add_argument("--bitrate", default="96k", help="libopus VBR target")
    ap.add_argument("--out", type=Path, default=ROOT / "build/vita-bench/stream")
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        ast, ogg, ref = tmp / args.track, tmp / "sample.opus", tmp / "ref.pcm"
        subprocess.run([args.dtk, "vfs", "cp", f"{args.iso}:files/Audiores/Stream/{args.track}", ast],
                       check=True, stdout=subprocess.DEVNULL)
        ffmpeg = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-i", ast]
        subprocess.run(ffmpeg + ["-ar", "48000", "-c:a", "libopus", "-b:a", args.bitrate, "-frame_duration", "20",
                                 "-y", ogg], check=True)
        subprocess.run(ffmpeg + ["-f", "s16le", "-y", ref], check=True)

        packets = list(ogg_packets(ogg.read_bytes()))
        head, audio = packets[0], packets[2:]  # packets[1] is OpusTags
        assert head[:8] == b"OpusHead"
        write(args.out / "opus.bin", b"NFOP", 48000, head[9], struct.unpack_from("<H", head, 10)[0], audio)

        rate, chans = ast_channels(ast.read_bytes())
        step = AFC_FRAMES_PER_PACKET * 9
        afc = [b"".join(c[i:i + step] for c in chans) for i in range(0, len(chans[0]), step)]
        write(args.out / "afc.bin", b"NFAF", rate, len(chans), 0, afc)

        cxx = shutil.which("c++")
        if not cxx:
            print("no host c++; skipped the afc.h check")
            return
        exe = tmp / "afc_check"
        subprocess.run([cxx, "-std=c++20", "-O2", HERE / "src/afc_check.cpp", "-o", exe], check=True)
        subprocess.run([exe, args.out / "afc.bin", ref], check=True)


if __name__ == "__main__":
    main()
