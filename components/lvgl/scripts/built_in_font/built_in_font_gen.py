#!/usr/bin/env python3

import argparse
from argparse import RawTextHelpFormatter
import os
import sys

parser = argparse.ArgumentParser(description="""Create fonts for LVGL including the built-in symbols. lv_font_conv needs to be installed. See https://github.com/lvgl/lv_font_conv
Example: python built_in_font_gen.py --size 16 -o lv_font_roboto_16.c --bpp 4 -r 0x20-0x7F""", formatter_class=RawTextHelpFormatter)
parser.add_argument('-s', '--size',
					type=int,
					metavar = 'px',
					nargs='?',
					help='Size of the font in px')
parser.add_argument('--bpp',
					type=int,
					metavar = '1,2,4',
					nargs='?',
					help='Bit per pixel')
parser.add_argument('-r', '--range',
					nargs='+',
					metavar = 'start-end',
					default=['0x20-0x7F,0xB0,0x2022,0xB5'],
					help='Ranges and/or characters to include. Default is 0x20-7F (ASCII). E.g. -r 0x20-0x7F, 0x200, 324')
parser.add_argument('--symbols',
					nargs='+',
					metavar = 'sym',
					default=[''],
					help=u'Symbols to include. E.g. -s ÁÉŐ'.encode('utf-8'))
parser.add_argument('--font',
					metavar = 'file',
					nargs='?',
					default='Montserrat-Medium.ttf',
					help='A TTF or WOFF file')
parser.add_argument('-o', '--output',
					nargs='?',
					metavar='file',
					help='Output file name. E.g. my_font_20.c')
parser.add_argument('--compressed', action='store_true',
					help='Compress the bitmaps')
parser.add_argument('--subpx', action='store_true',
					help='3 times wider letters for sub pixel rendering')

args = parser.parse_args()

if args.compressed == False:
	compr = "--no-compress --no-prefilter"
else:
	compr = ""

if len(args.symbols[0]) != 0:
	args.symbols[0] = "--symbols " +  args.symbols[0]

subpx = ""
if args.subpx: subpx = "--lcd"

#Built in symbols
#syms = "61441,61448,61451,61452,61452,61453,61457,61459,61461,61465,61468,61473,61478,61479,61480,61502,61507,61512,61515,61516,61517,61521,61522,61523,61524,61543,61544,61550,61552,61553,61556,61559,61560,61561,61563,61587,61589,61636,61637,61639,61641,61664,61671,61674,61683,61724,61732,61787,61931,62016,62017,62018,62019,62020,62087,62099,62212,62189,62810,63426,63650"
#syms = "61448,61918,61932,61541,62172,62151,62152,62153,62154,62155, 61441,61448,61451,61452,61452,61453,61457,61459,61461,61465,61468,61473,61478,61479,61480,61502,61507,61512,61515,61516,61517,61521,61522,61523,61524,61543,61544,61550,61552,61553,61556,61559,61560,61561,61563,61587,61589,61636,61637,61639,61641,61664,61671,61674,61683,61724,61732,61787,61931,62016,62017,62018,62019,62020,62087,62099,62212,62189,62810,63426,63650"
syms = "63105,63499,63500,61450,58562,58552,61953,61953,62171,63550,62704,62707,61918,61932,61541,62172,62151,62152,62153,62154,62155,61441,61448,61451,61452,61452,61453,61457,61459,61461,61465,61468,61473,61478,61479,61480,61502,61507,61512,61515,61516,61517,61521,61522,61523,61524,61543,61544,61550,61552,61553,61556,61559,61560,61561,61563,61587,61589,61636,61637,61639,61641,61664,61671,61674,61683,61724,61732,61787,61931,62016,62017,62018,62019,62020,62087,62099,62212,62189,62810,63426,63650"
# python3.11 chr(63550).encode('utf-8')

#if !defined LV_SYMBOL_SQUAREWAVE
#define LV_SYMBOL_SQUAREWAVE    "\xef\xa0\xbe" // 63550 
#endif
#if !defined LV_SYMBOL_BEAT
#define LV_SYMBOL_BEAT          "\xef\x93\xb0" // 62704
#endif
#if !defined LV_SYMBOL_ZERO
#define LV_SYMBOL_ZERO          "\xef\x93\xb3" // 62707
#endif
#if !defined LV_SYMBOL_SLIDER
#define LV_SYMBOL_SLIDER        "\xef\x87\x9e" // 61918
#endif
#if !defined LV_SYMBOL_CALCULATOR
#define LV_SYMBOL_CALCULATOR    "\xef\x87\xac" // 61932
#endif
#if !defined LV_SYMBOL_EXPAND
#define LV_SYMBOL_EXPAND        "\xef\x81\xa5" // 61541
#endif
#if !defined LV_SYMBOL_ICE
#define LV_SYMBOL_ICE           "\xef\x8b\x9c" // 62172 
#endif
#if !defined LV_SYMBOL_THERMO5
#define LV_SYMBOL_THERMO5       "\xef\x8b\x87" // 62151
#endif
#if !defined LV_SYMBOL_THERMO4
#define LV_SYMBOL_THERMO4       "\xef\x8b\x88" // 62152
#endif
#if !defined LV_SYMBOL_THERMO3
#define LV_SYMBOL_THERMO3       "\xef\x8b\x89" // 62153
#endif
#if !defined LV_SYMBOL_THERMO2
#define LV_SYMBOL_THERMO2       "\xef\x8b\x8a" // 62154
#endif
#if !defined LV_SYMBOL_THERMO1
#define LV_SYMBOL_THERMO1       "\xef\x8b\x8b" // 62155
#endif
#if !defined LV_SYMBOL_CHIP
#define LV_SYMBOL_CHIP       "\xef\x8b\x9b" // 62171
#endif
#if !defined LV_SYMBOL_CHART
#define LV_SYMBOL_CHART       "\xef\x88\x81" // 61953
#endif
#if !defined LV_SYMBOL_LINEDOWN
#define LV_SYMBOL_LINEDOWN "\xee\x92\xb8" // 58552
#endif
#if !defined LV_SYMBOL_LINEUP
#define LV_SYMBOL_LINEUP "\xee\x93\x82" // 58562
#endif
#if !defined LV_SYMBOL_ARRAY
#define LV_SYMBOL_ARRAY  "\xef\x80\x8a" // 61450
#endif
#if !defined LV_SYMBOL_BULBOFF
#define LV_SYMBOL_BULBOFF "\xef\xa0\x8b" // 63499
#endif
#if !defined LV_SYMBOL_BULBON
#define LV_SYMBOL_BULBON  "\xef\xa0\x8c" // 63500
#endif

#Run the command (Add degree and bullet symbol)
cmd = "lv_font_conv {} {} --bpp {} --size {} --font {} -r {} {} --font FontAwesome5-Solid+Brands+Regular.woff -r {} --format lvgl -o {} --force-fast-kern-format".format(subpx, compr, args.bpp, args.size, args.font, args.range[0], args.symbols[0], syms, args.output)
os.system(cmd)
