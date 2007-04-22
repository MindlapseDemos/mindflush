
/* pnggccrd.c - mixed C/assembler version of utilities to read a PNG file
 *
 * For Intel x86 CPU (Pentium-MMX or later) and GNU C compiler.
 *
 *     See http:
 *     and http:
 *     for Intel's performance analysis of the MMX vs. non-MMX code.
 *
 * Last changed in libpng 1.2.9 April 14, 2006
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2006 Glenn Randers-Pehrson
 * Copyright (c) 1998, Intel Corporation
 *
 * Based on MSVC code contributed by Nirav Chhatrapati, Intel Corp., 1998.
 * Interface to libpng contributed by Gilles Vollant, 1999.
 * GNU C port by Greg Roelofs, 1999-2001.
 *
 * Lines 2350-4300 converted in place with intel2gas 1.3.1:
 *
 *   intel2gas -mdI pnggccrd.c.partially-msvc -o pnggccrd.c
 *
 * and then cleaned up by hand.  See http:
 *
 * NOTE:  A sufficiently recent version of GNU as (or as.exe under DOS/Windows)
 *        is required to assemble the newer MMX instructions such as movq.
 *        For djgpp, see
 *
 *           ftp:
 *
 *        (or a later version in the same directory).  For Linux, check your
 *        distribution's web site(s) or try these links:
 *
 *           http:
 *           http:
 *           ftp:
 *             binutils.tgz
 *
 *        For other platforms, see the main GNU site:
 *
 *           ftp:
 *
 *        Version 2.5.2l.15 is definitely too old...
 */

/*
 * TEMPORARY PORTING NOTES AND CHANGELOG (mostly by Greg Roelofs)
 * =====================================
 *
 * 19991006:
 *  - fixed sign error in post-MMX cleanup code (16- & 32-bit cases)
 *
 * 19991007:
 *  - additional optimizations (possible or definite):
 *     x [DONE] write MMX code for 64-bit case (pixel_bytes == 8) [not tested]
 *     - write MMX code for 48-bit case (pixel_bytes == 6)
 *     - figure out what's up with 24-bit case (pixel_bytes == 3):
 *        why subtract 8 from width_mmx in the pass 4/5 case?
 *        (only width_mmx case) (near line 1606)
 *     x [DONE] replace pixel_bytes within each block with the true
 *        constant value (or are compilers smart enough to do that?)
 *     - rewrite all MMX interlacing code so it's aligned with
 *        the *beginning* of the row buffer, not the end.  This
 *        would not only allow one to eliminate half of the memory
 *        writes for odd passes (that is, pass == odd), it may also
 *        eliminate some unaligned-data-access exceptions (assuming
 *        there's a penalty for not aligning 64-bit accesses on
 *        64-bit boundaries).  The only catch is that the "leftover"
 *        pixel(s) at the end of the row would have to be saved,
 *        but there are enough unused MMX registers in every case,
 *        so this is not a problem.  A further benefit is that the
 *        post-MMX cleanup code (C code) in at least some of the
 *        cases could be done within the assembler block.
 *  x [DONE] the "v3 v2 v1 v0 v7 v6 v5 v4" comments are confusing,
 *     inconsistent, and don't match the MMX Programmer's Reference
 *     Manual conventions anyway.  They should be changed to
 *     "b7 b6 b5 b4 b3 b2 b1 b0," where b0 indicates the byte that
 *     was lowest in memory (e.g., corresponding to a left pixel)
 *     and b7 is the byte that was highest (e.g., a right pixel).
 *
 * 19991016:
 *  - Brennan's Guide notwithstanding, gcc under Linux does *not*
 *     want globals prefixed by underscores when referencing them--
 *     i.e., if the variable is const4, then refer to it as const4,
 *     not _const4.  This seems to be a djgpp-specific requirement.
 *     Also, such variables apparently *must* be declared outside
 *     of functions; neither static nor automatic variables work if
 *     defined within the scope of a single function, but both
 *     static and truly global (multi-module) variables work fine.
 *
 * 19991023:
 *  - fixed png_combine_row() non-MMX replication bug (odd passes only?)
 *  - switched from string-concatenation-with-macros to cleaner method of
 *     renaming global variables for djgpp--i.e., always use prefixes in
 *     inlined assembler code (== strings) and conditionally rename the
 *     variables, not the other way around.  Hence _const4, _mask8_0, etc.
 *
 * 19991024:
 *  - fixed mmxsupport()/png_do_read_interlace() first-row bug
 *     This one was severely weird:  even though mmxsupport() doesn't touch
 *     ebx (where "row" pointer was stored), it nevertheless managed to zero
 *     the register (even in static/non-fPIC code--see below), which in turn
 *     caused png_do_read_interlace() to return prematurely on the first row of
 *     interlaced images (i.e., without expanding the interlaced pixels).
 *     Inspection of the generated assembly code didn't turn up any clues,
 *     although it did point at a minor optimization (i.e., get rid of
 *     mmx_supported_local variable and just use eax).  Possibly the CPUID
 *     instruction is more destructive than it looks?  (Not yet checked.)
 *  - "info gcc" was next to useless, so compared fPIC and non-fPIC assembly
 *     listings...  Apparently register spillage has to do with ebx, since
 *     it's used to index the global offset table.  Commenting it out of the
 *     input-reg lists in png_combine_row() eliminated compiler barfage, so
 *     ifdef'd with __PIC__ macro:  if defined, use a global for unmask
 *
 * 19991107:
 *  - verified CPUID clobberage:  12-char string constant ("GenuineIntel",
 *     "AuthenticAMD", etc.) placed in ebx:ecx:edx.  Still need to polish.
 *
 * 19991120:
 *  - made "diff" variable (now "_dif") global to simplify conversion of
 *     filtering routines (running out of regs, sigh).  "diff" is still used
 *     in interlacing routines, however.
 *  - fixed up both versions of mmxsupport() (ORIG_THAT_USED_TO_CLOBBER_EBX
 *     macro determines which is used); original not yet tested.
 *
 * 20000213:
 *  - when compiling with gcc, be sure to use  -fomit-frame-pointer
 *
 * 20000319:
 *  - fixed a register-name typo in png_do_read_interlace(), default (MMX) case,
 *     pass == 4 or 5, that caused visible corruption of interlaced images
 *
 * 20000623:
 *  - Various problems were reported with gcc 2.95.2 in the Cygwin environment,
 *     many of the form "forbidden register 0 (ax) was spilled for class AREG."
 *     This is explained at http:
 *     Chuck Wilson supplied a patch involving dummy output registers.  See
 *     http:
 *     for the original (anonymous) SourceForge bug report.
 *
 * 20000706:
 *  - Chuck Wilson passed along these remaining gcc 2.95.2 errors:
 *       pnggccrd.c: In function `png_combine_row':
 *       pnggccrd.c:525: more than 10 operands in `asm'
 *       pnggccrd.c:669: more than 10 operands in `asm'
 *       pnggccrd.c:828: more than 10 operands in `asm'
 *       pnggccrd.c:994: more than 10 operands in `asm'
 *       pnggccrd.c:1177: more than 10 operands in `asm'
 *     They are all the same problem and can be worked around by using the
 *     global _unmask variable unconditionally, not just in the -fPIC case.
 *     Reportedly earlier versions of gcc also have the problem with more than
 *     10 operands; they just don't report it.  Much strangeness ensues, etc.
 *
 * 20000729:
 *  - enabled png_read_filter_row_mmx_up() (shortest remaining unconverted
 *     MMX routine); began converting png_read_filter_row_mmx_sub()
 *  - to finish remaining sections:
 *     - clean up indentation and comments
 *     - preload local variables
 *     - add output and input regs (order of former determines numerical
 *        mapping of latter)
 *     - avoid all usage of ebx (including bx, bh, bl) register [20000823]
 *     - remove "$" from addressing of Shift and Mask variables [20000823]
 *
 * 20000731:
 *  - global union vars causing segfaults in png_read_filter_row_mmx_sub()?
 *
 * 20000822:
 *  - ARGH, stupid png_read_filter_row_mmx_sub() segfault only happens with
 *     shared-library (-fPIC) version!  Code works just fine as part of static
 *     library.  Damn damn damn damn damn, should have tested that sooner.
 *     ebx is getting clobbered again (explicitly this time); need to save it
 *     on stack or rewrite asm code to avoid using it altogether.  Blargh!
 *
 * 20000823:
 *  - first section was trickiest; all remaining sections have ebx -> edx now.
 *     (-fPIC works again.)  Also added missing underscores to various Shift*
 *     and *Mask* globals and got rid of leading "$" signs.
 *
 * 20000826:
 *  - added visual separators to help navigate microscopic printed copies
 *     (http:
 *     on png_read_filter_row_mmx_avg()
 *
 * 20000828:
 *  - finished png_read_filter_row_mmx_avg():  only Paeth left! (930 lines...)
 *     What the hell, did png_read_filter_row_mmx_paeth(), too.  Comments not
 *     cleaned up/shortened in either routine, but functionality is complete
 *     and seems to be working fine.
 *
 * 20000829:
 *  - ahhh, figured out last(?) bit of gcc/gas asm-fu:  if register is listed
 *     as an input reg (with dummy output variables, etc.), then it *cannot*
 *     also appear in the clobber list or gcc 2.95.2 will barf.  The solution
 *     is simple enough...
 *
 * 20000914:
 *  - bug in png_read_filter_row_mmx_avg():  16-bit grayscale not handled
 *     correctly (but 48-bit RGB just fine)
 *
 * 20000916:
 *  - fixed bug in png_read_filter_row_mmx_avg(), bpp == 2 case; three errors:
 *     - "_ShiftBpp.use = 24;"      should have been   "_ShiftBpp.use = 16;"
 *     - "_ShiftRem.use = 40;"      should have been   "_ShiftRem.use = 48;"
 *     - "psllq _ShiftRem, %%mm2"   should have been   "psrlq _ShiftRem, %%mm2"
 *
 * 20010101:
 *  - added new png_init_mmx_flags() function (here only because it needs to
 *     call mmxsupport(), which should probably become global png_mmxsupport());
 *     modified other MMX routines to run conditionally (png_ptr->asm_flags)
 *
 * 20010103:
 *  - renamed mmxsupport() to png_mmx_support(), with auto-set of mmx_supported,
 *     and made it public; moved png_init_mmx_flags() to png.c as internal func
 *
 * 20010104:
 *  - removed dependency on png_read_filter_row_c() (C code already duplicated
 *     within MMX version of png_read_filter_row()) so no longer necessary to
 *     compile it into pngrutil.o
 *
 * 20010310:
 *  - fixed buffer-overrun bug in png_combine_row() C code (non-MMX)
 *
 * 20020304:
 *  - eliminated incorrect use of width_mmx in pixel_bytes == 8 case
 *
 * 20040724:
 *   - more tinkering with clobber list at lines 4529 and 5033, to get
 *     it to compile on gcc-3.4.
 *
 * STILL TO DO:
 *     - test png_do_read_interlace() 64-bit case (pixel_bytes == 8)
 *     - write MMX code for 48-bit case (pixel_bytes == 6)
 *     - figure out what's up with 24-bit case (pixel_bytes == 3):
 *        why subtract 8 from width_mmx in the pass 4/5 case?
 *        (only width_mmx case) (near line 1606)
 *     - rewrite all MMX interlacing code so it's aligned with beginning
 *        of the row buffer, not the end (see 19991007 for details)
 *     x pick one version of mmxsupport() and get rid of the other
 *     - add error messages to any remaining bogus default cases
 *     - enable pixel_depth == 8 cases in png_read_filter_row()? (test speed)
 *     x add support for runtime enable/disable/query of various MMX routines
 */

#define PNG_INTERNAL
#include "png.h"

#if defined(PNG_USE_PNGGCCRD)

int PNGAPI png_mmx_support(void);

#ifdef PNG_USE_LOCAL_ARRAYS
static const int FARDATA png_pass_start[7] = {0, 4, 0, 2, 0, 1, 0};
static const int FARDATA png_pass_inc[7]   = {8, 8, 4, 4, 2, 2, 1};
static const int FARDATA png_pass_width[7] = {8, 4, 4, 2, 2, 1, 1};
#endif

#if defined(PNG_ASSEMBLER_CODE_SUPPORTED)
/* djgpp, Win32, Cygwin, and OS2 add their own underscores to global variables,
 * so define them without: */
#if defined(__DJGPP__) || defined(WIN32) || defined(__CYGWIN__) || \
    defined(__OS2__)
#  define _mmx_supported  mmx_supported
#  define _const4         const4
#  define _const6         const6
#  define _mask8_0        mask8_0
#  define _mask16_1       mask16_1
#  define _mask16_0       mask16_0
#  define _mask24_2       mask24_2
#  define _mask24_1       mask24_1
#  define _mask24_0       mask24_0
#  define _mask32_3       mask32_3
#  define _mask32_2       mask32_2
#  define _mask32_1       mask32_1
#  define _mask32_0       mask32_0
#  define _mask48_5       mask48_5
#  define _mask48_4       mask48_4
#  define _mask48_3       mask48_3
#  define _mask48_2       mask48_2
#  define _mask48_1       mask48_1
#  define _mask48_0       mask48_0
#  define _LBCarryMask    LBCarryMask
#  define _HBClearMask    HBClearMask
#  define _ActiveMask     ActiveMask
#  define _ActiveMask2    ActiveMask2
#  define _ActiveMaskEnd  ActiveMaskEnd
#  define _ShiftBpp       ShiftBpp
#  define _ShiftRem       ShiftRem
#ifdef PNG_THREAD_UNSAFE_OK
#  define _unmask         unmask
#  define _FullLength     FullLength
#  define _MMXLength      MMXLength
#  define _dif            dif
#  define _patemp         patemp
#  define _pbtemp         pbtemp
#  define _pctemp         pctemp
#endif
#endif


/* These constants are used in the inlined MMX assembly code.
   Ignore gcc's "At top level: defined but not used" warnings. */

/* GRR 20000706:  originally _unmask was needed only when compiling with -fPIC,
 *  since that case uses the %ebx register for indexing the Global Offset Table
 *  and there were no other registers available.  But gcc 2.95 and later emit
 *  "more than 10 operands in `asm'" errors when %ebx is used to preload unmask
 *  in the non-PIC case, so we'll just use the global unconditionally now.
 */
#ifdef PNG_THREAD_UNSAFE_OK
static int _unmask;
#endif

static unsigned long long _mask8_0  = 0x0102040810204080LL;

static unsigned long long _mask16_1 = 0x0101020204040808LL;
static unsigned long long _mask16_0 = 0x1010202040408080LL;

static unsigned long long _mask24_2 = 0x0101010202020404LL;
static unsigned long long _mask24_1 = 0x0408080810101020LL;
static unsigned long long _mask24_0 = 0x2020404040808080LL;

static unsigned long long _mask32_3 = 0x0101010102020202LL;
static unsigned long long _mask32_2 = 0x0404040408080808LL;
static unsigned long long _mask32_1 = 0x1010101020202020LL;
static unsigned long long _mask32_0 = 0x4040404080808080LL;

static unsigned long long _mask48_5 = 0x0101010101010202LL;
static unsigned long long _mask48_4 = 0x0202020204040404LL;
static unsigned long long _mask48_3 = 0x0404080808080808LL;
static unsigned long long _mask48_2 = 0x1010101010102020LL;
static unsigned long long _mask48_1 = 0x2020202040404040LL;
static unsigned long long _mask48_0 = 0x4040808080808080LL;

static unsigned long long _const4   = 0x0000000000FFFFFFLL;

static unsigned long long _const6   = 0x00000000000000FFLL;





#ifdef PNG_THREAD_UNSAFE_OK
static png_uint_32  _FullLength;
static png_uint_32  _MMXLength;
static int          _dif;
static int          _patemp; 
static int          _pbtemp;
static int          _pctemp;
#endif

void /* PRIVATE */
png_squelch_warnings(void)
{
#ifdef PNG_THREAD_UNSAFE_OK
   _dif = _dif;
   _patemp = _patemp;
   _pbtemp = _pbtemp;
   _pctemp = _pctemp;
   _MMXLength = _MMXLength;
#endif
   _const4  = _const4;
   _const6  = _const6;
   _mask8_0  = _mask8_0;
   _mask16_1 = _mask16_1;
   _mask16_0 = _mask16_0;
   _mask24_2 = _mask24_2;
   _mask24_1 = _mask24_1;
   _mask24_0 = _mask24_0;
   _mask32_3 = _mask32_3;
   _mask32_2 = _mask32_2;
   _mask32_1 = _mask32_1;
   _mask32_0 = _mask32_0;
   _mask48_5 = _mask48_5;
   _mask48_4 = _mask48_4;
   _mask48_3 = _mask48_3;
   _mask48_2 = _mask48_2;
   _mask48_1 = _mask48_1;
   _mask48_0 = _mask48_0;
}
#endif /* PNG_ASSEMBLER_CODE_SUPPORTED */


static int _mmx_supported = 2;

/*===========================================================================*/
/*                                                                           */
/*                       P N G _ C O M B I N E _ R O W                       */
/*                                                                           */
/*===========================================================================*/

#if defined(PNG_HAVE_ASSEMBLER_COMBINE_ROW)

#define BPP2  2
#define BPP3  3 /* bytes per pixel (a.k.a. pixel_bytes) */
#define BPP4  4
#define BPP6  6 /* (defined only to help avoid cut-and-paste errors) */
#define BPP8  8

/* Combines the row recently read in with the previous row.
   This routine takes care of alpha and transparency if requested.
   This routine also handles the two methods of progressive display
   of interlaced images, depending on the mask value.
   The mask value describes which pixels are to be combined with
   the row.  The pattern always repeats every 8 pixels, so just 8
   bits are needed.  A one indicates the pixel is to be combined; a
   zero indicates the pixel is to be skipped.  This is in addition
   to any alpha or transparency value associated with the pixel.
   If you want all pixels to be combined, pass 0xff (255) in mask. */

/* Use this routine for the x86 platform - it uses a faster MMX routine
   if the machine supports MMX. */

void /* PRIVATE */
png_combine_row(png_structp png_ptr, png_bytep row, int mask)
{
   png_debug(1, "in png_combine_row (pnggccrd.c)\n");

#if defined(PNG_ASSEMBLER_CODE_SUPPORTED)
   if (_mmx_supported == 2) {
#if !defined(PNG_1_0_X)
       /* this should have happened in png_init_mmx_flags() already */
       png_warning(png_ptr, "asm_flags may not have been initialized");
#endif
       png_mmx_support();
   }
#endif

   if (mask == 0xff)
   {
      png_debug(2,"mask == 0xff:  doing single png_memcpy()\n");
      png_memcpy(row, png_ptr->row_buf + 1,
       (png_size_t)PNG_ROWBYTES(png_ptr->row_info.pixel_depth,png_ptr->width));
   }
   else   /* (png_combine_row() is never called with mask == 0) */
   {
      switch (png_ptr->row_info.pixel_depth)
      {
         case 1:        /* png_ptr->row_info.pixel_depth */
         {
            png_bytep sp;
            png_bytep dp;
            int s_inc, s_start, s_end;
            int m;
            int shift;
            png_uint_32 i;

            sp = png_ptr->row_buf + 1;
            dp = row;
            m = 0x80;
#if defined(PNG_READ_PACKSWAP_SUPPORTED)
            if (png_ptr->transformations & PNG_PACKSWAP)
            {
                s_start = 0;
                s_end = 7;
                s_inc = 1;
            }
            else
#endif
            {
                s_start = 7;
                s_end = 0;
                s_inc = -1;
            }

            shift = s_start;

            for (i = 0; i < png_ptr->width; i++)
            {
               if (m & mask)
               {
                  int value;

                  value = (*sp >> shift) & 0x1;
                  *dp &= (png_byte)((0x7f7f >> (7 - shift)) & 0xff);
                  *dp |= (png_byte)(value << shift);
               }

               if (shift == s_end)
               {
                  shift = s_start;
                  sp++;
                  dp++;
               }
               else
                  shift += s_inc;

               if (m == 1)
                  m = 0x80;
               else
                  m >>= 1;
            }
            break;
         }

         case 2:        /* png_ptr->row_info.pixel_depth */
         {
            png_bytep sp;
            png_bytep dp;
            int s_start, s_end, s_inc;
            int m;
            int shift;
            png_uint_32 i;
            int value;

            sp = png_ptr->row_buf + 1;
            dp = row;
            m = 0x80;
#if defined(PNG_READ_PACKSWAP_SUPPORTED)
            if (png_ptr->transformations & PNG_PACKSWAP)
            {
               s_start = 0;
               s_end = 6;
               s_inc = 2;
            }
            else
#endif
            {
               s_start = 6;
               s_end = 0;
               s_inc = -2;
            }

            shift = s_start;

            for (i = 0; i < png_ptr->width; i++)
            {
               if (m & mask)
               {
                  value = (*sp >> shift) & 0x3;
                  *dp &= (png_byte)((0x3f3f >> (6 - shift)) & 0xff);
                  *dp |= (png_byte)(value << shift);
               }

               if (shift == s_end)
               {
                  shift = s_start;
                  sp++;
                  dp++;
               }
               else
                  shift += s_inc;
               if (m == 1)
                  m = 0x80;
               else
                  m >>= 1;
            }
            break;
         }

         case 4:        /* png_ptr->row_info.pixel_depth */
         {
            png_bytep sp;
            png_bytep dp;
            int s_start, s_end, s_inc;
            int m;
            int shift;
            png_uint_32 i;
            int value;

            sp = png_ptr->row_buf + 1;
            dp = row;
            m = 0x80;
#if defined(PNG_READ_PACKSWAP_SUPPORTED)
            if (png_ptr->transformations & PNG_PACKSWAP)
            {
               s_start = 0;
               s_end = 4;
               s_inc = 4;
            }
            else
#endif
            {
               s_start = 4;
               s_end = 0;
               s_inc = -4;
            }
            shift = s_start;

            for (i = 0; i < png_ptr->width; i++)
            {
               if (m & mask)
               {
                  value = (*sp >> shift) & 0xf;
                  *dp &= (png_byte)((0xf0f >> (4 - shift)) & 0xff);
                  *dp |= (png_byte)(value << shift);
               }

               if (shift == s_end)
               {
                  shift = s_start;
                  sp++;
                  dp++;
               }
               else
                  shift += s_inc;
               if (m == 1)
                  m = 0x80;
               else
                  m >>= 1;
            }
            break;
         }

         case 8:        /* png_ptr->row_info.pixel_depth */
         {
            png_bytep srcptr;
            png_bytep dstptr;

#if defined(PNG_ASSEMBLER_CODE_SUPPORTED) && defined(PNG_THREAD_UNSAFE_OK)
#if !defined(PNG_1_0_X)
            if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_COMBINE_ROW)
                /* && _mmx_supported */ )
#else
            if (_mmx_supported)
#endif
            {
               png_uint_32 len;
               int diff;
               int dummy_value_a;   
               int dummy_value_d;
               int dummy_value_c;
               int dummy_value_S;
               int dummy_value_D;
               _unmask = ~mask;            
               srcptr = png_ptr->row_buf + 1;
               dstptr = row;
               len  = png_ptr->width &~7;  
               diff = (int) (png_ptr->width & 7);  

               __asm__ __volatile__ (
                  "movd      _unmask, %%mm7  \n\t" 
                  "psubb     %%mm6, %%mm6    \n\t" 
                  "punpcklbw %%mm7, %%mm7    \n\t"
                  "punpcklwd %%mm7, %%mm7    \n\t"
                  "punpckldq %%mm7, %%mm7    \n\t" 

                  "movq      _mask8_0, %%mm0 \n\t"
                  "pand      %%mm7, %%mm0    \n\t" 
                  "pcmpeqb   %%mm6, %%mm0    \n\t" 





                  "cmpl      $0, %%ecx       \n\t" 
                  "je        mainloop8end    \n\t"

                "mainloop8:                  \n\t"
                  "movq      (%%esi), %%mm4  \n\t" 
                  "pand      %%mm0, %%mm4    \n\t"
                  "movq      %%mm0, %%mm6    \n\t"
                  "pandn     (%%edi), %%mm6  \n\t" 
                  "por       %%mm6, %%mm4    \n\t"
                  "movq      %%mm4, (%%edi)  \n\t"
                  "addl      $8, %%esi       \n\t" 
                  "addl      $8, %%edi       \n\t"
                  "subl      $8, %%ecx       \n\t" 
                  "ja        mainloop8       \n\t"

                "mainloop8end:               \n\t"

                  "movl      %%eax, %%ecx    \n\t"
                  "cmpl      $0, %%ecx       \n\t"
                  "jz        end8            \n\t"

                  "sall      $24, %%edx      \n\t" 

                "secondloop8:                \n\t"
                  "sall      %%edx           \n\t" 
                  "jnc       skip8           \n\t" 
                  "movb      (%%esi), %%al   \n\t"
                  "movb      %%al, (%%edi)   \n\t"

                "skip8:                      \n\t"
                  "incl      %%esi           \n\t"
                  "incl      %%edi           \n\t"
                  "decl      %%ecx           \n\t"
                  "jnz       secondloop8     \n\t"

                "end8:                       \n\t"
                  "EMMS                      \n\t"  

                  : "=a" (dummy_value_a),           
                    "=d" (dummy_value_d),
                    "=c" (dummy_value_c),
                    "=S" (dummy_value_S),
                    "=D" (dummy_value_D)

                  : "3" (srcptr),      
                    "4" (dstptr),      
                    "0" (diff),        

                    "2" (len),         
                    "1" (mask)         

#if 0  /* MMX regs (%mm0, etc.) not supported by gcc 2.7.2.3 or egcs 1.1 */
                  : "%mm0", "%mm4", "%mm6", "%mm7"  
#endif
               );
            }
            else /* mmx _not supported - Use modified C routine */
#endif /* PNG_ASSEMBLER_CODE_SUPPORTED */
            {
               register png_uint_32 i;
               png_uint_32 initial_val = png_pass_start[png_ptr->pass];
                 /* png.c:  png_pass_start[] = {0, 4, 0, 2, 0, 1, 0}; */
               register int stride = png_pass_inc[png_ptr->pass];
                 /* png.c:  png_pass_inc[] = {8, 8, 4, 4, 2, 2, 1}; */
               register int rep_bytes = png_pass_width[png_ptr->pass];
                 /* png.c:  png_pass_width[] = {8, 4, 4, 2, 2, 1, 1}; */
               png_uint_32 len = png_ptr->width &~7;  /* reduce to mult. of 8 */
               int diff = (int) (png_ptr->width & 7); /* amount lost */
               register png_uint_32 final_val = len;  /* GRR bugfix */

               srcptr = png_ptr->row_buf + 1 + initial_val;
               dstptr = row + initial_val;

               for (i = initial_val; i < final_val; i += stride)
               {
                  png_memcpy(dstptr, srcptr, rep_bytes);
                  srcptr += stride;
                  dstptr += stride;
               }
               if (diff)  /* number of leftover pixels:  3 for pngtest */
               {
                  final_val+=diff /* *BPP1 */ ;
                  for (; i < final_val; i += stride)
                  {
                     if (rep_bytes > (int)(final_val-i))
                        rep_bytes = (int)(final_val-i);
                     png_memcpy(dstptr, srcptr, rep_bytes);
                     srcptr += stride;
                     dstptr += stride;
                  }
               }

            } /* end of else (_mmx_supported) */

            break;
         }       /* end 8 bpp */

         case 16:       /* png_ptr->row_info.pixel_depth */
         {
            png_bytep srcptr;
            png_bytep dstptr;

#if defined(PNG_ASSEMBLER_CODE_SUPPORTED) && defined(PNG_THREAD_UNSAFE_OK)
#if !defined(PNG_1_0_X)
            if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_COMBINE_ROW)
                /* && _mmx_supported */ )
