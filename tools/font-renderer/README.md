# Windows Instructions

To use this script you have to install [FontForge](https://fontforge.org/en-US/downloads/) and [ImageMagick](https://imagemagick.org/script/download.php).
Then you add FontForge and ImageMagick's installation folder
to your [environment path](https://stackoverflow.com/a/44272417/2373275).

Once you have that completed you can `cd` into the script directory
with either the CMD prompt, Windows PowerShell, or any Bash Shell,
type in `fontforge-console` and press `Enter`. 
This will allow you to  use FontForge's built-in Python interpreter.
Then you can run the script by typing in `ffpython ttf2png.py` and 
hitting `Enter`. The script will prompt you to enter the filename of
a font (for now just use 'dpcomic.ttf') and then the font size.

If succesful it should start spitting out PNG files
from the test font 'DPComic' into a newly created "dpcomic" directory.

With the test completed you too can have the fun of exporting PNG files
from any given font.

## Troubleshooting
### Glyphs are at different heights
When each glyph image has a differing height it can make for an uneven font 
display within UQM. Luckily this is an easily resolved issue. 
First you find largest image height within the font directory.
Say, for example, it's 65. (Width does not matter in this context)
We click over to our CMD prompt, PowerShell, or Bash Shell and `cd` into
the font folder and type in this command and press enter:
`mogrify -extent 0x65 -gravity North -background black *.png`

What this does is expands the image height of every glyph `.png` within 
the current directory to 65 pixels, leaving width intact.

All that needs to be done for your particular use case is to take the 
actual largest height from your glyph `.png` files and plug it into the command.

DPComic font made by codeman38 - http://www.zone38.net/