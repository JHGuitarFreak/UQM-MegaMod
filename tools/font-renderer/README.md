# Font to PNG Converter

This Python script converts font glyphs into individual PNG images, named by their Unicode values.

## Requirements
- Python 3.x
- FontForge Python bindings (`fontforge`)
- Pillow library (`PIL`)

Install dependencies with:
`pip install fontforge pillow`

If you're using MSYS2/MinGW64:
`pacman -S mingw-w64-x86_64-fontforge mingw-w64-x86_64-python-pillow`

## Usage
1. Place your font files (TTF/OTF/WOFF) in the same directory as the script
2. Run the script: `python font2png.py`
3. Choose your font from the list provided
4. Enter the desired font size in point
5. If there are no errors your font will be exported in a matter of seconds

### Command Line Options
- `--ansi` Export only ANSI character set (0x20-0xFF) - Can not be used with `--ascii`
- `--ascii` Export only basic ASCII character set (0x20-0x7F) - Can not be used with `--ansi`
- `--invert` Export PNG as black on white (default is white on black)
- `--ascender #` Custom ascender value for fonts with cut-off tops - Must be taller than the font's ascender
- `--descender #` Custom descender value for fonts with cut-off bottoms - Must be shorter than the font's descender
- `--uqm` Option for UQM fonts that adjusts final widths

## Example Output
`python font2png.py --uqm --ascii --invert`

This will export ASCII characters (32-127) as black glyphs on white background using UQM width adjustments.

<img width="189" height="256" alt="Example Python output" src="https://github.com/user-attachments/assets/108f0845-b0bf-4ed2-815f-1ade3e9332a7" />
<img width="547" height="256" alt="Example image output" src="https://github.com/user-attachments/assets/434254c4-4510-44f2-80be-06d2b265b2a5" />

## Notes
The `Ascender`, `Descender`, and `ImgHeight` values shown in the output are for scenarios where the font is chopped off
either at the top or bottom and to be used in conjunction with the `--ascender #` and `--descender #` command line
options accordingly.  
Also some vertical adjustment will still be required for all exported glyphs in order to get the desired size absolutely perfect.