#else
            if (_mmx_supported)
#endif
            {
               png_uint_32 len;
               int diff;
               int dummy_value_a;   
               int dummy_value_d;
               int dummy_value_c;
               int dummy_value_S;
               int dummy_value_D;
               _unmask = ~mask;            
               srcptr = png_ptr->row_buf + 1;
               dstptr = row;
               len  = png_ptr->width &~7;  
               diff = (int) (png_ptr->width & 7); 

               __asm__ __volatile__ (
                  "movd      _unmask, %%mm7   \n\t" 
                  "psubb     %%mm6, %%mm6     \n\t" 
                  "punpcklbw %%mm7, %%mm7     \n\t"
                  "punpcklwd %%mm7, %%mm7     \n\t"
                  "punpckldq %%mm7, %%mm7     \n\t" 

                  "movq      _mask16_0, %%mm0 \n\t"
                  "movq      _mask16_1, %%mm1 \n\t"

                  "pand      %%mm7, %%mm0     \n\t"
                  "pand      %%mm7, %%mm1     \n\t"

                  "pcmpeqb   %%mm6, %%mm0     \n\t"
                  "pcmpeqb   %%mm6, %%mm1     \n\t"





                  "cmpl      $0, %%ecx        \n\t"
                  "jz        mainloop16end    \n\t"

                "mainloop16:                  \n\t"
                  "movq      (%%esi), %%mm4   \n\t"
                  "pand      %%mm0, %%mm4     \n\t"
                  "movq      %%mm0, %%mm6     \n\t"
                  "movq      (%%edi), %%mm7   \n\t"
                  "pandn     %%mm7, %%mm6     \n\t"
                  "por       %%mm6, %%mm4     \n\t"
                  "movq      %%mm4, (%%edi)   \n\t"

                  "movq      8(%%esi), %%mm5  \n\t"
                  "pand      %%mm1, %%mm5     \n\t"
                  "movq      %%mm1, %%mm7     \n\t"
                  "movq      8(%%edi), %%mm6  \n\t"
                  "pandn     %%mm6, %%mm7     \n\t"
                  "por       %%mm7, %%mm5     \n\t"
                  "movq      %%mm5, 8(%%edi)  \n\t"

                  "addl      $16, %%esi       \n\t" 
                  "addl      $16, %%edi       \n\t"
                  "subl      $8, %%ecx        \n\t" 
                  "ja        mainloop16       \n\t"

                "mainloop16end:               \n\t"

                  "movl      %%eax, %%ecx     \n\t"
                  "cmpl      $0, %%ecx        \n\t"
                  "jz        end16            \n\t"

                  "sall      $24, %%edx       \n\t" 

                "secondloop16:                \n\t"
                  "sall      %%edx            \n\t" 
                  "jnc       skip16           \n\t" 
                  "movw      (%%esi), %%ax    \n\t"
                  "movw      %%ax, (%%edi)    \n\t"

                "skip16:                      \n\t"
                  "addl      $2, %%esi        \n\t"
                  "addl      $2, %%edi        \n\t"
                  "decl      %%ecx            \n\t"
                  "jnz       secondloop16     \n\t"

                "end16:                       \n\t"
                  "EMMS                       \n\t" 

                  : "=a" (dummy_value_a),           
                    "=c" (dummy_value_c),
                    "=d" (dummy_value_d),
                    "=S" (dummy_value_S),
                    "=D" (dummy_value_D)

                  : "0" (diff),        

                    "1" (len),         
                    "2" (mask),        
                    "3" (srcptr),      
                    "4" (dstptr)       

#if 0  /* MMX regs (%mm0, etc.) not supported by gcc 2.7.2.3 or egcs 1.1 */
                  : "%mm0", "%mm1", "%mm4"          
                  , "%mm5", "%mm6", "%mm7"
#endif
               );
            }
            else /* mmx _not supported - Use modified C routine */
#endif /* PNG_ASSEMBLER_CODE_SUPPORTED */
            {
               register png_uint_32 i;
               png_uint_32 initial_val = BPP2 * png_pass_start[png_ptr->pass];
                 /* png.c:  png_pass_start[] = {0, 4, 0, 2, 0, 1, 0}; */
               register int stride = BPP2 * png_pass_inc[png_ptr->pass];
                 /* png.c:  png_pass_inc[] = {8, 8, 4, 4, 2, 2, 1}; */
               register int rep_bytes = BPP2 * png_pass_width[png_ptr->pass];
                 /* png.c:  png_pass_width[] = {8, 4, 4, 2, 2, 1, 1}; */
               png_uint_32 len = png_ptr->width &~7;  /* reduce to mult. of 8 */
               int diff = (int) (png_ptr->width & 7); /* amount lost */
               register png_uint_32 final_val = BPP2 * len;   /* GRR bugfix */

               srcptr = png_ptr->row_buf + 1 + initial_val;
               dstptr = row + initial_val;

               for (i = initial_val; i < final_val; i += stride)
               {
                  png_memcpy(dstptr, srcptr, rep_bytes);
                  srcptr += stride;
                  dstptr += stride;
               }
               if (diff)  /* number of leftover pixels:  3 for pngtest */
               {
                  final_val+=diff*BPP2;
                  for (; i < final_val; i += stride)
                  {
                     if (rep_bytes > (int)(final_val-i))
                        rep_bytes = (int)(final_val-i);
                     png_memcpy(dstptr, srcptr, rep_bytes);
                     srcptr += stride;
                     dstptr += stride;
                  }
               }
            } /* end of else (_mmx_supported) */

            break;
         }       /* end 16 bpp */

         case 24:       /* png_ptr->row_info.pixel_depth */
         {
            png_bytep srcptr;
            png_bytep dstptr;

#if defined(PNG_ASSEMBLER_CODE_SUPPORTED) && defined(PNG_THREAD_UNSAFE_OK)
#if !defined(PNG_1_0_X)
            if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_COMBINE_ROW)
                /* && _mmx_supported */ )
#else
            if (_mmx_supported)
#endif
            {
               png_uint_32 len;
               int diff;
               int dummy_value_a;   
               int dummy_value_d;
               int dummy_value_c;
               int dummy_value_S;
               int dummy_value_D;
               _unmask = ~mask;            
               srcptr = png_ptr->row_buf + 1;
               dstptr = row;
               len  = png_ptr->width &~7;  
               diff = (int) (png_ptr->width & 7); 

               __asm__ __volatile__ (
                  "movd      _unmask, %%mm7   \n\t" 
                  "psubb     %%mm6, %%mm6     \n\t" 
                  "punpcklbw %%mm7, %%mm7     \n\t"
                  "punpcklwd %%mm7, %%mm7     \n\t"
                  "punpckldq %%mm7, %%mm7     \n\t" 

                  "movq      _mask24_0, %%mm0 \n\t"
                  "movq      _mask24_1, %%mm1 \n\t"
                  "movq      _mask24_2, %%mm2 \n\t"

                  "pand      %%mm7, %%mm0     \n\t"
                  "pand      %%mm7, %%mm1     \n\t"
                  "pand      %%mm7, %%mm2     \n\t"

                  "pcmpeqb   %%mm6, %%mm0     \n\t"
                  "pcmpeqb   %%mm6, %%mm1     \n\t"
                  "pcmpeqb   %%mm6, %%mm2     \n\t"





                  "cmpl      $0, %%ecx        \n\t"
                  "jz        mainloop24end    \n\t"

                "mainloop24:                  \n\t"
                  "movq      (%%esi), %%mm4   \n\t"
                  "pand      %%mm0, %%mm4     \n\t"
                  "movq      %%mm0, %%mm6     \n\t"
                  "movq      (%%edi), %%mm7   \n\t"
                  "pandn     %%mm7, %%mm6     \n\t"
                  "por       %%mm6, %%mm4     \n\t"
                  "movq      %%mm4, (%%edi)   \n\t"

                  "movq      8(%%esi), %%mm5  \n\t"
                  "pand      %%mm1, %%mm5     \n\t"
                  "movq      %%mm1, %%mm7     \n\t"
                  "movq      8(%%edi), %%mm6  \n\t"
                  "pandn     %%mm6, %%mm7     \n\t"
                  "por       %%mm7, %%mm5     \n\t"
                  "movq      %%mm5, 8(%%edi)  \n\t"

                  "movq      16(%%esi), %%mm6 \n\t"
                  "pand      %%mm2, %%mm6     \n\t"
                  "movq      %%mm2, %%mm4     \n\t"
                  "movq      16(%%edi), %%mm7 \n\t"
                  "pandn     %%mm7, %%mm4     \n\t"
                  "por       %%mm4, %%mm6     \n\t"
                  "movq      %%mm6, 16(%%edi) \n\t"

                  "addl      $24, %%esi       \n\t" 
                  "addl      $24, %%edi       \n\t"
                  "subl      $8, %%ecx        \n\t" 

                  "ja        mainloop24       \n\t"

                "mainloop24end:               \n\t"

                  "movl      %%eax, %%ecx     \n\t"
                  "cmpl      $0, %%ecx        \n\t"
                  "jz        end24            \n\t"

                  "sall      $24, %%edx       \n\t" 

                "secondloop24:                \n\t"
                  "sall      %%edx            \n\t" 
                  "jnc       skip24           \n\t" 
                  "movw      (%%esi), %%ax    \n\t"
                  "movw      %%ax, (%%edi)    \n\t"
                  "xorl      %%eax, %%eax     \n\t"
                  "movb      2(%%esi), %%al   \n\t"
                  "movb      %%al, 2(%%edi)   \n\t"

                "skip24:                      \n\t"
                  "addl      $3, %%esi        \n\t"
                  "addl      $3, %%edi        \n\t"
                  "decl      %%ecx            \n\t"
                  "jnz       secondloop24     \n\t"

                "end24:                       \n\t"
                  "EMMS                       \n\t" 

                  : "=a" (dummy_value_a),           
                    "=d" (dummy_value_d),
                    "=c" (dummy_value_c),
                    "=S" (dummy_value_S),
                    "=D" (dummy_value_D)

                  : "3" (srcptr),      
                    "4" (dstptr),      
                    "0" (diff),        

                    "2" (len),         
                    "1" (mask)         

#if 0  /* MMX regs (%mm0, etc.) not supported by gcc 2.7.2.3 or egcs 1.1 */
                  : "%mm0", "%mm1", "%mm2"          
                  , "%mm4", "%mm5", "%mm6", "%mm7"
#endif
               );
            }
            else /* mmx _not supported - Use modified C routine */
#endif /* PNG_ASSEMBLER_CODE_SUPPORTED */
            {
               register png_uint_32 i;
               png_uint_32 initial_val = BPP3 * png_pass_start[png_ptr->pass];
                 /* png.c:  png_pass_start[] = {0, 4, 0, 2, 0, 1, 0}; */
               register int stride = BPP3 * png_pass_inc[png_ptr->pass];
                 /* png.c:  png_pass_inc[] = {8, 8, 4, 4, 2, 2, 1}; */
               register int rep_bytes = BPP3 * png_pass_width[png_ptr->pass];
                 /* png.c:  png_pass_width[] = {8, 4, 4, 2, 2, 1, 1}; */
               png_uint_32 len = png_ptr->width &~7;  /* reduce to mult. of 8 */
               int diff = (int) (png_ptr->width & 7); /* amount lost */
               register png_uint_32 final_val = BPP3 * len;   /* GRR bugfix */

               srcptr = png_ptr->row_buf + 1 + initial_val;
               dstptr = row + initial_val;

               for (i = initial_val; i < final_val; i += stride)
               {
                  png_memcpy(dstptr, srcptr, rep_bytes);
                  srcptr += stride;
                  dstptr += stride;
               }
               if (diff)  /* number of leftover pixels:  3 for pngtest */
               {
                  final_val+=diff*BPP3;
                  for (; i < final_val; i += stride)
                  {
                     if (rep_bytes > (int)(final_val-i))
                        rep_bytes = (int)(final_val-i);
                     png_memcpy(dstptr, srcptr, rep_bytes);
                     srcptr += stride;
                     dstptr += stride;
                  }
               }
            } /* end of else (_mmx_supported) */

            break;
         }       /* end 24 bpp */

         case 32:       /* png_ptr->row_info.pixel_depth */
         {
            png_bytep srcptr;
            png_bytep dstptr;

#if defined(PNG_ASSEMBLER_CODE_SUPPORTED) && defined(PNG_THREAD_UNSAFE_OK)
#if !defined(PNG_1_0_X)
            if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_COMBINE_ROW)
                /* && _mmx_supported */ )
#else
            if (_mmx_supported)
#endif
            {
               png_uint_32 len;
               int diff;
               int dummy_value_a;   
               int dummy_value_d;
               int dummy_value_c;
               int dummy_value_S;
               int dummy_value_D;
               _unmask = ~mask;            
               srcptr = png_ptr->row_buf + 1;
               dstptr = row;
               len  = png_ptr->width &~7;  
               diff = (int) (png_ptr->width & 7); 

               __asm__ __volatile__ (
                  "movd      _unmask, %%mm7   \n\t" 
                  "psubb     %%mm6, %%mm6     \n\t" 
                  "punpcklbw %%mm7, %%mm7     \n\t"
                  "punpcklwd %%mm7, %%mm7     \n\t"
                  "punpckldq %%mm7, %%mm7     \n\t" 

                  "movq      _mask32_0, %%mm0 \n\t"
                  "movq      _mask32_1, %%mm1 \n\t"
                  "movq      _mask32_2, %%mm2 \n\t"
                  "movq      _mask32_3, %%mm3 \n\t"

                  "pand      %%mm7, %%mm0     \n\t"
                  "pand      %%mm7, %%mm1     \n\t"
                  "pand      %%mm7, %%mm2     \n\t"
                  "pand      %%mm7, %%mm3     \n\t"

                  "pcmpeqb   %%mm6, %%mm0     \n\t"
                  "pcmpeqb   %%mm6, %%mm1     \n\t"
                  "pcmpeqb   %%mm6, %%mm2     \n\t"
                  "pcmpeqb   %%mm6, %%mm3     \n\t"





                  "cmpl      $0, %%ecx        \n\t" 
                  "jz        mainloop32end    \n\t"

                "mainloop32:                  \n\t"
                  "movq      (%%esi), %%mm4   \n\t"
                  "pand      %%mm0, %%mm4     \n\t"
                  "movq      %%mm0, %%mm6     \n\t"
                  "movq      (%%edi), %%mm7   \n\t"
                  "pandn     %%mm7, %%mm6     \n\t"
                  "por       %%mm6, %%mm4     \n\t"
                  "movq      %%mm4, (%%edi)   \n\t"

                  "movq      8(%%esi), %%mm5  \n\t"
                  "pand      %%mm1, %%mm5     \n\t"
                  "movq      %%mm1, %%mm7     \n\t"
                  "movq      8(%%edi), %%mm6  \n\t"
                  "pandn     %%mm6, %%mm7     \n\t"
                  "por       %%mm7, %%mm5     \n\t"
                  "movq      %%mm5, 8(%%edi)  \n\t"

                  "movq      16(%%esi), %%mm6 \n\t"
                  "pand      %%mm2, %%mm6     \n\t"
                  "movq      %%mm2, %%mm4     \n\t"
                  "movq      16(%%edi), %%mm7 \n\t"
                  "pandn     %%mm7, %%mm4     \n\t"
                  "por       %%mm4, %%mm6     \n\t"
                  "movq      %%mm6, 16(%%edi) \n\t"

                  "movq      24(%%esi), %%mm7 \n\t"
                  "pand      %%mm3, %%mm7     \n\t"
                  "movq      %%mm3, %%mm5     \n\t"
                  "movq      24(%%edi), %%mm4 \n\t"
                  "pandn     %%mm4, %%mm5     \n\t"
                  "por       %%mm5, %%mm7     \n\t"
                  "movq      %%mm7, 24(%%edi) \n\t"

                  "addl      $32, %%esi       \n\t" 
                  "addl      $32, %%edi       \n\t"
                  "subl      $8, %%ecx        \n\t" 
                  "ja        mainloop32       \n\t"

                "mainloop32end:               \n\t"

                  "movl      %%eax, %%ecx     \n\t"
                  "cmpl      $0, %%ecx        \n\t"
                  "jz        end32            \n\t"

                  "sall      $24, %%edx       \n\t" 

                "secondloop32:                \n\t"
                  "sall      %%edx            \n\t" 
                  "jnc       skip32           \n\t" 
                  "movl      (%%esi), %%eax   \n\t"
                  "movl      %%eax, (%%edi)   \n\t"

                "skip32:                      \n\t"
                  "addl      $4, %%esi        \n\t"
                  "addl      $4, %%edi        \n\t"
                  "decl      %%ecx            \n\t"
                  "jnz       secondloop32     \n\t"

                "end32:                       \n\t"
                  "EMMS                       \n\t" 

                  : "=a" (dummy_value_a),           
                    "=d" (dummy_value_d),
                    "=c" (dummy_value_c),
                    "=S" (dummy_value_S),
                    "=D" (dummy_value_D)

                  : "3" (srcptr),      
                    "4" (dstptr),      
                    "0" (diff),        

                    "2" (len),         
                    "1" (mask)         

#if 0  /* MMX regs (%mm0, etc.) not supported by gcc 2.7.2.3 or egcs 1.1 */
                  : "%mm0", "%mm1", "%mm2", "%mm3"  
                  , "%mm4", "%mm5", "%mm6", "%mm7"
#endif
               );
            }
            else /* mmx _not supported - Use modified C routine */
#endif /* PNG_ASSEMBLER_CODE_SUPPORTED */
            {
               register png_uint_32 i;
               png_uint_32 initial_val = BPP4 * png_pass_start[png_ptr->pass];
                 /* png.c:  png_pass_start[] = {0, 4, 0, 2, 0, 1, 0}; */
               register int stride = BPP4 * png_pass_inc[png_ptr->pass];
                 /* png.c:  png_pass_inc[] = {8, 8, 4, 4, 2, 2, 1}; */
               register int rep_bytes = BPP4 * png_pass_width[png_ptr->pass];
                 /* png.c:  png_pass_width[] = {8, 4, 4, 2, 2, 1, 1}; */
               png_uint_32 len = png_ptr->width &~7;  /* reduce to mult. of 8 */
               int diff = (int) (png_ptr->width & 7); /* amount lost */
               register png_uint_32 final_val = BPP4 * len;   /* GRR bugfix */

               srcptr = png_ptr->row_buf + 1 + initial_val;
               dstptr = row + initial_val;

               for (i = initial_val; i < final_val; i += stride)
               {
                  png_memcpy(dstptr, srcptr, rep_bytes);
                  srcptr += stride;
                  dstptr += stride;
               }
               if (diff)  /* number of leftover pixels:  3 for pngtest */
               {
                  final_val+=diff*BPP4;
                  for (; i < final_val; i += stride)
                  {
                     if (rep_bytes > (int)(final_val-i))
                        rep_bytes = (int)(final_val-i);
                     png_memcpy(dstptr, srcptr, rep_bytes);
                     srcptr += stride;
                     dstptr += stride;
                  }
               }
            } /* end of else (_mmx_supported) */

            break;
         }       /* end 32 bpp */

         case 48:       /* png_ptr->row_info.pixel_depth */
         {
            png_bytep srcptr;
            png_bytep dstptr;

#if defined(PNG_ASSEMBLER_CODE_SUPPORTED) && defined(PNG_THREAD_UNSAFE_OK)
#if !defined(PNG_1_0_X)
            if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_COMBINE_ROW)
                /* && _mmx_supported */ )
