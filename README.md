# vdfs4

Samsung's VDFS4 filesystem — the kernel driver and the `mkfs.vdfs` userspace
tools — patched to build and run on a modern Linux kernel.

The original driver targets Linux 5.4. This repo fixes the build against a
much newer kernel (folio-based page cache, `mnt_idmap`, `fs_context`, modern
block layer, etc.) and fixes a few real bugs found by mounting, reading, and
writing actual firmware images:

- A deadlock and a data-corruption bug in the compressed-file read path.
- A NULL-pointer crash on writeback (`dirty_folio` was never wired up for
  this kernel).

Full history of every change, with rationale, is in `git log`.

## Requirements

```sh
sudo apt-get install build-essential linux-headers-$(uname -r) perl patch
```

`linux-headers-$(uname -r)` must match the running kernel exactly. zlib,
zstd, lzo, and OpenSSL are vendored and built from source inside
`vdfs-tools`, so no `-dev` packages are needed for those.

## Build

**Kernel module:**

```sh
cd linux/fs/vdfs4
make
sudo insmod vdfs4.ko
sudo mount -t vdfs -o dncs /dev/<device-or-loop> <mountpoint>
```

`dncs` (do-not-check-signature) is required unless the volume is signed with
Samsung's private key — none of ours are.

**Userspace tools (`mkfs.vdfs`):**

```sh
cd vdfs-tools
make mkfs
truncate -s 256M image.vdfs4
./mkfs.vdfs --root-dir <some-dir> image.vdfs4
```

## Known limitations

- `-c <algo>` (mkfs compression) is broken for non-`--read-only` images —
  the on-disk file descriptor comes back zeroed on read. Looks like a gap in
  Samsung's own tooling rather than anything in the kernel driver: every real
  firmware image we found pairs compression exclusively with `--read-only`.
- Authenticated/signed volumes need Samsung's actual private key; testing
  here used `-o dncs` to skip signature checks on unsigned synthetic images.

## License

GPL-2.0-or-later, inherited from Samsung's original source (see `LICENSE`).
