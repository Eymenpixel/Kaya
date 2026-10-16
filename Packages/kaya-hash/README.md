# Kaya File Hash

Calculates SHA-256 by default. SHA-512 and BLAKE2b are also available.

Examples: `kaya-hash file.iso`, `kaya-hash --algorithm sha512 file.iso`.
Files are streamed in chunks and never modified.