#else
            if (_mmx_supported)
#endif
            {
               png_uint_32 len;
               int diff;
               int dummy_value_a;   
               int dummy_value_d;
               int dummy_value_c;
               int dummy_value_S;
               int dummy_value_D;
               _unmask = ~mask;            
               srcptr = png_ptr->row_buf + 1;
               dstptr = row;
               len  = png_ptr->width &~7;  
               diff = (int) (png_ptr->width & 7); 

               __asm__ __volatile__ (
                  "movd      _unmask, %%mm7   \n\t" 
                  "psubb     %%mm6, %%mm6     \n\t" 
                  "punpcklbw %%mm7, %%mm7     \n\t"
                  "punpcklwd %%mm7, %%mm7     \n\t"
                  "punpckldq %%mm7, %%mm7     \n\t" 

                  "movq      _mask48_0, %%mm0 \n\t"
                  "movq      _mask48_1, %%mm1 \n\t"
                  "movq      _mask48_2, %%mm2 \n\t"
                  "movq      _mask48_3, %%mm3 \n\t"
                  "movq      _mask48_4, %%mm4 \n\t"
                  "movq      _mask48_5, %%mm5 \n\t"

                  "pand      %%mm7, %%mm0     \n\t"
                  "pand      %%mm7, %%mm1     \n\t"
                  "pand      %%mm7, %%mm2     \n\t"
                  "pand      %%mm7, %%mm3     \n\t"
                  "pand      %%mm7, %%mm4     \n\t"
                  "pand      %%mm7, %%mm5     \n\t"

                  "pcmpeqb   %%mm6, %%mm0     \n\t"
                  "pcmpeqb   %%mm6, %%mm1     \n\t"
                  "pcmpeqb   %%mm6, %%mm2     \n\t"
                  "pcmpeqb   %%mm6, %%mm3     \n\t"
                  "pcmpeqb   %%mm6, %%mm4     \n\t"
                  "pcmpeqb   %%mm6, %%mm5     \n\t"





                  "cmpl      $0, %%ecx        \n\t"
                  "jz        mainloop48end    \n\t"

                "mainloop48:                  \n\t"
                  "movq      (%%esi), %%mm7   \n\t"
                  "pand      %%mm0, %%mm7     \n\t"
                  "movq      %%mm0, %%mm6     \n\t"
                  "pandn     (%%edi), %%mm6   \n\t"
                  "por       %%mm6, %%mm7     \n\t"
                  "movq      %%mm7, (%%edi)   \n\t"

                  "movq      8(%%esi), %%mm6  \n\t"
                  "pand      %%mm1, %%mm6     \n\t"
                  "movq      %%mm1, %%mm7     \n\t"
                  "pandn     8(%%edi), %%mm7  \n\t"
                  "por       %%mm7, %%mm6     \n\t"
                  "movq      %%mm6, 8(%%edi)  \n\t"

                  "movq      16(%%esi), %%mm6 \n\t"
                  "pand      %%mm2, %%mm6     \n\t"
                  "movq      %%mm2, %%mm7     \n\t"
                  "pandn     16(%%edi), %%mm7 \n\t"
                  "por       %%mm7, %%mm6     \n\t"
                  "movq      %%mm6, 16(%%edi) \n\t"

                  "movq      24(%%esi), %%mm7 \n\t"
                  "pand      %%mm3, %%mm7     \n\t"
                  "movq      %%mm3, %%mm6     \n\t"
                  "pandn     24(%%edi), %%mm6 \n\t"
                  "por       %%mm6, %%mm7     \n\t"
                  "movq      %%mm7, 24(%%edi) \n\t"

                  "movq      32(%%esi), %%mm6 \n\t"
                  "pand      %%mm4, %%mm6     \n\t"
                  "movq      %%mm4, %%mm7     \n\t"
                  "pandn     32(%%edi), %%mm7 \n\t"
                  "por       %%mm7, %%mm6     \n\t"
                  "movq      %%mm6, 32(%%edi) \n\t"

                  "movq      40(%%esi), %%mm7 \n\t"
                  "pand      %%mm5, %%mm7     \n\t"
                  "movq      %%mm5, %%mm6     \n\t"
                  "pandn     40(%%edi), %%mm6 \n\t"
                  "por       %%mm6, %%mm7     \n\t"
                  "movq      %%mm7, 40(%%edi) \n\t"

                  "addl      $48, %%esi       \n\t" 
                  "addl      $48, %%edi       \n\t"
                  "subl      $8, %%ecx        \n\t" 

                  "ja        mainloop48       \n\t"

                "mainloop48end:               \n\t"

                  "movl      %%eax, %%ecx     \n\t"
                  "cmpl      $0, %%ecx        \n\t"
                  "jz        end48            \n\t"

                  "sall      $24, %%edx       \n\t" 

                "secondloop48:                \n\t"
                  "sall      %%edx            \n\t" 
                  "jnc       skip48           \n\t" 
                  "movl      (%%esi), %%eax   \n\t"
                  "movl      %%eax, (%%edi)   \n\t"

                "skip48:                      \n\t"
                  "addl      $4, %%esi        \n\t"
                  "addl      $4, %%edi        \n\t"
                  "decl      %%ecx            \n\t"
                  "jnz       secondloop48     \n\t"

                "end48:                       \n\t"
                  "EMMS                       \n\t" 

                  : "=a" (dummy_value_a),           
                    "=d" (dummy_value_d),
                    "=c" (dummy_value_c),
                    "=S" (dummy_value_S),
                    "=D" (dummy_value_D)

                  : "3" (srcptr),      
                    "4" (dstptr),      
                    "0" (diff),        

                    "2" (len),         
                    "1" (mask)         

#if 0  /* MMX regs (%mm0, etc.) not supported by gcc 2.7.2.3 or egcs 1.1 */
                  : "%mm0", "%mm1", "%mm2", "%mm3"  
                  , "%mm4", "%mm5", "%mm6", "%mm7"
#endif
               );
            }
            else /* mmx _not supported - Use modified C routine */
#endif /* PNG_ASSEMBLER_CODE_SUPPORTED */
            {
               register png_uint_32 i;
               png_uint_32 initial_val = BPP6 * png_pass_start[png_ptr->pass];
                 /* png.c:  png_pass_start[] = {0, 4, 0, 2, 0, 1, 0}; */
               register int stride = BPP6 * png_pass_inc[png_ptr->pass];
                 /* png.c:  png_pass_inc[] = {8, 8, 4, 4, 2, 2, 1}; */
               register int rep_bytes = BPP6 * png_pass_width[png_ptr->pass];
                 /* png.c:  png_pass_width[] = {8, 4, 4, 2, 2, 1, 1}; */
               png_uint_32 len = png_ptr->width &~7;  /* reduce to mult. of 8 */
               int diff = (int) (png_ptr->width & 7); /* amount lost */
               register png_uint_32 final_val = BPP6 * len;   /* GRR bugfix */

               srcptr = png_ptr->row_buf + 1 + initial_val;
               dstptr = row + initial_val;

               for (i = initial_val; i < final_val; i += stride)
               {
                  png_memcpy(dstptr, srcptr, rep_bytes);
                  srcptr += stride;
                  dstptr += stride;
               }
               if (diff)  /* number of leftover pixels:  3 for pngtest */
               {
                  final_val+=diff*BPP6;
                  for (; i < final_val; i += stride)
                  {
                     if (rep_bytes > (int)(final_val-i))
                        rep_bytes = (int)(final_val-i);
                     png_memcpy(dstptr, srcptr, rep_bytes);
                     srcptr += stride;
                     dstptr += stride;
                  }
               }
            } /* end of else (_mmx_supported) */

            break;
         }       /* end 48 bpp */

         case 64:       /* png_ptr->row_info.pixel_depth */
         {
            png_bytep srcptr;
            png_bytep dstptr;
            register png_uint_32 i;
            png_uint_32 initial_val = BPP8 * png_pass_start[png_ptr->pass];
              /* png.c:  png_pass_start[] = {0, 4, 0, 2, 0, 1, 0}; */
            register int stride = BPP8 * png_pass_inc[png_ptr->pass];
              /* png.c:  png_pass_inc[] = {8, 8, 4, 4, 2, 2, 1}; */
            register int rep_bytes = BPP8 * png_pass_width[png_ptr->pass];
              /* png.c:  png_pass_width[] = {8, 4, 4, 2, 2, 1, 1}; */
            png_uint_32 len = png_ptr->width &~7;  /* reduce to mult. of 8 */
            int diff = (int) (png_ptr->width & 7); /* amount lost */
            register png_uint_32 final_val = BPP8 * len;   /* GRR bugfix */

            srcptr = png_ptr->row_buf + 1 + initial_val;
            dstptr = row + initial_val;

            for (i = initial_val; i < final_val; i += stride)
            {
               png_memcpy(dstptr, srcptr, rep_bytes);
               srcptr += stride;
               dstptr += stride;
            }
            if (diff)  /* number of leftover pixels:  3 for pngtest */
            {
               final_val+=diff*BPP8;
               for (; i < final_val; i += stride)
               {
                  if (rep_bytes > (int)(final_val-i))
                     rep_bytes = (int)(final_val-i);
                  png_memcpy(dstptr, srcptr, rep_bytes);
                  srcptr += stride;
                  dstptr += stride;
               }
            }

            break;
         }       /* end 64 bpp */

         default: /* png_ptr->row_info.pixel_depth != 1,2,4,8,16,24,32,48,64 */
         {
            /* this should never happen */
            png_warning(png_ptr, "Invalid row_info.pixel_depth in pnggccrd");
            break;
         }
      } /* end switch (png_ptr->row_info.pixel_depth) */

   } /* end if (non-trivial mask) */

} /* end png_combine_row() */

#endif /* PNG_HAVE_ASSEMBLER_COMBINE_ROW */




/*===========================================================================*/
/*                                                                           */
/*                 P N G _ D O _ R E A D _ I N T E R L A C E                 */
/*                                                                           */
/*===========================================================================*/

#if defined(PNG_READ_INTERLACING_SUPPORTED)
#if defined(PNG_HAVE_ASSEMBLER_READ_INTERLACE)

/* png_do_read_interlace() is called after any 16-bit to 8-bit conversion
 * has taken place.  [GRR: what other steps come before and/or after?]
 */

