"""Rebuild original demo pixel art and a fixed-cell adaptation of the licensed UI font.
Optional development dependency: Pillow. Generated BMP files are committed.
"""
from pathlib import Path
from PIL import Image, ImageDraw
import shutil

root = Path(__file__).resolve().parents[1]
out = root / 'assets/rendering'
out.mkdir(parents=True, exist_ok=True)
key = (255, 0, 255)
sheet = Image.new('RGB', (128, 64), key)
d = ImageDraw.Draw(sheet)
for row, color in enumerate([(255, 117, 66), (62, 191, 244)]):
    for frame in range(4):
        x, y = frame * 32, row * 32
        bob = frame % 2
        d.rounded_rectangle((x+7,y+9+bob,x+24,y+26+bob), radius=5, fill=color)
        if row == 0:
            d.polygon([(x+9,y+13),(x+12,y+2+bob),(x+17,y+9),(x+22,y+5),(x+24,y+17)], fill=color)
            d.polygon([(x+13,y+10),(x+15,y+6),(x+18,y+13)], fill=(255,214,126))
        else:
            d.polygon([(x+8,y+13),(x+17,y+2+bob),(x+24,y+13)], fill=color)
            d.line([(x+12,y+13),(x+15,y+8)],fill=(175,239,255),width=2)
        d.rectangle((x+11,y+15+bob,x+13,y+18+bob),fill=(16,29,43))
        d.rectangle((x+19,y+15+bob,x+21,y+18+bob),fill=(16,29,43))
        d.rectangle((x+10,y+27,x+14,y+29+(frame%2)),fill=color)
        d.rectangle((x+19,y+27,x+23,y+30-(frame%2)),fill=color)
sheet.save(out/'characters.bmp')
tiles = Image.new('RGB',(64,32),(41,59,72))
d = ImageDraw.Draw(tiles)
for x in (0,32):
    d.rectangle((x,0,x+31,3),fill=(88,143,140) if x==0 else (110,172,166))
    d.rectangle((x+1,7,x+30,29),outline=(55,78,91))
    d.line((x+16,8,x+16,28),fill=(32,48,61))
tiles.save(out/'tiles.bmp')
source = Image.open(root/'apps/ui_prototype/assets/font.bmp').convert('L')
font = Image.new('RGB',(128,72),key)
for glyph in range(95):
    cell = source.crop((glyph%16*64,glyph//16*64,glyph%16*64+64,glyph//16*64+64))
    box=cell.getbbox()
    if box:
        # Keep a common baseline by scaling the full vertical extent.
        cell=cell.crop((max(0,box[0]-1),0,min(64,box[2]+1),64)).resize((7,12),Image.Resampling.LANCZOS)
        for y in range(12):
            for x in range(7):
                if cell.getpixel((x,y))>80: font.putpixel((glyph%16*8+x,glyph//16*12+y),(255,255,255))
font.save(out/'font.bmp')
shutil.copyfile(root/'apps/ui_prototype/assets/FONT-LICENSE.txt',out/'FONT-LICENSE.txt')
