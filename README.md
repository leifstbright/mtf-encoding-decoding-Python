# MTF Encode/Decode

A move-to-front (MTF) text encoder/decoder. Encodes a text file into a compact binary `.mtf` format, and decodes an `.mtf` file back into plain text.

## Requirements

- Python 3 (no external libraries needed)

## Usage

Both encoding and decoding are handled by the same script — it automatically picks the right mode based on the file extension of the input file.

**To encode** a text file into `.mtf`:
```bash
python mtf-encode-decode.py <file>.txt
```
This creates `<file>.mtf` in the same directory.

**To decode** an `.mtf` file back into text:
```bash
python mtf-encode-decode.py <file>.mtf
```
This creates `<file>.txt` in the same directory.