void /* PRIVATE */
png_do_read_interlace(png_structp png_ptr)
{
   png_row_infop row_info = &(png_ptr->row_info);
   png_bytep row = png_ptr->row_buf + 1;
   int pass = png_ptr->pass;
#if defined(PNG_READ_PACKSWAP_SUPPORTED)
   png_uint_32 transformations = png_ptr->transformations;
#endif

   png_debug(1, "in png_do_read_interlace (pnggccrd.c)\n");

#if defined(PNG_ASSEMBLER_CODE_SUPPORTED)
   if (_mmx_supported == 2) {
#if !defined(PNG_1_0_X)
       /* this should have happened in png_init_mmx_flags() already */
       png_warning(png_ptr, "asm_flags may not have been initialized");
#endif
       png_mmx_support();
   }
#endif

   if (row != NULL && row_info != NULL)
   {
      png_uint_32 final_width;

      final_width = row_info->width * png_pass_inc[pass];

      switch (row_info->pixel_depth)
      {
         case 1:
         {
            png_bytep sp, dp;
            int sshift, dshift;
            int s_start, s_end, s_inc;
            png_byte v;
            png_uint_32 i;
            int j;

            sp = row + (png_size_t)((row_info->width - 1) >> 3);
            dp = row + (png_size_t)((final_width - 1) >> 3);
#if defined(PNG_READ_PACKSWAP_SUPPORTED)
            if (transformations & PNG_PACKSWAP)
            {
               sshift = (int)((row_info->width + 7) & 7);
               dshift = (int)((final_width + 7) & 7);
               s_start = 7;
               s_end = 0;
               s_inc = -1;
            }
            else
#endif
            {
               sshift = 7 - (int)((row_info->width + 7) & 7);
               dshift = 7 - (int)((final_width + 7) & 7);
               s_start = 0;
               s_end = 7;
               s_inc = 1;
            }

            for (i = row_info->width; i; i--)
            {
               v = (png_byte)((*sp >> sshift) & 0x1);
               for (j = 0; j < png_pass_inc[pass]; j++)
               {
                  *dp &= (png_byte)((0x7f7f >> (7 - dshift)) & 0xff);
                  *dp |= (png_byte)(v << dshift);
                  if (dshift == s_end)
                  {
                     dshift = s_start;
                     dp--;
                  }
                  else
                     dshift += s_inc;
               }
               if (sshift == s_end)
               {
                  sshift = s_start;
                  sp--;
               }
               else
                  sshift += s_inc;
            }
            break;
         }

         case 2:
         {
            png_bytep sp, dp;
            int sshift, dshift;
            int s_start, s_end, s_inc;
            png_uint_32 i;

            sp = row + (png_size_t)((row_info->width - 1) >> 2);
            dp = row + (png_size_t)((final_width - 1) >> 2);
#if defined(PNG_READ_PACKSWAP_SUPPORTED)
            if (transformations & PNG_PACKSWAP)
            {
               sshift = (png_size_t)(((row_info->width + 3) & 3) << 1);
               dshift = (png_size_t)(((final_width + 3) & 3) << 1);
               s_start = 6;
               s_end = 0;
               s_inc = -2;
            }
            else
#endif
            {
               sshift = (png_size_t)((3 - ((row_info->width + 3) & 3)) << 1);
               dshift = (png_size_t)((3 - ((final_width + 3) & 3)) << 1);
               s_start = 0;
               s_end = 6;
               s_inc = 2;
            }

            for (i = row_info->width; i; i--)
            {
               png_byte v;
               int j;

               v = (png_byte)((*sp >> sshift) & 0x3);
               for (j = 0; j < png_pass_inc[pass]; j++)
               {
                  *dp &= (png_byte)((0x3f3f >> (6 - dshift)) & 0xff);
                  *dp |= (png_byte)(v << dshift);
                  if (dshift == s_end)
                  {
                     dshift = s_start;
                     dp--;
                  }
                  else
                     dshift += s_inc;
               }
               if (sshift == s_end)
               {
                  sshift = s_start;
                  sp--;
               }
               else
                  sshift += s_inc;
            }
            break;
         }

         case 4:
         {
            png_bytep sp, dp;
            int sshift, dshift;
            int s_start, s_end, s_inc;
            png_uint_32 i;

            sp = row + (png_size_t)((row_info->width - 1) >> 1);
            dp = row + (png_size_t)((final_width - 1) >> 1);
#if defined(PNG_READ_PACKSWAP_SUPPORTED)
            if (transformations & PNG_PACKSWAP)
            {
               sshift = (png_size_t)(((row_info->width + 1) & 1) << 2);
               dshift = (png_size_t)(((final_width + 1) & 1) << 2);
               s_start = 4;
               s_end = 0;
               s_inc = -4;
            }
            else
#endif
            {
               sshift = (png_size_t)((1 - ((row_info->width + 1) & 1)) << 2);
               dshift = (png_size_t)((1 - ((final_width + 1) & 1)) << 2);
               s_start = 0;
               s_end = 4;
               s_inc = 4;
            }

            for (i = row_info->width; i; i--)
            {
               png_byte v;
               int j;

               v = (png_byte)((*sp >> sshift) & 0xf);
               for (j = 0; j < png_pass_inc[pass]; j++)
               {
                  *dp &= (png_byte)((0xf0f >> (4 - dshift)) & 0xff);
                  *dp |= (png_byte)(v << dshift);
                  if (dshift == s_end)
                  {
                     dshift = s_start;
                     dp--;
                  }
                  else
                     dshift += s_inc;
               }
               if (sshift == s_end)
               {
                  sshift = s_start;
                  sp--;
               }
               else
                  sshift += s_inc;
            }
            break;
         }

       /*====================================================================*/

         default: /* 8-bit or larger (this is where the routine is modified) */
         {
#if 0




#endif
            png_bytep sptr, dp;
            png_uint_32 i;
            png_size_t pixel_bytes;
            int width = (int)row_info->width;

            pixel_bytes = (row_info->pixel_depth >> 3);

            /* point sptr at the last pixel in the pre-expanded row: */
            sptr = row + (width - 1) * pixel_bytes;

            /* point dp at the last pixel position in the expanded row: */
            dp = row + (final_width - 1) * pixel_bytes;

            /* New code by Nirav Chhatrapati - Intel Corporation */

#if defined(PNG_ASSEMBLER_CODE_SUPPORTED)
#if !defined(PNG_1_0_X)
            if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_INTERLACE)
                /* && _mmx_supported */ )
#else
            if (_mmx_supported)
#endif
            {
               
               if (pixel_bytes == 3)
               {
                  if (((pass == 0) || (pass == 1)) && width)
                  {
                     int dummy_value_c;   
                     int dummy_value_S;
                     int dummy_value_D;
                     int dummy_value_a;

                     __asm__ __volatile__ (
                        "subl $21, %%edi         \n\t"
                                     

                     ".loop3_pass0:              \n\t"
                        "movd (%%esi), %%mm0     \n\t" 
                        "pand (%3), %%mm0        \n\t" 
                        "movq %%mm0, %%mm1       \n\t" 
                        "psllq $16, %%mm0        \n\t" 
                        "movq %%mm0, %%mm2       \n\t" 
                        "psllq $24, %%mm0        \n\t" 
                        "psrlq $8, %%mm1         \n\t" 
                        "por %%mm2, %%mm0        \n\t" 
                        "por %%mm1, %%mm0        \n\t" 
                        "movq %%mm0, %%mm3       \n\t" 
                        "psllq $16, %%mm0        \n\t" 
                        "movq %%mm3, %%mm4       \n\t" 
                        "punpckhdq %%mm0, %%mm3  \n\t" 
                        "movq %%mm4, 16(%%edi)   \n\t"
                        "psrlq $32, %%mm0        \n\t" 
                        "movq %%mm3, 8(%%edi)    \n\t"
                        "punpckldq %%mm4, %%mm0  \n\t" 
                        "subl $3, %%esi          \n\t"
                        "movq %%mm0, (%%edi)     \n\t"
                        "subl $24, %%edi         \n\t"
                        "decl %%ecx              \n\t"
                        "jnz .loop3_pass0        \n\t"
                        "EMMS                    \n\t" 

                        : "=c" (dummy_value_c),        
                          "=S" (dummy_value_S),
                          "=D" (dummy_value_D),
                          "=a" (dummy_value_a)


                        : "1" (sptr),      
                          "2" (dp),        
                          "0" (width),     
                          "3" (&_const4)  

#if 0  /* %mm0, ..., %mm4 not supported by gcc 2.7.2.3 or egcs 1.1 */
                        : "%mm0", "%mm1", "%mm2"       
                        , "%mm3", "%mm4"
#endif
                     );
                  }
                  else if (((pass == 2) || (pass == 3)) && width)
                  {
                     int dummy_value_c;   
                     int dummy_value_S;
                     int dummy_value_D;
                     int dummy_value_a;

                     __asm__ __volatile__ (
                        "subl $9, %%edi          \n\t"
                                     

                     ".loop3_pass2:              \n\t"
                        "movd (%%esi), %%mm0     \n\t" 
                        "pand (%3), %%mm0     \n\t" 
                        "movq %%mm0, %%mm1       \n\t" 
                        "psllq $16, %%mm0        \n\t" 
                        "movq %%mm0, %%mm2       \n\t" 
                        "psllq $24, %%mm0        \n\t" 
                        "psrlq $8, %%mm1         \n\t" 
                        "por %%mm2, %%mm0        \n\t" 
                        "por %%mm1, %%mm0        \n\t" 
                        "movq %%mm0, 4(%%edi)    \n\t"
                        "psrlq $16, %%mm0        \n\t" 
                        "subl $3, %%esi          \n\t"
                        "movd %%mm0, (%%edi)     \n\t"
                        "subl $12, %%edi         \n\t"
                        "decl %%ecx              \n\t"
                        "jnz .loop3_pass2        \n\t"
                        "EMMS                    \n\t" 

                        : "=c" (dummy_value_c),        
                          "=S" (dummy_value_S),
                          "=D" (dummy_value_D),
                          "=a" (dummy_value_a)

                        : "1" (sptr),      
                          "2" (dp),        
                          "0" (width),     
                          "3" (&_const4)  

#if 0  /* %mm0, ..., %mm2 not supported by gcc 2.7.2.3 or egcs 1.1 */
                        : "%mm0", "%mm1", "%mm2"       
#endif
                     );
                  }
                  else if (width) /* && ((pass == 4) || (pass == 5)) */
                  {
                     int width_mmx = ((width >> 1) << 1) - 8;   
                     if (width_mmx < 0)
                         width_mmx = 0;
                     width -= width_mmx;        
                     if (width_mmx)
                     {
                        
                        
                        
                        int dummy_value_c;  
                        int dummy_value_S;
                        int dummy_value_D;
                        int dummy_value_a;
                        int dummy_value_d;

                        __asm__ __volatile__ (
                           "subl $3, %%esi          \n\t"
                           "subl $9, %%edi          \n\t"
                                        

                        ".loop3_pass4:              \n\t"
                           "movq (%%esi), %%mm0     \n\t" 
                           "movq %%mm0, %%mm1       \n\t" 
                           "movq %%mm0, %%mm2       \n\t" 
                           "psllq $24, %%mm0        \n\t" 
                           "pand (%3), %%mm1          \n\t" 
                           "psrlq $24, %%mm2        \n\t" 
                           "por %%mm1, %%mm0        \n\t" 
                           "movq %%mm2, %%mm3       \n\t" 
                           "psllq $8, %%mm2         \n\t" 
                           "movq %%mm0, (%%edi)     \n\t"
                           "psrlq $16, %%mm3        \n\t" 
                           "pand (%4), %%mm3     \n\t" 
                           "por %%mm3, %%mm2        \n\t" 
                           "subl $6, %%esi          \n\t"
                           "movd %%mm2, 8(%%edi)    \n\t"
                           "subl $12, %%edi         \n\t"
                           "subl $2, %%ecx          \n\t"
                           "jnz .loop3_pass4        \n\t"
                           "EMMS                    \n\t" 

                           : "=c" (dummy_value_c),        
                             "=S" (dummy_value_S),
                             "=D" (dummy_value_D),
                             "=a" (dummy_value_a),
                             "=d" (dummy_value_d)

                           : "1" (sptr),      
                             "2" (dp),        
                             "0" (width_mmx), 
                             "3" (&_const4), 
                             "4" (&_const6)  

#if 0  /* %mm0, ..., %mm3 not supported by gcc 2.7.2.3 or egcs 1.1 */
                           : "%mm0", "%mm1"               
                           , "%mm2", "%mm3"
#endif
                        );
                     }

                     sptr -= width_mmx*3;
                     dp -= width_mmx*6;
                     for (i = width; i; i--)
                     {
                        png_byte v[8];
                        int j;

                        png_memcpy(v, sptr, 3);
                        for (j = 0; j < png_pass_inc[pass]; j++)
                        {
                           png_memcpy(dp, v, 3);
                           dp -= 3;
                        }
                        sptr -= 3;
                     }
                  }
               } /* end of pixel_bytes == 3 */

               
               else if (pixel_bytes == 1)
               {
                  if (((pass == 0) || (pass == 1)) && width)
                  {
                     int width_mmx = ((width >> 2) << 2);
                     width -= width_mmx;        
                     if (width_mmx)
                     {
                        int dummy_value_c;  
                        int dummy_value_S;
                        int dummy_value_D;

                        __asm__ __volatile__ (
                           "subl $3, %%esi          \n\t"
                           "subl $31, %%edi         \n\t"

                        ".loop1_pass0:              \n\t"
                           "movd (%%esi), %%mm0     \n\t" 
                           "movq %%mm0, %%mm1       \n\t" 
                           "punpcklbw %%mm0, %%mm0  \n\t" 
                           "movq %%mm0, %%mm2       \n\t" 
                           "punpcklwd %%mm0, %%mm0  \n\t" 
                           "movq %%mm0, %%mm3       \n\t" 
                           "punpckldq %%mm0, %%mm0  \n\t" 
                           "punpckhdq %%mm3, %%mm3  \n\t" 
                           "movq %%mm0, (%%edi)     \n\t"
                           "punpckhwd %%mm2, %%mm2  \n\t" 
                           "movq %%mm3, 8(%%edi)    \n\t"
                           "movq %%mm2, %%mm4       \n\t" 
                           "punpckldq %%mm2, %%mm2  \n\t" 
                           "punpckhdq %%mm4, %%mm4  \n\t" 
                           "movq %%mm2, 16(%%edi)   \n\t"
                           "subl $4, %%esi          \n\t"
                           "movq %%mm4, 24(%%edi)   \n\t"
                           "subl $32, %%edi         \n\t"
                           "subl $4, %%ecx          \n\t"
                           "jnz .loop1_pass0        \n\t"
                           "EMMS                    \n\t" 

                           : "=c" (dummy_value_c),        
                             "=S" (dummy_value_S),
                             "=D" (dummy_value_D)

                           : "1" (sptr),      
                             "2" (dp),        
                             "0" (width_mmx)  

#if 0  /* %mm0, ..., %mm4 not supported by gcc 2.7.2.3 or egcs 1.1 */
                           : "%mm0", "%mm1", "%mm2"       
                           , "%mm3", "%mm4"
#endif
                        );
                     }

                     sptr -= width_mmx;
                     dp -= width_mmx*8;
                     for (i = width; i; i--)
                     {
                        int j;

                       /* I simplified this part in version 1.0.4e
                        * here and in several other instances where
                        * pixel_bytes == 1  -- GR-P
                        *
                        * Original code:
                        *
                        * png_byte v[8];
                        * png_memcpy(v, sptr, pixel_bytes);
                        * for (j = 0; j < png_pass_inc[pass]; j++)
                        * {
                        *    png_memcpy(dp, v, pixel_bytes);
                        *    dp -= pixel_bytes;
                        * }
                        * sptr -= pixel_bytes;
                        *
                        * Replacement code is in the next three lines:
                        */

                        for (j = 0; j < png_pass_inc[pass]; j++)
                        {
                           *dp-- = *sptr;
                        }
                        --sptr;
                     }
                  }
                  else if (((pass == 2) || (pass == 3)) && width)
                  {
                     int width_mmx = ((width >> 2) << 2);
                     width -= width_mmx;        
                     if (width_mmx)
                     {
                        int dummy_value_c;  
                        int dummy_value_S;
                        int dummy_value_D;

                        __asm__ __volatile__ (
                           "subl $3, %%esi          \n\t"
                           "subl $15, %%edi         \n\t"

                        ".loop1_pass2:              \n\t"
                           "movd (%%esi), %%mm0     \n\t" 
                           "punpcklbw %%mm0, %%mm0  \n\t" 
                           "movq %%mm0, %%mm1       \n\t" 
                           "punpcklwd %%mm0, %%mm0  \n\t" 
                           "punpckhwd %%mm1, %%mm1  \n\t" 
                           "movq %%mm0, (%%edi)     \n\t"
                           "subl $4, %%esi          \n\t"
                           "movq %%mm1, 8(%%edi)    \n\t"
                           "subl $16, %%edi         \n\t"
                           "subl $4, %%ecx          \n\t"
                           "jnz .loop1_pass2        \n\t"
                           "EMMS                    \n\t" 

                           : "=c" (dummy_value_c),        
                             "=S" (dummy_value_S),
                             "=D" (dummy_value_D)

                           : "1" (sptr),      
                             "2" (dp),        
                             "0" (width_mmx)  

#if 0  /* %mm0, %mm1 not supported by gcc 2.7.2.3 or egcs 1.1 */
                           : "%mm0", "%mm1"               
#endif
                        );
                     }

                     sptr -= width_mmx;
                     dp -= width_mmx*4;
                     for (i = width; i; i--)
                     {
                        int j;

                        for (j = 0; j < png_pass_inc[pass]; j++)
                        {
                           *dp-- = *sptr;
                        }
                        --sptr;
                     }
                  }
                  else if (width)  /* && ((pass == 4) || (pass == 5)) */
                  {
                     int width_mmx = ((width >> 3) << 3);
                     width -= width_mmx;        
                     if (width_mmx)
                     {
                        int dummy_value_c;  
                        int dummy_value_S;
                        int dummy_value_D;

                        __asm__ __volatile__ (
                           "subl $7, %%esi          \n\t"
                           "subl $15, %%edi         \n\t"

                        ".loop1_pass4:              \n\t"
                           "movq (%%esi), %%mm0     \n\t" 
                           "movq %%mm0, %%mm1       \n\t" 
                           "punpcklbw %%mm0, %%mm0  \n\t" 
                           "punpckhbw %%mm1, %%mm1  \n\t" 
                           "movq %%mm1, 8(%%edi)    \n\t"
                           "subl $8, %%esi          \n\t"
                           "movq %%mm0, (%%edi)     \n\t"
                           "subl $16, %%edi         \n\t"
                           "subl $8, %%ecx          \n\t"
                           "jnz .loop1_pass4        \n\t"
                           "EMMS                    \n\t" 

                           : "=c" (dummy_value_c),        
                             "=S" (dummy_value_S),
                             "=D" (dummy_value_D)

                           : "1" (sptr),      
                             "2" (dp),        
                             "0" (width_mmx)  

#if 0  /* %mm0, %mm1 not supported by gcc 2.7.2.3 or egcs 1.1 */
                           : "%mm0", "%mm1"               
#endif
                        );
                     }

                     sptr -= width_mmx;
                     dp -= width_mmx*2;
                     for (i = width; i; i--)
                     {
                        int j;

                        for (j = 0; j < png_pass_inc[pass]; j++)
                        {
                           *dp-- = *sptr;
                        }
                        --sptr;
                     }
                  }
               } /* end of pixel_bytes == 1 */

               
               else if (pixel_bytes == 2)
               {
                  if (((pass == 0) || (pass == 1)) && width)
                  {
                     int width_mmx = ((width >> 1) << 1);
                     width -= width_mmx;        
                     if (width_mmx)
                     {
                        int dummy_value_c;  
                        int dummy_value_S;
                        int dummy_value_D;

                        __asm__ __volatile__ (
                           "subl $2, %%esi          \n\t"
                           "subl $30, %%edi         \n\t"

                        ".loop2_pass0:              \n\t"
                           "movd (%%esi), %%mm0     \n\t" 
                           "punpcklwd %%mm0, %%mm0  \n\t" 
                           "movq %%mm0, %%mm1       \n\t" 
                           "punpckldq %%mm0, %%mm0  \n\t" 
                           "punpckhdq %%mm1, %%mm1  \n\t" 
                           "movq %%mm0, (%%edi)     \n\t"
                           "movq %%mm0, 8(%%edi)    \n\t"
                           "movq %%mm1, 16(%%edi)   \n\t"
                           "subl $4, %%esi          \n\t"
                           "movq %%mm1, 24(%%edi)   \n\t"
                           "subl $32, %%edi         \n\t"
                           "subl $2, %%ecx          \n\t"
                           "jnz .loop2_pass0        \n\t"
                           "EMMS                    \n\t" 

                           : "=c" (dummy_value_c),        
                             "=S" (dummy_value_S),
                             "=D" (dummy_value_D)

                           : "1" (sptr),      
                             "2" (dp),        
                             "0" (width_mmx)  

#if 0  /* %mm0, %mm1 not supported by gcc 2.7.2.3 or egcs 1.1 */
                           : "%mm0", "%mm1"               
#endif
                        );
                     }

                     sptr -= (width_mmx*2 - 2); 
                     dp -= (width_mmx*16 - 2);  
                     for (i = width; i; i--)
                     {
                        png_byte v[8];
                        int j;
                        sptr -= 2;
                        png_memcpy(v, sptr, 2);
                        for (j = 0; j < png_pass_inc[pass]; j++)
                        {
                           dp -= 2;
                           png_memcpy(dp, v, 2);
                        }
                     }
                  }
                  else if (((pass == 2) || (pass == 3)) && width)
                  {
                     int width_mmx = ((width >> 1) << 1) ;
                     width -= width_mmx;        
                     if (width_mmx)
                     {
                        int dummy_value_c;  
                        int dummy_value_S;
                        int dummy_value_D;

                        __asm__ __volatile__ (
                           "subl $2, %%esi          \n\t"
                           "subl $14, %%edi         \n\t"

                        ".loop2_pass2:              \n\t"
                           "movd (%%esi), %%mm0     \n\t" 
                           "punpcklwd %%mm0, %%mm0  \n\t" 
                           "movq %%mm0, %%mm1       \n\t" 
                           "punpckldq %%mm0, %%mm0  \n\t" 
                           "punpckhdq %%mm1, %%mm1  \n\t" 
                           "movq %%mm0, (%%edi)     \n\t"
                           "subl $4, %%esi          \n\t"
                           "movq %%mm1, 8(%%edi)    \n\t"
                           "subl $16, %%edi         \n\t"
                           "subl $2, %%ecx          \n\t"
                           "jnz .loop2_pass2        \n\t"
                           "EMMS                    \n\t" 

                           : "=c" (dummy_value_c),        
                             "=S" (dummy_value_S),
                             "=D" (dummy_value_D)

                           : "1" (sptr),      
                             "2" (dp),        
                             "0" (width_mmx)  

#if 0  /* %mm0, %mm1 not supported by gcc 2.7.2.3 or egcs 1.1 */
                           : "%mm0", "%mm1"               
#endif
                        );
                     }

                     sptr -= (width_mmx*2 - 2); 
                     dp -= (width_mmx*8 - 2);   
                     for (i = width; i; i--)
                     {
                        png_byte v[8];
                        int j;
                        sptr -= 2;
                        png_memcpy(v, sptr, 2);
                        for (j = 0; j < png_pass_inc[pass]; j++)
                        {
                           dp -= 2;
                           png_memcpy(dp, v, 2);
                        }
                     }
                  }
                  else if (width)  
                  {
                     int width_mmx = ((width >> 1) << 1) ;
                     width -= width_mmx;        
                     if (width_mmx)
                     {
                        int dummy_value_c;  
                        int dummy_value_S;
                        int dummy_value_D;

                        __asm__ __volatile__ (
                           "subl $2, %%esi          \n\t"
                           "subl $6, %%edi          \n\t"

                        ".loop2_pass4:              \n\t"
                           "movd (%%esi), %%mm0     \n\t" 
                           "punpcklwd %%mm0, %%mm0  \n\t" 
                           "subl $4, %%esi          \n\t"
                           "movq %%mm0, (%%edi)     \n\t"
                           "subl $8, %%edi          \n\t"
                           "subl $2, %%ecx          \n\t"
                           "jnz .loop2_pass4        \n\t"
                           "EMMS                    \n\t" 

                           : "=c" (dummy_value_c),        
                             "=S" (dummy_value_S),
                             "=D" (dummy_value_D)

                           : "1" (sptr),      
                             "2" (dp),        
                             "0" (width_mmx)  

#if 0  /* %mm0 not supported by gcc 2.7.2.3 or egcs 1.1 */
                           : "%mm0"                       
#endif
                        );
                     }

                     sptr -= (width_mmx*2 - 2); 
                     dp -= (width_mmx*4 - 2);   
                     for (i = width; i; i--)
                     {
                        png_byte v[8];
                        int j;
                        sptr -= 2;
                        png_memcpy(v, sptr, 2);
                        for (j = 0; j < png_pass_inc[pass]; j++)
                        {
                           dp -= 2;
                           png_memcpy(dp, v, 2);
                        }
                     }
                  }
               } /* end of pixel_bytes == 2 */

               
               else if (pixel_bytes == 4)
               {
                  if (((pass == 0) || (pass == 1)) && width)
                  {
                     int width_mmx = ((width >> 1) << 1);
                     width -= width_mmx;        
                     if (width_mmx)
                     {
                        int dummy_value_c;  
                        int dummy_value_S;
                        int dummy_value_D;

                        __asm__ __volatile__ (
                           "subl $4, %%esi          \n\t"
                           "subl $60, %%edi         \n\t"

                        ".loop4_pass0:              \n\t"
                           "movq (%%esi), %%mm0     \n\t" 
                           "movq %%mm0, %%mm1       \n\t" 
                           "punpckldq %%mm0, %%mm0  \n\t" 
                           "punpckhdq %%mm1, %%mm1  \n\t" 
                           "movq %%mm0, (%%edi)     \n\t"
                           "movq %%mm0, 8(%%edi)    \n\t"
                           "movq %%mm0, 16(%%edi)   \n\t"
                           "movq %%mm0, 24(%%edi)   \n\t"
                           "movq %%mm1, 32(%%edi)   \n\t"
                           "movq %%mm1, 40(%%edi)   \n\t"
                           "movq %%mm1, 48(%%edi)   \n\t"
                           "subl $8, %%esi          \n\t"
                           "movq %%mm1, 56(%%edi)   \n\t"
                           "subl $64, %%edi         \n\t"
                           "subl $2, %%ecx          \n\t"
                           "jnz .loop4_pass0        \n\t"
                           "EMMS                    \n\t" 

                           : "=c" (dummy_value_c),        
                             "=S" (dummy_value_S),
                             "=D" (dummy_value_D)

                           : "1" (sptr),      
                             "2" (dp),        
                             "0" (width_mmx)  

#if 0  /* %mm0, %mm1 not supported by gcc 2.7.2.3 or egcs 1.1 */
                           : "%mm0", "%mm1"               
#endif
                        );
                     }

                     sptr -= (width_mmx*4 - 4); 
                     dp -= (width_mmx*32 - 4);  
                     for (i = width; i; i--)
                     {
                        png_byte v[8];
                        int j;
                        sptr -= 4;
                        png_memcpy(v, sptr, 4);
                        for (j = 0; j < png_pass_inc[pass]; j++)
                        {
                           dp -= 4;
                           png_memcpy(dp, v, 4);
                        }
                     }
                  }
                  else if (((pass == 2) || (pass == 3)) && width)
                  {
                     int width_mmx = ((width >> 1) << 1);
                     width -= width_mmx;        
                     if (width_mmx)
                     {
                        int dummy_value_c;  
                        int dummy_value_S;
                        int dummy_value_D;

                        __asm__ __volatile__ (
                           "subl $4, %%esi          \n\t"
                           "subl $28, %%edi         \n\t"

                        ".loop4_pass2:              \n\t"
                           "movq (%%esi), %%mm0     \n\t" 
                           "movq %%mm0, %%mm1       \n\t" 
                           "punpckldq %%mm0, %%mm0  \n\t" 
                           "punpckhdq %%mm1, %%mm1  \n\t" 
                           "movq %%mm0, (%%edi)     \n\t"
                           "movq %%mm0, 8(%%edi)    \n\t"
                           "movq %%mm1, 16(%%edi)   \n\t"
                           "movq %%mm1, 24(%%edi)   \n\t"
                           "subl $8, %%esi          \n\t"
                           "subl $32, %%edi         \n\t"
                           "subl $2, %%ecx          \n\t"
                           "jnz .loop4_pass2        \n\t"
                           "EMMS                    \n\t" 

                           : "=c" (dummy_value_c),        
                             "=S" (dummy_value_S),
                             "=D" (dummy_value_D)

                           : "1" (sptr),      
                             "2" (dp),        
                             "0" (width_mmx)  

#if 0  /* %mm0, %mm1 not supported by gcc 2.7.2.3 or egcs 1.1 */
                           : "%mm0", "%mm1"               
#endif
                        );
                     }

                     sptr -= (width_mmx*4 - 4); 
                     dp -= (width_mmx*16 - 4);  
                     for (i = width; i; i--)
                     {
                        png_byte v[8];
                        int j;
                        sptr -= 4;
                        png_memcpy(v, sptr, 4);
                        for (j = 0; j < png_pass_inc[pass]; j++)
                        {
                           dp -= 4;
                           png_memcpy(dp, v, 4);
                        }
                     }
                  }
                  else if (width)  
                  {
                     int width_mmx = ((width >> 1) << 1) ;
                     width -= width_mmx;        
                     if (width_mmx)
                     {
                        int dummy_value_c;  
                        int dummy_value_S;
                        int dummy_value_D;

                        __asm__ __volatile__ (
                           "subl $4, %%esi          \n\t"
                           "subl $12, %%edi         \n\t"

                        ".loop4_pass4:              \n\t"
                           "movq (%%esi), %%mm0     \n\t" 
                           "movq %%mm0, %%mm1       \n\t" 
                           "punpckldq %%mm0, %%mm0  \n\t" 
                           "punpckhdq %%mm1, %%mm1  \n\t" 
                           "movq %%mm0, (%%edi)     \n\t"
                           "subl $8, %%esi          \n\t"
                           "movq %%mm1, 8(%%edi)    \n\t"
                           "subl $16, %%edi         \n\t"
                           "subl $2, %%ecx          \n\t"
                           "jnz .loop4_pass4        \n\t"
                           "EMMS                    \n\t" 

                           : "=c" (dummy_value_c),        
                             "=S" (dummy_value_S),
                             "=D" (dummy_value_D)

                           : "1" (sptr),      
                             "2" (dp),        
                             "0" (width_mmx)  

#if 0  /* %mm0, %mm1 not supported by gcc 2.7.2.3 or egcs 1.1 */
                           : "%mm0", "%mm1"               
#endif
                        );
                     }

                     sptr -= (width_mmx*4 - 4); 
                     dp -= (width_mmx*8 - 4);   
                     for (i = width; i; i--)
                     {
                        png_byte v[8];
                        int j;
                        sptr -= 4;
                        png_memcpy(v, sptr, 4);
                        for (j = 0; j < png_pass_inc[pass]; j++)
                        {
                           dp -= 4;
                           png_memcpy(dp, v, 4);
                        }
                     }
                  }
               } /* end of pixel_bytes == 4 */

               
               else if (pixel_bytes == 8)
               {

                  
                  if (((pass == 0) || (pass == 1)) && width)
                  {
                     int dummy_value_c;  
                     int dummy_value_S;
                     int dummy_value_D;

                     
                     
                     __asm__ __volatile__ (
                        "subl $56, %%edi         \n\t" 

                     ".loop8_pass0:              \n\t"
                        "movq (%%esi), %%mm0     \n\t" 
                        "movq %%mm0, (%%edi)     \n\t"
                        "movq %%mm0, 8(%%edi)    \n\t"
                        "movq %%mm0, 16(%%edi)   \n\t"
                        "movq %%mm0, 24(%%edi)   \n\t"
                        "movq %%mm0, 32(%%edi)   \n\t"
                        "movq %%mm0, 40(%%edi)   \n\t"
                        "movq %%mm0, 48(%%edi)   \n\t"
                        "subl $8, %%esi          \n\t"
                        "movq %%mm0, 56(%%edi)   \n\t"
                        "subl $64, %%edi         \n\t"
                        "decl %%ecx              \n\t"
                        "jnz .loop8_pass0        \n\t"
                        "EMMS                    \n\t" 

                        : "=c" (dummy_value_c),        
                          "=S" (dummy_value_S),
                          "=D" (dummy_value_D)

                        : "1" (sptr),      
                          "2" (dp),        
                          "0" (width)      

#if 0  /* %mm0 not supported by gcc 2.7.2.3 or egcs 1.1 */
                        : "%mm0"                       
#endif
                     );
                  }
                  else if (((pass == 2) || (pass == 3)) && width)
                  {
                     
                     
                     
                     
                     {
                        int dummy_value_c;  
                        int dummy_value_S;
                        int dummy_value_D;

                        __asm__ __volatile__ (
                           "subl $24, %%edi         \n\t" 

                        ".loop8_pass2:              \n\t"
                           "movq (%%esi), %%mm0     \n\t" 
                           "movq %%mm0, (%%edi)     \n\t"
                           "movq %%mm0, 8(%%edi)    \n\t"
                           "movq %%mm0, 16(%%edi)   \n\t"
                           "subl $8, %%esi          \n\t"
                           "movq %%mm0, 24(%%edi)   \n\t"
                           "subl $32, %%edi         \n\t"
                           "decl %%ecx              \n\t"
                           "jnz .loop8_pass2        \n\t"
                           "EMMS                    \n\t" 

                           : "=c" (dummy_value_c),        
                             "=S" (dummy_value_S),
                             "=D" (dummy_value_D)

                           : "1" (sptr),      
                             "2" (dp),        
                             "0" (width)      

#if 0  /* %mm0 not supported by gcc 2.7.2.3 or egcs 1.1 */
                           : "%mm0"                       
#endif
                        );
                     }
                  }
                  else if (width)  
                  {
                     
                     
                     {
                        int dummy_value_c;  
                        int dummy_value_S;
                        int dummy_value_D;

                        __asm__ __volatile__ (
                           "subl $8, %%edi          \n\t" 

                        ".loop8_pass4:              \n\t"
                           "movq (%%esi), %%mm0     \n\t" 
                           "movq %%mm0, (%%edi)     \n\t"
                           "subl $8, %%esi          \n\t"
                           "movq %%mm0, 8(%%edi)    \n\t"
                           "subl $16, %%edi         \n\t"
                           "decl %%ecx              \n\t"
                           "jnz .loop8_pass4        \n\t"
                           "EMMS                    \n\t" 

                           : "=c" (dummy_value_c),        
                             "=S" (dummy_value_S),
                             "=D" (dummy_value_D)

                           : "1" (sptr),      
                             "2" (dp),        
                             "0" (width)      

#if 0  /* %mm0 not supported by gcc 2.7.2.3 or egcs 1.1 */
                           : "%mm0"                       
#endif
                        );
                     }
                  }

               } /* end of pixel_bytes == 8 */

               
               else if (pixel_bytes == 6)
               {
                  for (i = width; i; i--)
                  {
                     png_byte v[8];
                     int j;
                     png_memcpy(v, sptr, 6);
                     for (j = 0; j < png_pass_inc[pass]; j++)
                     {
                        png_memcpy(dp, v, 6);
                        dp -= 6;
                     }
                     sptr -= 6;
                  }
               } /* end of pixel_bytes == 6 */

               
               else
               {
                  for (i = width; i; i--)
                  {
                     png_byte v[8];
                     int j;
                     png_memcpy(v, sptr, pixel_bytes);
                     for (j = 0; j < png_pass_inc[pass]; j++)
                     {
                        png_memcpy(dp, v, pixel_bytes);
                        dp -= pixel_bytes;
                     }
                     sptr-= pixel_bytes;
                  }
               }
            } 

            else /* MMX not supported:  use modified C code - takes advantage
                  *   of inlining of png_memcpy for a constant */
                 /* GRR 19991007:  does it?  or should pixel_bytes in each
                  *   block be replaced with immediate value (e.g., 1)? */
                 /* GRR 19991017:  replaced with constants in each case */
#endif /* PNG_ASSEMBLER_CODE_SUPPORTED */
            {
               if (pixel_bytes == 1)
               {
                  for (i = width; i; i--)
                  {
                     int j;
                     for (j = 0; j < png_pass_inc[pass]; j++)
                     {
                        *dp-- = *sptr;
                     }
                     --sptr;
                  }
               }
               else if (pixel_bytes == 3)
               {
                  for (i = width; i; i--)
                  {
                     png_byte v[8];
                     int j;
                     png_memcpy(v, sptr, 3);
                     for (j = 0; j < png_pass_inc[pass]; j++)
                     {
                        png_memcpy(dp, v, 3);
                        dp -= 3;
                     }
                     sptr -= 3;
                  }
               }
               else if (pixel_bytes == 2)
               {
                  for (i = width; i; i--)
                  {
                     png_byte v[8];
                     int j;
                     png_memcpy(v, sptr, 2);
                     for (j = 0; j < png_pass_inc[pass]; j++)
                     {
                        png_memcpy(dp, v, 2);
                        dp -= 2;
                     }
                     sptr -= 2;
                  }
               }
               else if (pixel_bytes == 4)
               {
                  for (i = width; i; i--)
                  {
                     png_byte v[8];
                     int j;
                     png_memcpy(v, sptr, 4);
                     for (j = 0; j < png_pass_inc[pass]; j++)
                     {
#ifdef PNG_DEBUG
                        if (dp < row || dp+3 > row+png_ptr->row_buf_size)
                        {
                           printf("dp out of bounds: row=%d, dp=%d, rp=%d\n",
                             row, dp, row+png_ptr->row_buf_size);
                           printf("row_buf=%d\n",png_ptr->row_buf_size);
                        }
#endif
                        png_memcpy(dp, v, 4);
                        dp -= 4;
                     }
                     sptr -= 4;
                  }
               }
               else if (pixel_bytes == 6)
               {
                  for (i = width; i; i--)
                  {
                     png_byte v[8];
                     int j;
                     png_memcpy(v, sptr, 6);
                     for (j = 0; j < png_pass_inc[pass]; j++)
                     {
                        png_memcpy(dp, v, 6);
                        dp -= 6;
                     }
                     sptr -= 6;
                  }
               }
               else if (pixel_bytes == 8)
               {
                  for (i = width; i; i--)
                  {
                     png_byte v[8];
                     int j;
                     png_memcpy(v, sptr, 8);
                     for (j = 0; j < png_pass_inc[pass]; j++)
                     {
                        png_memcpy(dp, v, 8);
                        dp -= 8;
                     }
                     sptr -= 8;
                  }
               }
               else     /* GRR:  should never be reached */
               {
                  for (i = width; i; i--)
                  {
                     png_byte v[8];
                     int j;
                     png_memcpy(v, sptr, pixel_bytes);
                     for (j = 0; j < png_pass_inc[pass]; j++)
                     {
                        png_memcpy(dp, v, pixel_bytes);
                        dp -= pixel_bytes;
                     }
                     sptr -= pixel_bytes;
                  }
               }

            } /* end if (MMX not supported) */
            break;
         }
      } /* end switch (row_info->pixel_depth) */

      row_info->width = final_width;

      row_info->rowbytes = PNG_ROWBYTES(row_info->pixel_depth,final_width);
   }

} /* end png_do_read_interlace() */

#endif /* PNG_HAVE_ASSEMBLER_READ_INTERLACE */
#endif /* PNG_READ_INTERLACING_SUPPORTED */



#if defined(PNG_HAVE_ASSEMBLER_READ_FILTER_ROW)
#if defined(PNG_ASSEMBLER_CODE_SUPPORTED)




union uAll {
   long long use;
   double  align;
} _LBCarryMask = {0x0101010101010101LL},
  _HBClearMask = {0x7f7f7f7f7f7f7f7fLL},
  _ActiveMask, _ActiveMask2, _ActiveMaskEnd, _ShiftBpp, _ShiftRem;

#ifdef PNG_THREAD_UNSAFE_OK








static void /* PRIVATE */
png_read_filter_row_mmx_avg(png_row_infop row_info, png_bytep row,
                            png_bytep prev_row)
{
   int bpp;
   int dummy_value_c;   
   int dummy_value_S;
   int dummy_value_D;

   bpp = (row_info->pixel_depth + 7) >> 3;  
   _FullLength  = row_info->rowbytes;       

   __asm__ __volatile__ (
      
#ifdef __PIC__
      "pushl %%ebx                 \n\t" 
#endif

      "xorl %%ebx, %%ebx           \n\t" 
      "movl %%edi, %%edx           \n\t"


      "subl %%ecx, %%edx           \n\t" 

      "xorl %%eax,%%eax            \n\t"

      
      
   "avg_rlp:                       \n\t"
      "movb (%%esi,%%ebx,),%%al    \n\t" 
      "incl %%ebx                  \n\t"
      "shrb %%al                   \n\t" 
      "addb -1(%%edi,%%ebx,),%%al  \n\t" 

      "cmpl %%ecx, %%ebx           \n\t"
      "movb %%al,-1(%%edi,%%ebx,)  \n\t" 
      "jb avg_rlp                  \n\t" 

      
      "movl %%edi, _dif            \n\t" 
      "addl %%ebx, _dif            \n\t" 
      "addl $0xf, _dif             \n\t" 
      "andl $0xfffffff8, _dif      \n\t" 
      "subl %%edi, _dif            \n\t" 
      "jz avg_go                   \n\t" 

      
      
      
      "xorl %%ecx, %%ecx           \n\t"

   "avg_lp1:                       \n\t"
      "xorl %%eax, %%eax           \n\t"
      "movb (%%esi,%%ebx,), %%cl   \n\t" 
      "movb (%%edx,%%ebx,), %%al   \n\t" 
      "addw %%cx, %%ax             \n\t"
      "incl %%ebx                  \n\t"
      "shrw %%ax                   \n\t" 
      "addb -1(%%edi,%%ebx,), %%al \n\t" 
      "cmpl _dif, %%ebx            \n\t" 
      "movb %%al, -1(%%edi,%%ebx,) \n\t" 
      "jb avg_lp1                  \n\t" 

   "avg_go:                        \n\t"
      "movl _FullLength, %%eax     \n\t"
      "movl %%eax, %%ecx           \n\t"
      "subl %%ebx, %%eax           \n\t" 
      "andl $0x00000007, %%eax     \n\t" 
      "subl %%eax, %%ecx           \n\t" 
      "movl %%ecx, _MMXLength      \n\t"
#ifdef __PIC__
      "popl %%ebx                  \n\t" 
#endif

      : "=c" (dummy_value_c),            
        "=S" (dummy_value_S),
        "=D" (dummy_value_D)

      : "0" (bpp),       
        "1" (prev_row),  
        "2" (row)        

      : "%eax", "%edx"                   
#ifndef __PIC__
      , "%ebx"
#endif
      
      
   );

   
   switch (bpp)
   {
      case 3:
      {
         _ActiveMask.use  = 0x0000000000ffffffLL;
         _ShiftBpp.use = 24;    
         _ShiftRem.use = 40;    

         __asm__ __volatile__ (
            
            "movq _ActiveMask, %%mm7      \n\t"
            "movl _dif, %%ecx             \n\t" 
            "movq _LBCarryMask, %%mm5     \n\t" 

            "movq _HBClearMask, %%mm4     \n\t"


            
            "movq -8(%%edi,%%ecx,), %%mm2 \n\t" 
                                                
         "avg_3lp:                        \n\t"
            "movq (%%edi,%%ecx,), %%mm0   \n\t" 
            "movq %%mm5, %%mm3            \n\t"
            "psrlq _ShiftRem, %%mm2       \n\t" 
                                                
            "movq (%%esi,%%ecx,), %%mm1   \n\t" 
            "movq %%mm7, %%mm6            \n\t"
            "pand %%mm1, %%mm3            \n\t" 
            "psrlq $1, %%mm1              \n\t" 
            "pand  %%mm4, %%mm1           \n\t" 
                                                
            "paddb %%mm1, %%mm0           \n\t" 
                                                
            
            "movq %%mm3, %%mm1            \n\t" 
                                                
            "pand %%mm2, %%mm1            \n\t" 
                                                
                               
            "psrlq $1, %%mm2              \n\t" 
            "pand  %%mm4, %%mm2           \n\t" 
                                                
            "paddb %%mm1, %%mm2           \n\t" 
                                                
            "pand %%mm6, %%mm2            \n\t" 
                                                
            "paddb %%mm2, %%mm0           \n\t" 
                                                
                               
            
            "psllq _ShiftBpp, %%mm6       \n\t" 
                                                
            "movq %%mm0, %%mm2            \n\t" 
            "psllq _ShiftBpp, %%mm2       \n\t" 
            "movq %%mm3, %%mm1            \n\t" 
                                                
            "pand %%mm2, %%mm1            \n\t" 
                                                
                               
            "psrlq $1, %%mm2              \n\t" 
            "pand  %%mm4, %%mm2           \n\t" 
                                                
            "paddb %%mm1, %%mm2           \n\t" 
                                                
            "pand %%mm6, %%mm2            \n\t" 
                                                
            "paddb %%mm2, %%mm0           \n\t" 
                                                
                               

            
            "psllq _ShiftBpp, %%mm6       \n\t" 
                                                
                                 
            "movq %%mm0, %%mm2            \n\t" 
            "psllq _ShiftBpp, %%mm2       \n\t" 
                              
                              
            "movq %%mm3, %%mm1            \n\t" 
                                                
            "pand %%mm2, %%mm1            \n\t" 
                                                
                              
            "psrlq $1, %%mm2              \n\t" 
            "pand  %%mm4, %%mm2           \n\t" 
                                                
            "paddb %%mm1, %%mm2           \n\t" 
                                                
            "pand %%mm6, %%mm2            \n\t" 
                                                
            "addl $8, %%ecx               \n\t"
            "paddb %%mm2, %%mm0           \n\t" 
                                                
                                                
            
            "movq %%mm0, -8(%%edi,%%ecx,) \n\t"
            
            "cmpl _MMXLength, %%ecx       \n\t"
            "movq %%mm0, %%mm2            \n\t" 
            "jb avg_3lp                   \n\t"

            : "=S" (dummy_value_S),             
              "=D" (dummy_value_D)

            : "0" (prev_row),  
              "1" (row)        

            : "%ecx"                            
#if 0  /* %mm0, ..., %mm7 not supported by gcc 2.7.2.3 or egcs 1.1 */
            , "%mm0", "%mm1", "%mm2", "%mm3"
            , "%mm4", "%mm5", "%mm6", "%mm7"
#endif
         );
      }
      break;  

      case 6:
      case 4:
      
      
      {
         _ActiveMask.use  = 0xffffffffffffffffLL; 
                                                  
         _ShiftBpp.use = bpp << 3;
         _ShiftRem.use = 64 - _ShiftBpp.use;

         __asm__ __volatile__ (
            "movq _HBClearMask, %%mm4    \n\t"

            
            "movl _dif, %%ecx            \n\t" 
                                               

            
            "movq _ActiveMask, %%mm7     \n\t"

            "psrlq _ShiftRem, %%mm7      \n\t"

            "movq %%mm7, %%mm6           \n\t"
            "movq _LBCarryMask, %%mm5    \n\t"
            "psllq _ShiftBpp, %%mm6      \n\t" 
                                               

            
            "movq -8(%%edi,%%ecx,), %%mm2 \n\t" 
                                          
         "avg_4lp:                       \n\t"
            "movq (%%edi,%%ecx,), %%mm0  \n\t"
            "psrlq _ShiftRem, %%mm2      \n\t" 
            "movq (%%esi,%%ecx,), %%mm1  \n\t"
            
            "movq %%mm5, %%mm3           \n\t"
            "pand %%mm1, %%mm3           \n\t" 
            "psrlq $1, %%mm1             \n\t" 
            "pand  %%mm4, %%mm1          \n\t" 
                                               
            "paddb %%mm1, %%mm0          \n\t" 
                                               
            
            "movq %%mm3, %%mm1           \n\t" 
                                               
            "pand %%mm2, %%mm1           \n\t" 
                                               
                              
            "psrlq $1, %%mm2             \n\t" 
            "pand  %%mm4, %%mm2          \n\t" 
                                               
            "paddb %%mm1, %%mm2          \n\t" 
                                               
            "pand %%mm7, %%mm2           \n\t" 
                                               
            "paddb %%mm2, %%mm0          \n\t" 
                                               
                              
            
            "movq %%mm0, %%mm2           \n\t" 
            "psllq _ShiftBpp, %%mm2      \n\t" 
            "addl $8, %%ecx              \n\t"
            "movq %%mm3, %%mm1           \n\t" 
                                               
            "pand %%mm2, %%mm1           \n\t" 
                                               
                              
            "psrlq $1, %%mm2             \n\t" 
            "pand  %%mm4, %%mm2          \n\t" 
                                               
            "paddb %%mm1, %%mm2          \n\t" 
                                               
            "pand %%mm6, %%mm2           \n\t" 
                                               
            "paddb %%mm2, %%mm0          \n\t" 
                                               
                              
            "cmpl _MMXLength, %%ecx      \n\t"
            
            "movq %%mm0, -8(%%edi,%%ecx,) \n\t"
            
            "movq %%mm0, %%mm2           \n\t" 
            "jb avg_4lp                  \n\t"

            : "=S" (dummy_value_S),            
              "=D" (dummy_value_D)

            : "0" (prev_row),  
              "1" (row)        

            : "%ecx"                           
#if 0  /* %mm0, ..., %mm7 not supported by gcc 2.7.2.3 or egcs 1.1 */
            , "%mm0", "%mm1", "%mm2", "%mm3"
            , "%mm4", "%mm5", "%mm6", "%mm7"
#endif
         );
      }
      break;  

      case 2:
      {
         _ActiveMask.use  = 0x000000000000ffffLL;
         _ShiftBpp.use = 16;   
         _ShiftRem.use = 48;   

         __asm__ __volatile__ (
            
            "movq _ActiveMask, %%mm7     \n\t"
            
            "movl _dif, %%ecx            \n\t" 
                                               
            "movq _LBCarryMask, %%mm5    \n\t"

            "movq _HBClearMask, %%mm4    \n\t"


            
            "movq -8(%%edi,%%ecx,), %%mm2 \n\t" 
                              
         "avg_2lp:                       \n\t"
            "movq (%%edi,%%ecx,), %%mm0  \n\t"
            "psrlq _ShiftRem, %%mm2      \n\t" 
            "movq (%%esi,%%ecx,), %%mm1  \n\t" 
            
            "movq %%mm5, %%mm3           \n\t"
            "pand %%mm1, %%mm3           \n\t" 
            "psrlq $1, %%mm1             \n\t" 
            "pand  %%mm4, %%mm1          \n\t" 
                                               
            "movq %%mm7, %%mm6           \n\t"
            "paddb %%mm1, %%mm0          \n\t" 
                                               

            
            "movq %%mm3, %%mm1           \n\t" 
                                               
            "pand %%mm2, %%mm1           \n\t" 
                                               
                                               
                                               
            "psrlq $1, %%mm2             \n\t" 
            "pand  %%mm4, %%mm2          \n\t" 
                                               
            "paddb %%mm1, %%mm2          \n\t" 
                                               
            "pand %%mm6, %%mm2           \n\t" 
                                               
            "paddb %%mm2, %%mm0          \n\t" 
                                               

            
            "psllq _ShiftBpp, %%mm6      \n\t" 
                                               
            "movq %%mm0, %%mm2           \n\t" 
            "psllq _ShiftBpp, %%mm2      \n\t" 
            "movq %%mm3, %%mm1           \n\t" 
                                               
            "pand %%mm2, %%mm1           \n\t" 
                                               
                                               
                                               
            "psrlq $1, %%mm2             \n\t" 
            "pand  %%mm4, %%mm2          \n\t" 
                                               
            "paddb %%mm1, %%mm2          \n\t" 
                                               
            "pand %%mm6, %%mm2           \n\t" 
                                               
            "paddb %%mm2, %%mm0          \n\t" 
                                               

            
            "psllq _ShiftBpp, %%mm6      \n\t" 
                                               
            "movq %%mm0, %%mm2           \n\t" 
            "psllq _ShiftBpp, %%mm2      \n\t" 
            "movq %%mm3, %%mm1           \n\t" 
                                               
            "pand %%mm2, %%mm1           \n\t" 
                                               
                                               
            "psrlq $1, %%mm2             \n\t" 
            "pand  %%mm4, %%mm2          \n\t" 
                                               
            "paddb %%mm1, %%mm2          \n\t" 
                                               
            "pand %%mm6, %%mm2           \n\t" 
                                               
            "paddb %%mm2, %%mm0          \n\t" 
                                               

            
            "psllq _ShiftBpp, %%mm6      \n\t" 
                                               
            "movq %%mm0, %%mm2           \n\t" 
            "psllq _ShiftBpp, %%mm2      \n\t" 
            "addl $8, %%ecx              \n\t"
            "movq %%mm3, %%mm1           \n\t" 
                                               
            "pand %%mm2, %%mm1           \n\t" 
                                               
                                               
                                               
            "psrlq $1, %%mm2             \n\t" 
            "pand  %%mm4, %%mm2          \n\t" 
                                               
            "paddb %%mm1, %%mm2          \n\t" 
                                               
            "pand %%mm6, %%mm2           \n\t" 
                                               
            "paddb %%mm2, %%mm0          \n\t" 
                                               

            "cmpl _MMXLength, %%ecx      \n\t"
            
            "movq %%mm0, -8(%%edi,%%ecx,) \n\t"
            
            "movq %%mm0, %%mm2           \n\t" 
            "jb avg_2lp                  \n\t"

            : "=S" (dummy_value_S),            
              "=D" (dummy_value_D)

            : "0" (prev_row),  
              "1" (row)        

            : "%ecx"                           
#if 0  /* %mm0, ..., %mm7 not supported by gcc 2.7.2.3 or egcs 1.1 */
            , "%mm0", "%mm1", "%mm2", "%mm3"
            , "%mm4", "%mm5", "%mm6", "%mm7"
#endif
         );
      }
      break;  

      case 1:
      {
         __asm__ __volatile__ (
            
#ifdef __PIC__
            "pushl %%ebx                 \n\t" 
#endif
            "movl _dif, %%ebx            \n\t" 
                                               

            "cmpl _FullLength, %%ebx     \n\t" 
            "jnb avg_1end                \n\t"
            

            "movl %%edi, %%edx           \n\t"

            "subl %%ecx, %%edx           \n\t" 
            "xorl %%ecx, %%ecx           \n\t" 
                                               
         "avg_1lp:                       \n\t"
            
            "xorl %%eax, %%eax           \n\t"
            "movb (%%esi,%%ebx,), %%cl   \n\t" 
            "movb (%%edx,%%ebx,), %%al   \n\t" 
            "addw %%cx, %%ax             \n\t"
            "incl %%ebx                  \n\t"
            "shrw %%ax                   \n\t" 
            "addb -1(%%edi,%%ebx,), %%al \n\t" 
                                               
            "cmpl _FullLength, %%ebx     \n\t" 
            "movb %%al, -1(%%edi,%%ebx,) \n\t" 
                         
            "jb avg_1lp                  \n\t"

         "avg_1end:                      \n\t"
#ifdef __PIC__
            "popl %%ebx                  \n\t" 
#endif

            : "=c" (dummy_value_c),            
              "=S" (dummy_value_S),
              "=D" (dummy_value_D)

            : "0" (bpp),       
              "1" (prev_row),  
              "2" (row)        

            : "%eax", "%edx"                   
#ifndef __PIC__
            , "%ebx"
#endif
         );
      }
      return;  

      case 8:
      {
         __asm__ __volatile__ (
            
            "movl _dif, %%ecx            \n\t" 
            "movq _LBCarryMask, %%mm5    \n\t" 

            "movq _HBClearMask, %%mm4    \n\t"


            
            "movq -8(%%edi,%%ecx,), %%mm2 \n\t" 
                                      

         "avg_8lp:                       \n\t"
            "movq (%%edi,%%ecx,), %%mm0  \n\t"
            "movq %%mm5, %%mm3           \n\t"
            "movq (%%esi,%%ecx,), %%mm1  \n\t"
            "addl $8, %%ecx              \n\t"
            "pand %%mm1, %%mm3           \n\t" 
            "psrlq $1, %%mm1             \n\t" 
            "pand %%mm2, %%mm3           \n\t" 
                                               
            "psrlq $1, %%mm2             \n\t" 
            "pand  %%mm4, %%mm1          \n\t" 
            "paddb %%mm3, %%mm0          \n\t" 
            "pand  %%mm4, %%mm2          \n\t" 
            "paddb %%mm1, %%mm0          \n\t" 
            "paddb %%mm2, %%mm0          \n\t" 
            "cmpl _MMXLength, %%ecx      \n\t"
            "movq %%mm0, -8(%%edi,%%ecx,) \n\t"
            "movq %%mm0, %%mm2           \n\t" 
            "jb avg_8lp                  \n\t"

            : "=S" (dummy_value_S),            
              "=D" (dummy_value_D)

            : "0" (prev_row),  
              "1" (row)        

            : "%ecx"                           
#if 0  /* %mm0, ..., %mm5 not supported by gcc 2.7.2.3 or egcs 1.1 */
            , "%mm0", "%mm1", "%mm2"
            , "%mm3", "%mm4", "%mm5"
#endif
         );
      }
      break;  

      default:                  
      {

#ifdef PNG_DEBUG
         
        png_debug(1,
        "Internal logic error in pnggccrd (png_read_filter_row_mmx_avg())\n");
#endif

#if 0
        __asm__ __volatile__ (
            "movq _LBCarryMask, %%mm5    \n\t"
            
            "movl _dif, %%ebx            \n\t" 
                                               
            "movl row, %%edi             \n\t" 
            "movq _HBClearMask, %%mm4    \n\t"
            "movl %%edi, %%edx           \n\t"
            "movl prev_row, %%esi        \n\t" 
            "subl bpp, %%edx             \n\t" 
         "avg_Alp:                       \n\t"
            "movq (%%edi,%%ebx,), %%mm0  \n\t"
            "movq %%mm5, %%mm3           \n\t"
            "movq (%%esi,%%ebx,), %%mm1  \n\t"
            "pand %%mm1, %%mm3           \n\t" 
            "movq (%%edx,%%ebx,), %%mm2  \n\t"
            "psrlq $1, %%mm1             \n\t" 
            "pand %%mm2, %%mm3           \n\t" 
                                               
            "psrlq $1, %%mm2             \n\t" 
            "pand  %%mm4, %%mm1          \n\t" 
                                               
            "paddb %%mm3, %%mm0          \n\t" 
                                               
            "pand  %%mm4, %%mm2          \n\t" 
                                               
            "paddb %%mm1, %%mm0          \n\t" 
                                               
            "addl $8, %%ebx              \n\t"
            "paddb %%mm2, %%mm0          \n\t" 
                                               
            "cmpl _MMXLength, %%ebx      \n\t"
            "movq %%mm0, -8(%%edi,%%ebx,) \n\t"
            "jb avg_Alp                  \n\t"

            : 

            : 

            : "%ebx", "%edx", "%edi", "%esi" 
         );
#endif /* 0 - NEVER REACHED */
      }
      break;

   } 

   __asm__ __volatile__ (
      
      
#ifdef __PIC__
      "pushl %%ebx                 \n\t" 
#endif
      "movl _MMXLength, %%ebx      \n\t" 

      "cmpl _FullLength, %%ebx     \n\t" 
      "jnb avg_end                 \n\t"

      

      "movl %%edi, %%edx           \n\t"

      "subl %%ecx, %%edx           \n\t" 
      "xorl %%ecx, %%ecx           \n\t" 

   "avg_lp2:                       \n\t"
      
      "xorl %%eax, %%eax           \n\t"
      "movb (%%esi,%%ebx,), %%cl   \n\t" 
      "movb (%%edx,%%ebx,), %%al   \n\t" 
      "addw %%cx, %%ax             \n\t"
      "incl %%ebx                  \n\t"
      "shrw %%ax                   \n\t" 
      "addb -1(%%edi,%%ebx,), %%al \n\t" 
      "cmpl _FullLength, %%ebx     \n\t" 
      "movb %%al, -1(%%edi,%%ebx,) \n\t" 
      "jb avg_lp2                  \n\t" 

   "avg_end:                       \n\t"
      "EMMS                        \n\t" 
#ifdef __PIC__
      "popl %%ebx                  \n\t" 
#endif

      : "=c" (dummy_value_c),            
        "=S" (dummy_value_S),
        "=D" (dummy_value_D)

      : "0" (bpp),       
        "1" (prev_row),  
        "2" (row)        

      : "%eax", "%edx"                   
#ifndef __PIC__
      , "%ebx"
#endif
   );

} /* end png_read_filter_row_mmx_avg() */
#endif



#ifdef PNG_THREAD_UNSAFE_OK








static void /* PRIVATE */
png_read_filter_row_mmx_paeth(png_row_infop row_info, png_bytep row,
                              png_bytep prev_row)
{
   int bpp;
   int dummy_value_c;   
   int dummy_value_S;
   int dummy_value_D;

   bpp = (row_info->pixel_depth + 7) >> 3; 
   _FullLength  = row_info->rowbytes; 

   __asm__ __volatile__ (
#ifdef __PIC__
      "pushl %%ebx                 \n\t" 
#endif
      "xorl %%ebx, %%ebx           \n\t" 

      "xorl %%edx, %%edx           \n\t" 

      "xorl %%eax, %%eax           \n\t"

      
      
      
   "paeth_rlp:                     \n\t"
      "movb (%%edi,%%ebx,), %%al   \n\t"
      "addb (%%esi,%%ebx,), %%al   \n\t"
      "incl %%ebx                  \n\t"

      "cmpl %%ecx, %%ebx           \n\t"
      "movb %%al, -1(%%edi,%%ebx,) \n\t"
      "jb paeth_rlp                \n\t"
      
      "movl %%edi, _dif            \n\t" 
      "addl %%ebx, _dif            \n\t" 
      "xorl %%ecx, %%ecx           \n\t"
      "addl $0xf, _dif             \n\t" 
                                         
      "andl $0xfffffff8, _dif      \n\t" 
      "subl %%edi, _dif            \n\t" 
                                         
      "jz paeth_go                 \n\t"
      

   "paeth_lp1:                     \n\t"
      "xorl %%eax, %%eax           \n\t"
      
      "movb (%%esi,%%ebx,), %%al   \n\t" 
      "movb (%%esi,%%edx,), %%cl   \n\t" 
      "subl %%ecx, %%eax           \n\t" 
      "movl %%eax, _patemp         \n\t" 
      "xorl %%eax, %%eax           \n\t"
      
      "movb (%%edi,%%edx,), %%al   \n\t" 
      "subl %%ecx, %%eax           \n\t" 
      "movl %%eax, %%ecx           \n\t"
      
      "addl _patemp, %%eax         \n\t" 
      
      "testl $0x80000000, %%eax    \n\t"
      "jz paeth_pca                \n\t"
      "negl %%eax                  \n\t" 

   "paeth_pca:                     \n\t"
      "movl %%eax, _pctemp         \n\t" 
      
      "testl $0x80000000, %%ecx    \n\t"
      "jz paeth_pba                \n\t"
      "negl %%ecx                  \n\t" 

   "paeth_pba:                     \n\t"
      "movl %%ecx, _pbtemp         \n\t" 
      
      "movl _patemp, %%eax         \n\t"
      "testl $0x80000000, %%eax    \n\t"
      "jz paeth_paa                \n\t"
      "negl %%eax                  \n\t" 

   "paeth_paa:                     \n\t"
      "movl %%eax, _patemp         \n\t" 
      
      "cmpl %%ecx, %%eax           \n\t"
      "jna paeth_abb               \n\t"
      
      "cmpl _pctemp, %%ecx         \n\t"
      "jna paeth_bbc               \n\t"
      
      "movb (%%esi,%%edx,), %%cl   \n\t" 
      "jmp paeth_paeth             \n\t"

   "paeth_bbc:                     \n\t"
      
      "movb (%%esi,%%ebx,), %%cl   \n\t" 
      "jmp paeth_paeth             \n\t"

   "paeth_abb:                     \n\t"
      
      "cmpl _pctemp, %%eax         \n\t"
      "jna paeth_abc               \n\t"
      
      "movb (%%esi,%%edx,), %%cl   \n\t" 
      "jmp paeth_paeth             \n\t"

   "paeth_abc:                     \n\t"
      
      "movb (%%edi,%%edx,), %%cl   \n\t" 

   "paeth_paeth:                   \n\t"
      "incl %%ebx                  \n\t"
      "incl %%edx                  \n\t"
      
      "addb %%cl, -1(%%edi,%%ebx,) \n\t"
      "cmpl _dif, %%ebx            \n\t"
      "jb paeth_lp1                \n\t"

   "paeth_go:                      \n\t"
      "movl _FullLength, %%ecx     \n\t"
      "movl %%ecx, %%eax           \n\t"
      "subl %%ebx, %%eax           \n\t" 
      "andl $0x00000007, %%eax     \n\t" 
      "subl %%eax, %%ecx           \n\t" 
      "movl %%ecx, _MMXLength      \n\t"
#ifdef __PIC__
      "popl %%ebx                  \n\t" 
#endif

      : "=c" (dummy_value_c),            
        "=S" (dummy_value_S),
        "=D" (dummy_value_D)

      : "0" (bpp),       
        "1" (prev_row),  
        "2" (row)        

      : "%eax", "%edx"                   
#ifndef __PIC__
      , "%ebx"
#endif
   );

   
   switch (bpp)
   {
      case 3:
      {
         _ActiveMask.use = 0x0000000000ffffffLL;
         _ActiveMaskEnd.use = 0xffff000000000000LL;
         _ShiftBpp.use = 24;    
         _ShiftRem.use = 40;    

         __asm__ __volatile__ (
            "movl _dif, %%ecx            \n\t"


            "pxor %%mm0, %%mm0           \n\t"
            
            "movq -8(%%edi,%%ecx,), %%mm1 \n\t"
         "paeth_3lp:                     \n\t"
            "psrlq _ShiftRem, %%mm1      \n\t" 
                                               
            "movq (%%esi,%%ecx,), %%mm2  \n\t" 
            "punpcklbw %%mm0, %%mm1      \n\t" 
            "movq -8(%%esi,%%ecx,), %%mm3 \n\t" 
            "punpcklbw %%mm0, %%mm2      \n\t" 
            "psrlq _ShiftRem, %%mm3      \n\t" 
                                               
            
            "movq %%mm2, %%mm4           \n\t"
            "punpcklbw %%mm0, %%mm3      \n\t" 
            
            "movq %%mm1, %%mm5           \n\t"
            "psubw %%mm3, %%mm4          \n\t"
            "pxor %%mm7, %%mm7           \n\t"
            
            "movq %%mm4, %%mm6           \n\t"
            "psubw %%mm3, %%mm5          \n\t"

            
            
            
            "pcmpgtw %%mm4, %%mm0        \n\t" 
            "paddw %%mm5, %%mm6          \n\t"
            "pand %%mm4, %%mm0           \n\t" 
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "pand %%mm5, %%mm7           \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "psubw %%mm7, %%mm5          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "pcmpgtw %%mm6, %%mm0        \n\t" 
            "pand %%mm6, %%mm0           \n\t" 
            "psubw %%mm7, %%mm5          \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            
            "movq %%mm4, %%mm7           \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "movq %%mm7, %%mm0           \n\t"
            
            "pand %%mm7, %%mm5           \n\t"
            
            "pand %%mm0, %%mm2           \n\t"
            "pandn %%mm4, %%mm7          \n\t"
            "pandn %%mm1, %%mm0          \n\t"
            "paddw %%mm5, %%mm7          \n\t"
            "paddw %%mm2, %%mm0          \n\t"
            
            "pcmpgtw %%mm6, %%mm7        \n\t" 
            "pxor %%mm1, %%mm1           \n\t"
            "pand %%mm7, %%mm3           \n\t"
            "pandn %%mm0, %%mm7          \n\t"
            "paddw %%mm3, %%mm7          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "packuswb %%mm1, %%mm7       \n\t"
            "movq (%%esi,%%ecx,), %%mm3  \n\t" 
            "pand _ActiveMask, %%mm7     \n\t"
            "movq %%mm3, %%mm2           \n\t" 
            "paddb (%%edi,%%ecx,), %%mm7 \n\t" 
            "punpcklbw %%mm0, %%mm3      \n\t" 
            "movq %%mm7, (%%edi,%%ecx,)  \n\t" 
            "movq %%mm7, %%mm1           \n\t" 
                                               
            
            "psrlq _ShiftBpp, %%mm2      \n\t" 
            "punpcklbw %%mm0, %%mm1      \n\t" 
            "pxor %%mm7, %%mm7           \n\t"
            "punpcklbw %%mm0, %%mm2      \n\t" 
            
            "movq %%mm1, %%mm5           \n\t"
            
            "movq %%mm2, %%mm4           \n\t"
            "psubw %%mm3, %%mm5          \n\t"
            "psubw %%mm3, %%mm4          \n\t"
            
            
            "movq %%mm5, %%mm6           \n\t"
            "paddw %%mm4, %%mm6          \n\t"

            
            
            
            "pcmpgtw %%mm5, %%mm0        \n\t" 
            "pcmpgtw %%mm4, %%mm7        \n\t" 
            "pand %%mm5, %%mm0           \n\t" 
            "pand %%mm4, %%mm7           \n\t" 
            "psubw %%mm0, %%mm5          \n\t"
            "psubw %%mm7, %%mm4          \n\t"
            "psubw %%mm0, %%mm5          \n\t"
            "psubw %%mm7, %%mm4          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "pcmpgtw %%mm6, %%mm0        \n\t" 
            "pand %%mm6, %%mm0           \n\t" 
            "psubw %%mm0, %%mm6          \n\t"
            
            "movq %%mm4, %%mm7           \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "movq %%mm7, %%mm0           \n\t"
            
            "pand %%mm7, %%mm5           \n\t"
            
            "pand %%mm0, %%mm2           \n\t"
            "pandn %%mm4, %%mm7          \n\t"
            "pandn %%mm1, %%mm0          \n\t"
            "paddw %%mm5, %%mm7          \n\t"
            "paddw %%mm2, %%mm0          \n\t"
            
            "pcmpgtw %%mm6, %%mm7        \n\t" 
            "movq (%%esi,%%ecx,), %%mm2  \n\t" 
            "pand %%mm7, %%mm3           \n\t"
            "pandn %%mm0, %%mm7          \n\t"
            "pxor %%mm1, %%mm1           \n\t"
            "paddw %%mm3, %%mm7          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "packuswb %%mm1, %%mm7       \n\t"
            "movq %%mm2, %%mm3           \n\t" 
            "pand _ActiveMask, %%mm7     \n\t"
            "punpckhbw %%mm0, %%mm2      \n\t" 
            "psllq _ShiftBpp, %%mm7      \n\t" 
                                               
             
            "movq %%mm2, %%mm4           \n\t"
            "paddb (%%edi,%%ecx,), %%mm7 \n\t" 
            "psllq _ShiftBpp, %%mm3      \n\t" 
            "movq %%mm7, (%%edi,%%ecx,)  \n\t" 
            "movq %%mm7, %%mm1           \n\t"
            "punpckhbw %%mm0, %%mm3      \n\t" 
            "psllq _ShiftBpp, %%mm1      \n\t" 
                                    
            
            "pxor %%mm7, %%mm7           \n\t"
            "punpckhbw %%mm0, %%mm1      \n\t" 
            "psubw %%mm3, %%mm4          \n\t"
            
            "movq %%mm1, %%mm5           \n\t"
            
            "movq %%mm4, %%mm6           \n\t"
            "psubw %%mm3, %%mm5          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "paddw %%mm5, %%mm6          \n\t"

            
            
            
            "pcmpgtw %%mm4, %%mm0        \n\t" 
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "pand %%mm4, %%mm0           \n\t" 
            "pand %%mm5, %%mm7           \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "psubw %%mm7, %%mm5          \n\t"
            "psubw %%mm0, %%mm4          \n\t"
            "psubw %%mm7, %%mm5          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "pcmpgtw %%mm6, %%mm0        \n\t" 
            "pand %%mm6, %%mm0           \n\t" 
            "psubw %%mm0, %%mm6          \n\t"
            
            "movq %%mm4, %%mm7           \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "movq %%mm7, %%mm0           \n\t"
            
            "pand %%mm0, %%mm2           \n\t"
            
            "pand %%mm7, %%mm5           \n\t"
            "pandn %%mm1, %%mm0          \n\t"
            "pandn %%mm4, %%mm7          \n\t"
            "paddw %%mm2, %%mm0          \n\t"
            "paddw %%mm5, %%mm7          \n\t"
            
            "pcmpgtw %%mm6, %%mm7        \n\t" 
            "pand %%mm7, %%mm3           \n\t"
            "pandn %%mm0, %%mm7          \n\t"
            "paddw %%mm3, %%mm7          \n\t"
            "pxor %%mm1, %%mm1           \n\t"
            "packuswb %%mm7, %%mm1       \n\t"
            
            "addl $8, %%ecx              \n\t"
            "pand _ActiveMaskEnd, %%mm1  \n\t"
            "paddb -8(%%edi,%%ecx,), %%mm1 \n\t" 
                                                 

            "cmpl _MMXLength, %%ecx      \n\t"
            "pxor %%mm0, %%mm0           \n\t" 
            "movq %%mm1, -8(%%edi,%%ecx,) \n\t" 
                                 
                           
            "jb paeth_3lp                \n\t"

            : "=S" (dummy_value_S),             
              "=D" (dummy_value_D)

            : "0" (prev_row),  
              "1" (row)        

            : "%ecx"                            
#if 0  /* %mm0, ..., %mm7 not supported by gcc 2.7.2.3 or egcs 1.1 */
            , "%mm0", "%mm1", "%mm2", "%mm3"
            , "%mm4", "%mm5", "%mm6", "%mm7"
#endif
         );
      }
      break;  

      case 6:
      
      
      {
         _ActiveMask.use  = 0x00000000ffffffffLL;
         _ActiveMask2.use = 0xffffffff00000000LL;
         _ShiftBpp.use = bpp << 3;    
         _ShiftRem.use = 64 - _ShiftBpp.use;

         __asm__ __volatile__ (
            "movl _dif, %%ecx            \n\t"


            
            "movq -8(%%edi,%%ecx,), %%mm1 \n\t"
            "pxor %%mm0, %%mm0           \n\t"

         "paeth_6lp:                     \n\t"
            
            "psrlq _ShiftRem, %%mm1      \n\t"
            
            "movq -8(%%esi,%%ecx,), %%mm3 \n\t" 
            "punpcklbw %%mm0, %%mm1      \n\t" 
            "movq (%%esi,%%ecx,), %%mm2  \n\t" 
            "punpcklbw %%mm0, %%mm2      \n\t" 
            
            "psrlq _ShiftRem, %%mm3      \n\t"
            
            "movq %%mm2, %%mm4           \n\t"
            "punpcklbw %%mm0, %%mm3      \n\t" 
            
            "movq %%mm1, %%mm5           \n\t"
            "psubw %%mm3, %%mm4          \n\t"
            "pxor %%mm7, %%mm7           \n\t"
            
            "movq %%mm4, %%mm6           \n\t"
            "psubw %%mm3, %%mm5          \n\t"
            
            
            
            "pcmpgtw %%mm4, %%mm0        \n\t" 
            "paddw %%mm5, %%mm6          \n\t"
            "pand %%mm4, %%mm0           \n\t" 
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "pand %%mm5, %%mm7           \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "psubw %%mm7, %%mm5          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "pcmpgtw %%mm6, %%mm0        \n\t" 
            "pand %%mm6, %%mm0           \n\t" 
            "psubw %%mm7, %%mm5          \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            
            "movq %%mm4, %%mm7           \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "movq %%mm7, %%mm0           \n\t"
            
            "pand %%mm7, %%mm5           \n\t"
            
            "pand %%mm0, %%mm2           \n\t"
            "pandn %%mm4, %%mm7          \n\t"
            "pandn %%mm1, %%mm0          \n\t"
            "paddw %%mm5, %%mm7          \n\t"
            "paddw %%mm2, %%mm0          \n\t"
            
            "pcmpgtw %%mm6, %%mm7        \n\t" 
            "pxor %%mm1, %%mm1           \n\t"
            "pand %%mm7, %%mm3           \n\t"
            "pandn %%mm0, %%mm7          \n\t"
            "paddw %%mm3, %%mm7          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "packuswb %%mm1, %%mm7       \n\t"
            "movq -8(%%esi,%%ecx,), %%mm3 \n\t" 
            "pand _ActiveMask, %%mm7     \n\t"
            "psrlq _ShiftRem, %%mm3      \n\t"
            "movq (%%esi,%%ecx,), %%mm2  \n\t" 
            "paddb (%%edi,%%ecx,), %%mm7 \n\t" 
            "movq %%mm2, %%mm6           \n\t"
            "movq %%mm7, (%%edi,%%ecx,)  \n\t" 
            "movq -8(%%edi,%%ecx,), %%mm1 \n\t"
            "psllq _ShiftBpp, %%mm6      \n\t"
            "movq %%mm7, %%mm5           \n\t"
            "psrlq _ShiftRem, %%mm1      \n\t"
            "por %%mm6, %%mm3            \n\t"
            "psllq _ShiftBpp, %%mm5      \n\t"
            "punpckhbw %%mm0, %%mm3      \n\t" 
            "por %%mm5, %%mm1            \n\t"
            
            "punpckhbw %%mm0, %%mm2      \n\t" 
            "punpckhbw %%mm0, %%mm1      \n\t" 
            
            "movq %%mm2, %%mm4           \n\t"
            
            "movq %%mm1, %%mm5           \n\t"
            "psubw %%mm3, %%mm4          \n\t"
            "pxor %%mm7, %%mm7           \n\t"
            
            "movq %%mm4, %%mm6           \n\t"
            "psubw %%mm3, %%mm5          \n\t"
            
            
            
            "pcmpgtw %%mm4, %%mm0        \n\t" 
            "paddw %%mm5, %%mm6          \n\t"
            "pand %%mm4, %%mm0           \n\t" 
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "pand %%mm5, %%mm7           \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "psubw %%mm7, %%mm5          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "pcmpgtw %%mm6, %%mm0        \n\t" 
            "pand %%mm6, %%mm0           \n\t" 
            "psubw %%mm7, %%mm5          \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            
            "movq %%mm4, %%mm7           \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "movq %%mm7, %%mm0           \n\t"
            
            "pand %%mm7, %%mm5           \n\t"
            
            "pand %%mm0, %%mm2           \n\t"
            "pandn %%mm4, %%mm7          \n\t"
            "pandn %%mm1, %%mm0          \n\t"
            "paddw %%mm5, %%mm7          \n\t"
            "paddw %%mm2, %%mm0          \n\t"
            
            "pcmpgtw %%mm6, %%mm7        \n\t" 
            "pxor %%mm1, %%mm1           \n\t"
            "pand %%mm7, %%mm3           \n\t"
            "pandn %%mm0, %%mm7          \n\t"
            "pxor %%mm1, %%mm1           \n\t"
            "paddw %%mm3, %%mm7          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            
            "addl $8, %%ecx              \n\t"
            "packuswb %%mm7, %%mm1       \n\t"
            "paddb -8(%%edi,%%ecx,), %%mm1 \n\t" 
            "cmpl _MMXLength, %%ecx      \n\t"
            "movq %%mm1, -8(%%edi,%%ecx,) \n\t" 
                                
            "jb paeth_6lp                \n\t"

            : "=S" (dummy_value_S),             
              "=D" (dummy_value_D)

            : "0" (prev_row),  
              "1" (row)        

            : "%ecx"                            
#if 0  /* %mm0, ..., %mm7 not supported by gcc 2.7.2.3 or egcs 1.1 */
            , "%mm0", "%mm1", "%mm2", "%mm3"
            , "%mm4", "%mm5", "%mm6", "%mm7"
#endif
         );
      }
      break;  

      case 4:
      {
         _ActiveMask.use  = 0x00000000ffffffffLL;

         __asm__ __volatile__ (
            "movl _dif, %%ecx            \n\t"


            "pxor %%mm0, %%mm0           \n\t"
            
            "movq -8(%%edi,%%ecx,), %%mm1 \n\t" 
                                     
         "paeth_4lp:                     \n\t"
            
            "movq -8(%%esi,%%ecx,), %%mm3 \n\t" 
            "punpckhbw %%mm0, %%mm1      \n\t" 
            "movq (%%esi,%%ecx,), %%mm2  \n\t" 
            "punpcklbw %%mm0, %%mm2      \n\t" 
            
            "movq %%mm2, %%mm4           \n\t"
            "punpckhbw %%mm0, %%mm3      \n\t" 
            
            "movq %%mm1, %%mm5           \n\t"
            "psubw %%mm3, %%mm4          \n\t"
            "pxor %%mm7, %%mm7           \n\t"
            
            "movq %%mm4, %%mm6           \n\t"
            "psubw %%mm3, %%mm5          \n\t"
            
            
            
            "pcmpgtw %%mm4, %%mm0        \n\t" 
            "paddw %%mm5, %%mm6          \n\t"
            "pand %%mm4, %%mm0           \n\t" 
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "pand %%mm5, %%mm7           \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "psubw %%mm7, %%mm5          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "pcmpgtw %%mm6, %%mm0        \n\t" 
            "pand %%mm6, %%mm0           \n\t" 
            "psubw %%mm7, %%mm5          \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            
            "movq %%mm4, %%mm7           \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "movq %%mm7, %%mm0           \n\t"
            
            "pand %%mm7, %%mm5           \n\t"
            
            "pand %%mm0, %%mm2           \n\t"
            "pandn %%mm4, %%mm7          \n\t"
            "pandn %%mm1, %%mm0          \n\t"
            "paddw %%mm5, %%mm7          \n\t"
            "paddw %%mm2, %%mm0          \n\t"
            
            "pcmpgtw %%mm6, %%mm7        \n\t" 
            "pxor %%mm1, %%mm1           \n\t"
            "pand %%mm7, %%mm3           \n\t"
            "pandn %%mm0, %%mm7          \n\t"
            "paddw %%mm3, %%mm7          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "packuswb %%mm1, %%mm7       \n\t"
            "movq (%%esi,%%ecx,), %%mm3  \n\t" 
            "pand _ActiveMask, %%mm7     \n\t"
            "movq %%mm3, %%mm2           \n\t" 
            "paddb (%%edi,%%ecx,), %%mm7 \n\t" 
            "punpcklbw %%mm0, %%mm3      \n\t" 
            "movq %%mm7, (%%edi,%%ecx,)  \n\t" 
            "movq %%mm7, %%mm1           \n\t" 
            
            "punpckhbw %%mm0, %%mm2      \n\t" 
            "punpcklbw %%mm0, %%mm1      \n\t" 
            
            "movq %%mm2, %%mm4           \n\t"
            
            "movq %%mm1, %%mm5           \n\t"
            "psubw %%mm3, %%mm4          \n\t"
            "pxor %%mm7, %%mm7           \n\t"
            
            "movq %%mm4, %%mm6           \n\t"
            "psubw %%mm3, %%mm5          \n\t"
            
            
            
            "pcmpgtw %%mm4, %%mm0        \n\t" 
            "paddw %%mm5, %%mm6          \n\t"
            "pand %%mm4, %%mm0           \n\t" 
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "pand %%mm5, %%mm7           \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "psubw %%mm7, %%mm5          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "pcmpgtw %%mm6, %%mm0        \n\t" 
            "pand %%mm6, %%mm0           \n\t" 
            "psubw %%mm7, %%mm5          \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            
            "movq %%mm4, %%mm7           \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "movq %%mm7, %%mm0           \n\t"
            
            "pand %%mm7, %%mm5           \n\t"
            
            "pand %%mm0, %%mm2           \n\t"
            "pandn %%mm4, %%mm7          \n\t"
            "pandn %%mm1, %%mm0          \n\t"
            "paddw %%mm5, %%mm7          \n\t"
            "paddw %%mm2, %%mm0          \n\t"
            
            "pcmpgtw %%mm6, %%mm7        \n\t" 
            "pxor %%mm1, %%mm1           \n\t"
            "pand %%mm7, %%mm3           \n\t"
            "pandn %%mm0, %%mm7          \n\t"
            "pxor %%mm1, %%mm1           \n\t"
            "paddw %%mm3, %%mm7          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            
            "addl $8, %%ecx              \n\t"
            "packuswb %%mm7, %%mm1       \n\t"
            "paddb -8(%%edi,%%ecx,), %%mm1 \n\t" 
            "cmpl _MMXLength, %%ecx      \n\t"
            "movq %%mm1, -8(%%edi,%%ecx,) \n\t" 
                                
            "jb paeth_4lp                \n\t"

            : "=S" (dummy_value_S),             
              "=D" (dummy_value_D)

            : "0" (prev_row),  
              "1" (row)        

            : "%ecx"                            
#if 0  /* %mm0, ..., %mm7 not supported by gcc 2.7.2.3 or egcs 1.1 */
            , "%mm0", "%mm1", "%mm2", "%mm3"
            , "%mm4", "%mm5", "%mm6", "%mm7"
#endif
         );
      }
      break;  

      case 8:                          
      {
         _ActiveMask.use  = 0x00000000ffffffffLL;

         __asm__ __volatile__ (
            "movl _dif, %%ecx            \n\t"


            "pxor %%mm0, %%mm0           \n\t"
            
            "movq -8(%%edi,%%ecx,), %%mm1 \n\t" 
                                       
         "paeth_8lp:                     \n\t"
            
            "movq -8(%%esi,%%ecx,), %%mm3 \n\t" 
            "punpcklbw %%mm0, %%mm1      \n\t" 
            "movq (%%esi,%%ecx,), %%mm2  \n\t" 
            "punpcklbw %%mm0, %%mm2      \n\t" 
            
            "movq %%mm2, %%mm4           \n\t"
            "punpcklbw %%mm0, %%mm3      \n\t" 
            
            "movq %%mm1, %%mm5           \n\t"
            "psubw %%mm3, %%mm4          \n\t"
            "pxor %%mm7, %%mm7           \n\t"
            
            "movq %%mm4, %%mm6           \n\t"
            "psubw %%mm3, %%mm5          \n\t"
            
            
            
            "pcmpgtw %%mm4, %%mm0        \n\t" 
            "paddw %%mm5, %%mm6          \n\t"
            "pand %%mm4, %%mm0           \n\t" 
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "pand %%mm5, %%mm7           \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "psubw %%mm7, %%mm5          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "pcmpgtw %%mm6, %%mm0        \n\t" 
            "pand %%mm6, %%mm0           \n\t" 
            "psubw %%mm7, %%mm5          \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            
            "movq %%mm4, %%mm7           \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "movq %%mm7, %%mm0           \n\t"
            
            "pand %%mm7, %%mm5           \n\t"
            
            "pand %%mm0, %%mm2           \n\t"
            "pandn %%mm4, %%mm7          \n\t"
            "pandn %%mm1, %%mm0          \n\t"
            "paddw %%mm5, %%mm7          \n\t"
            "paddw %%mm2, %%mm0          \n\t"
            
            "pcmpgtw %%mm6, %%mm7        \n\t" 
            "pxor %%mm1, %%mm1           \n\t"
            "pand %%mm7, %%mm3           \n\t"
            "pandn %%mm0, %%mm7          \n\t"
            "paddw %%mm3, %%mm7          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "packuswb %%mm1, %%mm7       \n\t"
            "movq -8(%%esi,%%ecx,), %%mm3 \n\t" 
            "pand _ActiveMask, %%mm7     \n\t"
            "movq (%%esi,%%ecx,), %%mm2  \n\t" 
            "paddb (%%edi,%%ecx,), %%mm7 \n\t" 
            "punpckhbw %%mm0, %%mm3      \n\t" 
            "movq %%mm7, (%%edi,%%ecx,)  \n\t" 
            "movq -8(%%edi,%%ecx,), %%mm1 \n\t" 

            
            "punpckhbw %%mm0, %%mm2      \n\t" 
            "punpckhbw %%mm0, %%mm1      \n\t" 
            
            "movq %%mm2, %%mm4           \n\t"
            
            "movq %%mm1, %%mm5           \n\t"
            "psubw %%mm3, %%mm4          \n\t"
            "pxor %%mm7, %%mm7           \n\t"
            
            "movq %%mm4, %%mm6           \n\t"
            "psubw %%mm3, %%mm5          \n\t"
            
            
            
            "pcmpgtw %%mm4, %%mm0        \n\t" 
            "paddw %%mm5, %%mm6          \n\t"
            "pand %%mm4, %%mm0           \n\t" 
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "pand %%mm5, %%mm7           \n\t" 
            "psubw %%mm0, %%mm4          \n\t"
            "psubw %%mm7, %%mm5          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            "pcmpgtw %%mm6, %%mm0        \n\t" 
            "pand %%mm6, %%mm0           \n\t" 
            "psubw %%mm7, %%mm5          \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            
            "movq %%mm4, %%mm7           \n\t"
            "psubw %%mm0, %%mm6          \n\t"
            "pcmpgtw %%mm5, %%mm7        \n\t" 
            "movq %%mm7, %%mm0           \n\t"
            
            "pand %%mm7, %%mm5           \n\t"
            
            "pand %%mm0, %%mm2           \n\t"
            "pandn %%mm4, %%mm7          \n\t"
            "pandn %%mm1, %%mm0          \n\t"
            "paddw %%mm5, %%mm7          \n\t"
            "paddw %%mm2, %%mm0          \n\t"
            
            "pcmpgtw %%mm6, %%mm7        \n\t" 
            "pxor %%mm1, %%mm1           \n\t"
            "pand %%mm7, %%mm3           \n\t"
            "pandn %%mm0, %%mm7          \n\t"
            "pxor %%mm1, %%mm1           \n\t"
            "paddw %%mm3, %%mm7          \n\t"
            "pxor %%mm0, %%mm0           \n\t"
            
            "addl $8, %%ecx              \n\t"
            "packuswb %%mm7, %%mm1       \n\t"
            "paddb -8(%%edi,%%ecx,), %%mm1 \n\t" 
            "cmpl _MMXLength, %%ecx      \n\t"
            "movq %%mm1, -8(%%edi,%%ecx,) \n\t" 
                            
            "jb paeth_8lp                \n\t"

            : "=S" (dummy_value_S),             
              "=D" (dummy_value_D)

            : "0" (prev_row),  
              "1" (row)        

            : "%ecx"                            
#if 0  /* %mm0, ..., %mm7 not supported by gcc 2.7.2.3 or egcs 1.1 */
            , "%mm0", "%mm1", "%mm2", "%mm3"
            , "%mm4", "%mm5", "%mm6", "%mm7"
#endif
         );
      }
      break;  

      case 1:                
      case 2:                
      default:               
      {
         __asm__ __volatile__ (
#ifdef __PIC__
            "pushl %%ebx                 \n\t" 
#endif
            "movl _dif, %%ebx            \n\t"
            "cmpl _FullLength, %%ebx     \n\t"
            "jnb paeth_dend              \n\t"



            
            "movl %%ebx, %%edx           \n\t"

            "subl %%ecx, %%edx           \n\t" 
            "xorl %%ecx, %%ecx           \n\t" 

         "paeth_dlp:                     \n\t"
            "xorl %%eax, %%eax           \n\t"
            
            "movb (%%esi,%%ebx,), %%al   \n\t" 
            "movb (%%esi,%%edx,), %%cl   \n\t" 
            "subl %%ecx, %%eax           \n\t" 
            "movl %%eax, _patemp         \n\t" 
            "xorl %%eax, %%eax           \n\t"
            
            "movb (%%edi,%%edx,), %%al   \n\t" 
            "subl %%ecx, %%eax           \n\t" 
            "movl %%eax, %%ecx           \n\t"
            
            "addl _patemp, %%eax         \n\t" 
            
            "testl $0x80000000, %%eax    \n\t"
            "jz paeth_dpca               \n\t"
            "negl %%eax                  \n\t" 

         "paeth_dpca:                    \n\t"
            "movl %%eax, _pctemp         \n\t" 
            
            "testl $0x80000000, %%ecx    \n\t"
            "jz paeth_dpba               \n\t"
            "negl %%ecx                  \n\t" 

         "paeth_dpba:                    \n\t"
            "movl %%ecx, _pbtemp         \n\t" 
            
            "movl _patemp, %%eax         \n\t"
            "testl $0x80000000, %%eax    \n\t"
            "jz paeth_dpaa               \n\t"
            "negl %%eax                  \n\t" 

         "paeth_dpaa:                    \n\t"
            "movl %%eax, _patemp         \n\t" 
            
            "cmpl %%ecx, %%eax           \n\t"
            "jna paeth_dabb              \n\t"
            
            "cmpl _pctemp, %%ecx         \n\t"
            "jna paeth_dbbc              \n\t"
            
            "movb (%%esi,%%edx,), %%cl   \n\t" 
            "jmp paeth_dpaeth            \n\t"

         "paeth_dbbc:                    \n\t"
            
            "movb (%%esi,%%ebx,), %%cl   \n\t" 
            "jmp paeth_dpaeth            \n\t"

         "paeth_dabb:                    \n\t"
            
            "cmpl _pctemp, %%eax         \n\t"
            "jna paeth_dabc              \n\t"
            
            "movb (%%esi,%%edx,), %%cl   \n\t" 
            "jmp paeth_dpaeth            \n\t"

         "paeth_dabc:                    \n\t"
            
            "movb (%%edi,%%edx,), %%cl   \n\t" 

         "paeth_dpaeth:                  \n\t"
            "incl %%ebx                  \n\t"
            "incl %%edx                  \n\t"
            
            "addb %%cl, -1(%%edi,%%ebx,) \n\t"
            "cmpl _FullLength, %%ebx     \n\t"
            "jb paeth_dlp                \n\t"

         "paeth_dend:                    \n\t"
#ifdef __PIC__
            "popl %%ebx                  \n\t" 
#endif

            : "=c" (dummy_value_c),            
              "=S" (dummy_value_S),
              "=D" (dummy_value_D)

            : "0" (bpp),       
              "1" (prev_row),  
              "2" (row)        

            : "%eax", "%edx"                   
#ifndef __PIC__
            , "%ebx"
#endif
         );
      }
      return;                   

   } 

   __asm__ __volatile__ (
      
      
#ifdef __PIC__
      "pushl %%ebx                 \n\t" 
#endif
      "movl _MMXLength, %%ebx      \n\t"
      "cmpl _FullLength, %%ebx     \n\t"
      "jnb paeth_end               \n\t"


      
      "movl %%ebx, %%edx           \n\t"

      "subl %%ecx, %%edx           \n\t" 
      "xorl %%ecx, %%ecx           \n\t" 

   "paeth_lp2:                     \n\t"
      "xorl %%eax, %%eax           \n\t"
      
      "movb (%%esi,%%ebx,), %%al   \n\t" 
      "movb (%%esi,%%edx,), %%cl   \n\t" 
      "subl %%ecx, %%eax           \n\t" 
      "movl %%eax, _patemp         \n\t" 
      "xorl %%eax, %%eax           \n\t"
      
      "movb (%%edi,%%edx,), %%al   \n\t" 
      "subl %%ecx, %%eax           \n\t" 
      "movl %%eax, %%ecx           \n\t"
      
      "addl _patemp, %%eax         \n\t" 
      
      "testl $0x80000000, %%eax    \n\t"
      "jz paeth_pca2               \n\t"
      "negl %%eax                  \n\t" 

   "paeth_pca2:                    \n\t"
      "movl %%eax, _pctemp         \n\t" 
      
      "testl $0x80000000, %%ecx    \n\t"
      "jz paeth_pba2               \n\t"
      "negl %%ecx                  \n\t" 

   "paeth_pba2:                    \n\t"
      "movl %%ecx, _pbtemp         \n\t" 
      
      "movl _patemp, %%eax         \n\t"
      "testl $0x80000000, %%eax    \n\t"
      "jz paeth_paa2               \n\t"
      "negl %%eax                  \n\t" 

   "paeth_paa2:                    \n\t"
      "movl %%eax, _patemp         \n\t" 
      
      "cmpl %%ecx, %%eax           \n\t"
      "jna paeth_abb2              \n\t"
      
      "cmpl _pctemp, %%ecx         \n\t"
      "jna paeth_bbc2              \n\t"
      
      "movb (%%esi,%%edx,), %%cl   \n\t" 
      "jmp paeth_paeth2            \n\t"

   "paeth_bbc2:                    \n\t"
      
      "movb (%%esi,%%ebx,), %%cl   \n\t" 
      "jmp paeth_paeth2            \n\t"

   "paeth_abb2:                    \n\t"
      
      "cmpl _pctemp, %%eax         \n\t"
      "jna paeth_abc2              \n\t"
      
      "movb (%%esi,%%edx,), %%cl   \n\t" 
      "jmp paeth_paeth2            \n\t"

   "paeth_abc2:                    \n\t"
      
      "movb (%%edi,%%edx,), %%cl   \n\t" 

   "paeth_paeth2:                  \n\t"
      "incl %%ebx                  \n\t"
      "incl %%edx                  \n\t"
      
      "addb %%cl, -1(%%edi,%%ebx,) \n\t"
      "cmpl _FullLength, %%ebx     \n\t"
      "jb paeth_lp2                \n\t"

   "paeth_end:                     \n\t"
      "EMMS                        \n\t" 
#ifdef __PIC__
      "popl %%ebx                  \n\t" 
#endif

      : "=c" (dummy_value_c),            
        "=S" (dummy_value_S),
        "=D" (dummy_value_D)

      : "0" (bpp),       
        "1" (prev_row),  
        "2" (row)        

      : "%eax", "%edx"                   
#ifndef __PIC__
      , "%ebx"
#endif
   );

} /* end png_read_filter_row_mmx_paeth() */
#endif




#ifdef PNG_THREAD_UNSAFE_OK








static void /* PRIVATE */
png_read_filter_row_mmx_sub(png_row_infop row_info, png_bytep row)
{
   int bpp;
   int dummy_value_a;
   int dummy_value_D;

   bpp = (row_info->pixel_depth + 7) >> 3;   
   _FullLength = row_info->rowbytes - bpp;   

   __asm__ __volatile__ (

      "movl %%edi, %%esi           \n\t" 

      "addl %%eax, %%edi           \n\t" 

      
      "movl %%edi, _dif            \n\t" 
      "addl $0xf, _dif             \n\t" 
                                         
      "xorl %%ecx, %%ecx           \n\t"
      "andl $0xfffffff8, _dif      \n\t" 
      "subl %%edi, _dif            \n\t" 
      "jz sub_go                   \n\t" 

   "sub_lp1:                       \n\t" 
      "movb (%%esi,%%ecx,), %%al   \n\t"
      "addb %%al, (%%edi,%%ecx,)   \n\t"
      "incl %%ecx                  \n\t"
      "cmpl _dif, %%ecx            \n\t"
      "jb sub_lp1                  \n\t"

   "sub_go:                        \n\t"
      "movl _FullLength, %%eax     \n\t"
      "movl %%eax, %%edx           \n\t"
      "subl %%ecx, %%edx           \n\t" 
      "andl $0x00000007, %%edx     \n\t" 
      "subl %%edx, %%eax           \n\t" 
      "movl %%eax, _MMXLength      \n\t"

      : "=a" (dummy_value_a),   
        "=D" (dummy_value_D)    

      : "0" (bpp),              
        "1" (row)               

      : "%esi", "%ecx", "%edx"            

#if 0  /* MMX regs (%mm0, etc.) not supported by gcc 2.7.2.3 or egcs 1.1 */
      , "%mm0", "%mm1", "%mm2", "%mm3"
      , "%mm4", "%mm5", "%mm6", "%mm7"
#endif
   );

   
   switch (bpp)
   {
      case 3:
      {
         _ActiveMask.use  = 0x0000ffffff000000LL;
         _ShiftBpp.use = 24;       
         _ShiftRem.use  = 40;      

         __asm__ __volatile__ (

            "movq _ActiveMask, %%mm7       \n\t" 
                                                
            "movl %%edi, %%esi            \n\t" 

            "addl %%eax, %%edi            \n\t" 
            "movq %%mm7, %%mm6            \n\t"
            "movl _dif, %%edx             \n\t"
            "psllq _ShiftBpp, %%mm6       \n\t" 
                                                
            
            "movq -8(%%edi,%%edx,), %%mm1 \n\t"

         "sub_3lp:                        \n\t" 
            "psrlq _ShiftRem, %%mm1       \n\t" 
                                                
            
            "movq (%%edi,%%edx,), %%mm0   \n\t"
            "paddb %%mm1, %%mm0           \n\t"

            
            "movq %%mm0, %%mm1            \n\t" 
            "psllq _ShiftBpp, %%mm1       \n\t" 
            "pand %%mm7, %%mm1            \n\t" 
            "paddb %%mm1, %%mm0           \n\t"

            
            "movq %%mm0, %%mm1            \n\t" 
            "psllq _ShiftBpp, %%mm1       \n\t" 
            "pand %%mm6, %%mm1            \n\t" 
            "addl $8, %%edx               \n\t"
            "paddb %%mm1, %%mm0           \n\t"

            "cmpl _MMXLength, %%edx       \n\t"
            "movq %%mm0, -8(%%edi,%%edx,) \n\t" 
            "movq %%mm0, %%mm1            \n\t" 
            "jb sub_3lp                   \n\t"

            : "=a" (dummy_value_a),   
              "=D" (dummy_value_D)    

            : "0" (bpp),              
              "1" (row)               

            : "%edx", "%esi"                    
#if 0  /* MMX regs (%mm0, etc.) not supported by gcc 2.7.2.3 or egcs 1.1 */
            , "%mm0", "%mm1", "%mm6", "%mm7"
#endif
         );
      }
      break;

      case 1:
      {
         __asm__ __volatile__ (
            "movl _dif, %%edx            \n\t"

            "cmpl _FullLength, %%edx     \n\t"
            "jnb sub_1end                \n\t"
            "movl %%edi, %%esi           \n\t" 
            "xorl %%eax, %%eax           \n\t"

            "addl %%eax, %%edi           \n\t" 

         "sub_1lp:                       \n\t"
            "movb (%%esi,%%edx,), %%al   \n\t"
            "addb %%al, (%%edi,%%edx,)   \n\t"
            "incl %%edx                  \n\t"
            "cmpl _FullLength, %%edx     \n\t"
            "jb sub_1lp                  \n\t"

         "sub_1end:                      \n\t"

            : "=a" (dummy_value_a),   
              "=D" (dummy_value_D)    

            : "0" (bpp),              
              "1" (row)               

            : "%edx", "%esi"                    
         );
      }
      return;

      case 6:
      case 4:
      
      
      {
         _ShiftBpp.use = bpp << 3;
         _ShiftRem.use = 64 - _ShiftBpp.use;

         __asm__ __volatile__ (

            "movl _dif, %%edx             \n\t"
            "movl %%edi, %%esi            \n\t" 

            "addl %%eax, %%edi            \n\t" 

            
            "movq -8(%%edi,%%edx,), %%mm1 \n\t"

         "sub_4lp:                        \n\t" 
            "psrlq _ShiftRem, %%mm1       \n\t" 
                                                
            "movq (%%edi,%%edx,), %%mm0   \n\t"
            "paddb %%mm1, %%mm0           \n\t"

            
            "movq %%mm0, %%mm1            \n\t" 
            "psllq _ShiftBpp, %%mm1       \n\t" 
            "addl $8, %%edx               \n\t"
            "paddb %%mm1, %%mm0           \n\t"

            "cmpl _MMXLength, %%edx       \n\t"
            "movq %%mm0, -8(%%edi,%%edx,) \n\t"
            "movq %%mm0, %%mm1            \n\t" 
            "jb sub_4lp                   \n\t"

            : "=a" (dummy_value_a),   
              "=D" (dummy_value_D)    

            : "0" (bpp),              
              "1" (row)               

            : "%edx", "%esi"                    
#if 0  /* MMX regs (%mm0, etc.) not supported by gcc 2.7.2.3 or egcs 1.1 */
            , "%mm0", "%mm1"
#endif
         );
      }
      break;

      case 2:
      {
         _ActiveMask.use = 0x00000000ffff0000LL;
         _ShiftBpp.use = 16;       
         _ShiftRem.use = 48;       

         __asm__ __volatile__ (
            "movq _ActiveMask, %%mm7      \n\t" 
                                                
            "movl _dif, %%edx             \n\t"
            "movq %%mm7, %%mm6            \n\t"

            "psllq _ShiftBpp, %%mm6       \n\t" 
                                                
            "movl %%edi, %%esi            \n\t" 
            "movq %%mm6, %%mm5            \n\t"

            "addl %%eax, %%edi            \n\t" 
            "psllq _ShiftBpp, %%mm5       \n\t" 
                                                
            
            "movq -8(%%edi,%%edx,), %%mm1 \n\t"

         "sub_2lp:                        \n\t" 
            "psrlq _ShiftRem, %%mm1       \n\t" 
                                                
            
            "movq (%%edi,%%edx,), %%mm0   \n\t"
            "paddb %%mm1, %%mm0           \n\t"

            
            "movq %%mm0, %%mm1            \n\t" 
            "psllq _ShiftBpp, %%mm1       \n\t" 
            "pand %%mm7, %%mm1            \n\t" 
            "paddb %%mm1, %%mm0           \n\t"

            
            "movq %%mm0, %%mm1            \n\t" 
            "psllq _ShiftBpp, %%mm1       \n\t" 
            "pand %%mm6, %%mm1            \n\t" 
            "paddb %%mm1, %%mm0           \n\t"

            
            "movq %%mm0, %%mm1            \n\t" 
            "psllq _ShiftBpp, %%mm1       \n\t" 
            "pand %%mm5, %%mm1            \n\t" 
            "addl $8, %%edx               \n\t"
            "paddb %%mm1, %%mm0           \n\t"
            "cmpl _MMXLength, %%edx       \n\t"
            "movq %%mm0, -8(%%edi,%%edx,) \n\t" 
            "movq %%mm0, %%mm1            \n\t" 
            "jb sub_2lp                   \n\t"

            : "=a" (dummy_value_a),   
              "=D" (dummy_value_D)    

            : "0" (bpp),              
              "1" (row)               

            : "%edx", "%esi"                    
#if 0  /* MMX regs (%mm0, etc.) not supported by gcc 2.7.2.3 or egcs 1.1 */
            , "%mm0", "%mm1", "%mm5", "%mm6", "%mm7"
#endif
         );
      }
      break;

      case 8:
      {
         __asm__ __volatile__ (

            "movl _dif, %%edx             \n\t"
            "movl %%edi, %%esi            \n\t" 

            "addl %%eax, %%edi            \n\t" 
            "movl _MMXLength, %%ecx       \n\t"

            
            "movq -8(%%edi,%%edx,), %%mm7 \n\t"
            "andl $0x0000003f, %%ecx      \n\t" 

         "sub_8lp:                        \n\t"
            "movq (%%edi,%%edx,), %%mm0   \n\t" 
            "paddb %%mm7, %%mm0           \n\t"
            "movq 8(%%edi,%%edx,), %%mm1  \n\t" 
            "movq %%mm0, (%%edi,%%edx,)   \n\t" 

            
            
            
            

            "paddb %%mm0, %%mm1           \n\t"
            "movq 16(%%edi,%%edx,), %%mm2 \n\t" 
            "movq %%mm1, 8(%%edi,%%edx,)  \n\t" 
            "paddb %%mm1, %%mm2           \n\t"
            "movq 24(%%edi,%%edx,), %%mm3 \n\t" 
            "movq %%mm2, 16(%%edi,%%edx,) \n\t" 
            "paddb %%mm2, %%mm3           \n\t"
            "movq 32(%%edi,%%edx,), %%mm4 \n\t" 
            "movq %%mm3, 24(%%edi,%%edx,) \n\t" 
            "paddb %%mm3, %%mm4           \n\t"
            "movq 40(%%edi,%%edx,), %%mm5 \n\t" 
            "movq %%mm4, 32(%%edi,%%edx,) \n\t" 
            "paddb %%mm4, %%mm5           \n\t"
            "movq 48(%%edi,%%edx,), %%mm6 \n\t" 
            "movq %%mm5, 40(%%edi,%%edx,) \n\t" 
            "paddb %%mm5, %%mm6           \n\t"
            "movq 56(%%edi,%%edx,), %%mm7 \n\t" 
            "movq %%mm6, 48(%%edi,%%edx,) \n\t" 
            "addl $64, %%edx              \n\t"
            "paddb %%mm6, %%mm7           \n\t"
            "cmpl %%ecx, %%edx            \n\t"
            "movq %%mm7, -8(%%edi,%%edx,) \n\t" 
            "jb sub_8lp                   \n\t"

            "cmpl _MMXLength, %%edx       \n\t"
            "jnb sub_8lt8                 \n\t"

         "sub_8lpA:                       \n\t"
            "movq (%%edi,%%edx,), %%mm0   \n\t"
            "addl $8, %%edx               \n\t"
            "paddb %%mm7, %%mm0           \n\t"
            "cmpl _MMXLength, %%edx       \n\t"
            "movq %%mm0, -8(%%edi,%%edx,) \n\t" 
            "movq %%mm0, %%mm7            \n\t" 
                                                
                                                
            "jb sub_8lpA                  \n\t"

         "sub_8lt8:                       \n\t"

            : "=a" (dummy_value_a),   
              "=D" (dummy_value_D)    

            : "0" (bpp),              
              "1" (row)               

            : "%ecx", "%edx", "%esi"            
#if 0  /* MMX regs (%mm0, etc.) not supported by gcc 2.7.2.3 or egcs 1.1 */
            , "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7"
#endif
         );
      }
      break;

      default:                
      {
         __asm__ __volatile__ (
            "movl _dif, %%edx             \n\t"

            "movl %%edi, %%esi            \n\t" 

            "addl %%eax, %%edi            \n\t" 

         "sub_Alp:                        \n\t"
            "movq (%%edi,%%edx,), %%mm0   \n\t"
            "movq (%%esi,%%edx,), %%mm1   \n\t"
            "addl $8, %%edx               \n\t"
            "paddb %%mm1, %%mm0           \n\t"
            "cmpl _MMXLength, %%edx       \n\t"
            "movq %%mm0, -8(%%edi,%%edx,) \n\t" 
                                                
            "jb sub_Alp                   \n\t"

            : "=a" (dummy_value_a),   
              "=D" (dummy_value_D)    

            : "0" (bpp),              
              "1" (row)               

            : "%edx", "%esi"                    
#if 0  /* MMX regs (%mm0, etc.) not supported by gcc 2.7.2.3 or egcs 1.1 */
            , "%mm0", "%mm1"
#endif
         );
      }
      break;

   } 

   __asm__ __volatile__ (
      "movl _MMXLength, %%edx       \n\t"

      "cmpl _FullLength, %%edx      \n\t"
      "jnb sub_end                  \n\t"

      "movl %%edi, %%esi            \n\t" 

      "addl %%eax, %%edi            \n\t" 
      "xorl %%eax, %%eax            \n\t"

   "sub_lp2:                        \n\t"
      "movb (%%esi,%%edx,), %%al    \n\t"
      "addb %%al, (%%edi,%%edx,)    \n\t"
      "incl %%edx                   \n\t"
      "cmpl _FullLength, %%edx      \n\t"
      "jb sub_lp2                   \n\t"

   "sub_end:                        \n\t"
      "EMMS                         \n\t" 

      : "=a" (dummy_value_a),   
        "=D" (dummy_value_D)    

      : "0" (bpp),              
        "1" (row)               

      : "%edx", "%esi"                    
   );

} 
#endif












static void /* PRIVATE */
png_read_filter_row_mmx_up(png_row_infop row_info, png_bytep row,
                           png_bytep prev_row)
{
   png_uint_32 len;
   int dummy_value_d;   
   int dummy_value_S;
   int dummy_value_D;

   len = row_info->rowbytes;              

   __asm__ __volatile__ (

      
#ifdef __PIC__
      "pushl %%ebx                  \n\t"
#endif
      "movl %%edi, %%ecx            \n\t"
      "xorl %%ebx, %%ebx            \n\t"
      "addl $0x7, %%ecx             \n\t"
      "xorl %%eax, %%eax            \n\t"
      "andl $0xfffffff8, %%ecx      \n\t"

      "subl %%edi, %%ecx            \n\t"
      "jz up_go                     \n\t"

   "up_lp1:                         \n\t" 
      "movb (%%edi,%%ebx,), %%al    \n\t"
      "addb (%%esi,%%ebx,), %%al    \n\t"
      "incl %%ebx                   \n\t"
      "cmpl %%ecx, %%ebx            \n\t"
      "movb %%al, -1(%%edi,%%ebx,)  \n\t" 
      "jb up_lp1                    \n\t" 

   "up_go:                          \n\t"

      "movl %%edx, %%ecx            \n\t"
      "subl %%ebx, %%edx            \n\t" 
      "andl $0x0000003f, %%edx      \n\t" 
      "subl %%edx, %%ecx            \n\t" 

      
      
   "up_loop:                        \n\t"
      "movq (%%esi,%%ebx,), %%mm1   \n\t"
      "movq (%%edi,%%ebx,), %%mm0   \n\t"
      "movq 8(%%esi,%%ebx,), %%mm3  \n\t"
      "paddb %%mm1, %%mm0           \n\t"
      "movq 8(%%edi,%%ebx,), %%mm2  \n\t"
      "movq %%mm0, (%%edi,%%ebx,)   \n\t"
      "paddb %%mm3, %%mm2           \n\t"
      "movq 16(%%esi,%%ebx,), %%mm5 \n\t"
      "movq %%mm2, 8(%%edi,%%ebx,)  \n\t"
      "movq 16(%%edi,%%ebx,), %%mm4 \n\t"
      "movq 24(%%esi,%%ebx,), %%mm7 \n\t"
      "paddb %%mm5, %%mm4           \n\t"
      "movq 24(%%edi,%%ebx,), %%mm6 \n\t"
      "movq %%mm4, 16(%%edi,%%ebx,) \n\t"
      "paddb %%mm7, %%mm6           \n\t"
      "movq 32(%%esi,%%ebx,), %%mm1 \n\t"
      "movq %%mm6, 24(%%edi,%%ebx,) \n\t"
      "movq 32(%%edi,%%ebx,), %%mm0 \n\t"
      "movq 40(%%esi,%%ebx,), %%mm3 \n\t"
      "paddb %%mm1, %%mm0           \n\t"
      "movq 40(%%edi,%%ebx,), %%mm2 \n\t"
      "movq %%mm0, 32(%%edi,%%ebx,) \n\t"
      "paddb %%mm3, %%mm2           \n\t"
      "movq 48(%%esi,%%ebx,), %%mm5 \n\t"
      "movq %%mm2, 40(%%edi,%%ebx,) \n\t"
      "movq 48(%%edi,%%ebx,), %%mm4 \n\t"
      "movq 56(%%esi,%%ebx,), %%mm7 \n\t"
      "paddb %%mm5, %%mm4           \n\t"
      "movq 56(%%edi,%%ebx,), %%mm6 \n\t"
      "movq %%mm4, 48(%%edi,%%ebx,) \n\t"
      "addl $64, %%ebx              \n\t"
      "paddb %%mm7, %%mm6           \n\t"
      "cmpl %%ecx, %%ebx            \n\t"
      "movq %%mm6, -8(%%edi,%%ebx,) \n\t" 
      "jb up_loop                   \n\t" 

      "cmpl $0, %%edx               \n\t" 
      "jz up_end                    \n\t"

      "cmpl $8, %%edx               \n\t" 
      "jb up_lt8                    \n\t" 

      "addl %%edx, %%ecx            \n\t"
      "andl $0x00000007, %%edx      \n\t" 
      "subl %%edx, %%ecx            \n\t" 
      "jz up_lt8                    \n\t"

   "up_lpA:                         \n\t" 
      "movq (%%esi,%%ebx,), %%mm1   \n\t"
      "movq (%%edi,%%ebx,), %%mm0   \n\t"
      "addl $8, %%ebx               \n\t"
      "paddb %%mm1, %%mm0           \n\t"
      "cmpl %%ecx, %%ebx            \n\t"
      "movq %%mm0, -8(%%edi,%%ebx,) \n\t" 
      "jb up_lpA                    \n\t" 
      "cmpl $0, %%edx               \n\t" 
      "jz up_end                    \n\t"

   "up_lt8:                         \n\t"
      "xorl %%eax, %%eax            \n\t"
      "addl %%edx, %%ecx            \n\t" 

   "up_lp2:                         \n\t" 
      "movb (%%edi,%%ebx,), %%al    \n\t"
      "addb (%%esi,%%ebx,), %%al    \n\t"
      "incl %%ebx                   \n\t"
      "cmpl %%ecx, %%ebx            \n\t"
      "movb %%al, -1(%%edi,%%ebx,)  \n\t" 
      "jb up_lp2                    \n\t" 

   "up_end:                         \n\t"
      "EMMS                         \n\t" 
#ifdef __PIC__
      "popl %%ebx                   \n\t"
#endif

      : "=d" (dummy_value_d),   
        "=S" (dummy_value_S),   
        "=D" (dummy_value_D)    

      : "0" (len),              
        "1" (prev_row),         
        "2" (row)               

      : "%eax", "%ecx"            
#ifndef __PIC__
      , "%ebx"
#endif

#if 0  /* MMX regs (%mm0, etc.) not supported by gcc 2.7.2.3 or egcs 1.1 */
      , "%mm0", "%mm1", "%mm2", "%mm3"
      , "%mm4", "%mm5", "%mm6", "%mm7"
#endif
   );

} 

#endif /* PNG_ASSEMBLER_CODE_SUPPORTED */




/*===========================================================================*/
/*                                                                           */
/*                   P N G _ R E A D _ F I L T E R _ R O W                   */
/*                                                                           */
/*===========================================================================*/


/* Optimized png_read_filter_row routines */

void /* PRIVATE */
png_read_filter_row(png_structp png_ptr, png_row_infop row_info, png_bytep
   row, png_bytep prev_row, int filter)
{
#ifdef PNG_DEBUG
   char filnm[10];
#endif

#if defined(PNG_ASSEMBLER_CODE_SUPPORTED)
/* GRR:  these are superseded by png_ptr->asm_flags: */
#define UseMMX_sub    1   
#define UseMMX_up     1   
#define UseMMX_avg    1   
#define UseMMX_paeth  1   

   if (_mmx_supported == 2) {
       /* this should have happened in png_init_mmx_flags() already */
#if !defined(PNG_1_0_X)
       png_warning(png_ptr, "asm_flags may not have been initialized");
#endif
       png_mmx_support();
   }
#endif /* PNG_ASSEMBLER_CODE_SUPPORTED */

#ifdef PNG_DEBUG
   png_debug(1, "in png_read_filter_row (pnggccrd.c)\n");
   switch (filter)
   {
      case 0: sprintf(filnm, "none");
         break;
      case 1: sprintf(filnm, "sub-%s",
#if defined(PNG_ASSEMBLER_CODE_SUPPORTED) && defined(PNG_THREAD_UNSAFE_OK)
#if !defined(PNG_1_0_X)
        (png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_SUB)? "MMX" :
#endif
#endif
"x86");
         break;
      case 2: sprintf(filnm, "up-%s",
#ifdef PNG_ASSEMBLER_CODE_SUPPORTED
#if !defined(PNG_1_0_X)
        (png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_UP)? "MMX" :
#endif
#endif
 "x86");
         break;
      case 3: sprintf(filnm, "avg-%s",
#if defined(PNG_ASSEMBLER_CODE_SUPPORTED) && defined(PNG_THREAD_UNSAFE_OK)
#if !defined(PNG_1_0_X)
        (png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_AVG)? "MMX" :
#endif
#endif
 "x86");
         break;
      case 4: sprintf(filnm, "Paeth-%s",
#if defined(PNG_ASSEMBLER_CODE_SUPPORTED) && defined(PNG_THREAD_UNSAFE_OK)
#if !defined(PNG_1_0_X)
        (png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_PAETH)? "MMX":
#endif
#endif
"x86");
         break;
      default: sprintf(filnm, "unknw");
         break;
   }
   png_debug2(0, "row_number=%5ld, %5s, ", png_ptr->row_number, filnm);
   png_debug1(0, "row=0x%08lx, ", (unsigned long)row);
   png_debug2(0, "pixdepth=%2d, bytes=%d, ", (int)row_info->pixel_depth,
      (int)((row_info->pixel_depth + 7) >> 3));
   png_debug1(0,"rowbytes=%8ld\n", row_info->rowbytes);
#endif /* PNG_DEBUG */

   switch (filter)
   {
      case PNG_FILTER_VALUE_NONE:
         break;

      case PNG_FILTER_VALUE_SUB:
#if defined(PNG_ASSEMBLER_CODE_SUPPORTED) && defined(PNG_THREAD_UNSAFE_OK)
#if !defined(PNG_1_0_X)
         if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_SUB) &&
             (row_info->pixel_depth >= png_ptr->mmx_bitdepth_threshold) &&
             (row_info->rowbytes >= png_ptr->mmx_rowbytes_threshold))
#else
         if (_mmx_supported)
#endif
         {
            png_read_filter_row_mmx_sub(row_info, row);
         }
         else
#endif /* PNG_ASSEMBLER_CODE_SUPPORTED */
         {
            png_uint_32 i;
            png_uint_32 istop = row_info->rowbytes;
            png_uint_32 bpp = (row_info->pixel_depth + 7) >> 3;
            png_bytep rp = row + bpp;
            png_bytep lp = row;

            for (i = bpp; i < istop; i++)
            {
               *rp = (png_byte)(((int)(*rp) + (int)(*lp++)) & 0xff);
               rp++;
            }
         }  /* end !UseMMX_sub */
         break;

      case PNG_FILTER_VALUE_UP:
#if defined(PNG_ASSEMBLER_CODE_SUPPORTED)
#if !defined(PNG_1_0_X)
         if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_UP) &&
             (row_info->pixel_depth >= png_ptr->mmx_bitdepth_threshold) &&
             (row_info->rowbytes >= png_ptr->mmx_rowbytes_threshold))
#else
         if (_mmx_supported)
#endif
         {
            png_read_filter_row_mmx_up(row_info, row, prev_row);
         }
          else
#endif /* PNG_ASSEMBLER_CODE_SUPPORTED */
         {
            png_uint_32 i;
            png_uint_32 istop = row_info->rowbytes;
            png_bytep rp = row;
            png_bytep pp = prev_row;

            for (i = 0; i < istop; ++i)
            {
               *rp = (png_byte)(((int)(*rp) + (int)(*pp++)) & 0xff);
               rp++;
            }
         }  /* end !UseMMX_up */
         break;

      case PNG_FILTER_VALUE_AVG:
#if defined(PNG_ASSEMBLER_CODE_SUPPORTED) && defined(PNG_THREAD_UNSAFE_OK)
#if !defined(PNG_1_0_X)
         if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_AVG) &&
             (row_info->pixel_depth >= png_ptr->mmx_bitdepth_threshold) &&
             (row_info->rowbytes >= png_ptr->mmx_rowbytes_threshold))
#else
         if (_mmx_supported)
#endif
         {
            png_read_filter_row_mmx_avg(row_info, row, prev_row);
         }
         else
#endif /* PNG_ASSEMBLER_CODE_SUPPORTED */
         {
            png_uint_32 i;
            png_bytep rp = row;
            png_bytep pp = prev_row;
            png_bytep lp = row;
            png_uint_32 bpp = (row_info->pixel_depth + 7) >> 3;
            png_uint_32 istop = row_info->rowbytes - bpp;

            for (i = 0; i < bpp; i++)
            {
               *rp = (png_byte)(((int)(*rp) +
                  ((int)(*pp++) >> 1)) & 0xff);
               rp++;
            }

            for (i = 0; i < istop; i++)
            {
               *rp = (png_byte)(((int)(*rp) +
                  ((int)(*pp++ + *lp++) >> 1)) & 0xff);
               rp++;
            }
         }  /* end !UseMMX_avg */
         break;

      case PNG_FILTER_VALUE_PAETH:
#if defined(PNG_ASSEMBLER_CODE_SUPPORTED) && defined(PNG_THREAD_UNSAFE_OK)
#if !defined(PNG_1_0_X)
         if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_PAETH) &&
             (row_info->pixel_depth >= png_ptr->mmx_bitdepth_threshold) &&
             (row_info->rowbytes >= png_ptr->mmx_rowbytes_threshold))
#else
         if (_mmx_supported)
#endif
         {
            png_read_filter_row_mmx_paeth(row_info, row, prev_row);
         }
         else
#endif /* PNG_ASSEMBLER_CODE_SUPPORTED */
         {
            png_uint_32 i;
            png_bytep rp = row;
            png_bytep pp = prev_row;
            png_bytep lp = row;
            png_bytep cp = prev_row;
            png_uint_32 bpp = (row_info->pixel_depth + 7) >> 3;
            png_uint_32 istop = row_info->rowbytes - bpp;

            for (i = 0; i < bpp; i++)
            {
               *rp = (png_byte)(((int)(*rp) + (int)(*pp++)) & 0xff);
               rp++;
            }

            for (i = 0; i < istop; i++)   /* use leftover rp,pp */
            {
               int a, b, c, pa, pb, pc, p;

               a = *lp++;
               b = *pp++;
               c = *cp++;

               p = b - c;
               pc = a - c;

#ifdef PNG_USE_ABS
               pa = abs(p);
               pb = abs(pc);
               pc = abs(p + pc);
#else
               pa = p < 0 ? -p : p;
               pb = pc < 0 ? -pc : pc;
               pc = (p + pc) < 0 ? -(p + pc) : p + pc;
#endif

               /*
                  if (pa <= pb && pa <= pc)
                     p = a;
                  else if (pb <= pc)
                     p = b;
                  else
                     p = c;
                */

               p = (pa <= pb && pa <= pc) ? a : (pb <= pc) ? b : c;

               *rp = (png_byte)(((int)(*rp) + p) & 0xff);
               rp++;
            }
         }  /* end !UseMMX_paeth */
         break;

      default:
         png_warning(png_ptr, "Ignoring bad row-filter type");
         *row=0;
         break;
   }
}

#endif /* PNG_HAVE_ASSEMBLER_READ_FILTER_ROW */


/*===========================================================================*/
/*                                                                           */
/*                      P N G _ M M X _ S U P P O R T                        */
/*                                                                           */
/*===========================================================================*/

/* GRR NOTES:  (1) the following code assumes 386 or better (pushfl/popfl)
 *             (2) all instructions compile with gcc 2.7.2.3 and later
 *             (3) the function is moved down here to prevent gcc from
 *                  inlining it in multiple places and then barfing be-
 *                  cause the ".NOT_SUPPORTED" label is multiply defined
 *             [is there a way to signal that a *single* function should
 *              not be inlined?  is there a way to modify the label for
 *              each inlined instance, e.g., by appending _1, _2, etc.?
 *              maybe if don't use leading "." in label name? (nope...sigh)]
 */

int PNGAPI
png_mmx_support(void)
{
#if defined(PNG_MMX_CODE_SUPPORTED)
    int result;
    __asm__ __volatile__ (
        "pushl %%ebx          \n\t"  
        "pushl %%ecx          \n\t"  
        "pushl %%edx          \n\t"  


        "pushfl               \n\t"  
        "popl %%eax           \n\t"  
        "movl %%eax, %%ecx    \n\t"  
        "xorl $0x200000, %%eax \n\t" 
        "pushl %%eax          \n\t"  


        "popfl                \n\t"  
        "pushfl               \n\t"  
        "popl %%eax           \n\t"  
        "pushl %%ecx          \n\t"  
        "popfl                \n\t"  
        "xorl %%ecx, %%eax    \n\t"  
        "jz 0f                \n\t"  

        "xorl %%eax, %%eax    \n\t"  

        "cpuid                \n\t"  
        "cmpl $1, %%eax       \n\t"  
        "jl 0f                \n\t"  

        "xorl %%eax, %%eax    \n\t"  
        "incl %%eax           \n\t"  
                                     
        "cpuid                \n\t"  
        "andl $0x800000, %%edx \n\t" 
        "cmpl $0, %%edx       \n\t"  
        "jz 0f                \n\t"  

        "movl $1, %%eax       \n\t"  
        "jmp  1f              \n\t"  

    "0:                       \n\t"  
        "movl $0, %%eax       \n\t"  
    "1:                       \n\t"  
        "popl %%edx           \n\t"  
        "popl %%ecx           \n\t"  
        "popl %%ebx           \n\t"  


                                     

        : "=a" (result)              

        :                            

                                     



    );
    _mmx_supported = result;
#else
    _mmx_supported = 0;
#endif /* PNG_MMX_CODE_SUPPORTED */

    return _mmx_supported;
}


#endif /* PNG_USE_PNGGCCRD */
