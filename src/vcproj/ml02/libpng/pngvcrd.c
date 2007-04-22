
/* pngvcrd.c - mixed C/assembler version of utilities to read a PNG file
 *
 * For Intel x86 CPU and Microsoft Visual C++ compiler
 *
 * Last changed in libpng 1.2.6 - August 15, 2004
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1998-2004 Glenn Randers-Pehrson
 * Copyright (c) 1998, Intel Corporation
 *
 * Contributed by Nirav Chhatrapati, Intel Corporation, 1998
 * Interface to libpng contributed by Gilles Vollant, 1999
 *
 *
 * In png_do_read_interlace() in libpng versions 1.0.3a through 1.0.4d,
 * a sign error in the post-MMX cleanup code for each pixel_depth resulted
 * in bad pixels at the beginning of some rows of some images, and also
 * (due to out-of-range memory reads and writes) caused heap corruption
 * when compiled with MSVC 6.0.  The error was fixed in version 1.0.4e.
 *
 * [png_read_filter_row_mmx_avg() bpp == 2 bugfix, GRR 20000916]
 *
 * [runtime MMX configuration, GRR 20010102]
 *
 */

#define PNG_INTERNAL
#include "png.h"

#if defined(PNG_ASSEMBLER_CODE_SUPPORTED) && defined(PNG_USE_PNGVCRD)

static int mmx_supported=2;


int PNGAPI
png_mmx_support(void)
{
  int mmx_supported_local = 0;
  _asm {
    push ebx          
    push ecx
    push edx

    pushfd            
    pop eax           
    mov ecx, eax      
    xor eax, 0x200000 
    push eax          

    popfd             
    pushfd            
    pop eax           
    push ecx          
    popfd             
    xor eax, ecx      
    jz NOT_SUPPORTED  
                      
                      

    xor eax, eax      

    _asm _emit 0x0f   
    _asm _emit 0xa2

    cmp eax, 1        
    jl NOT_SUPPORTED  

    xor eax, eax      
    inc eax           
                      

    _asm _emit 0x0f   
    _asm _emit 0xa2

    and edx, 0x00800000  
    cmp edx, 0        
    jz  NOT_SUPPORTED 

    mov  mmx_supported_local, 1  

NOT_SUPPORTED:
    mov  eax, mmx_supported_local  
    pop edx          
    pop ecx
    pop ebx
  }

  
  

  mmx_supported = mmx_supported_local;
  return mmx_supported_local;
}

/* Combines the row recently read in with the previous row.
   This routine takes care of alpha and transparency if requested.
   This routine also handles the two methods of progressive display
   of interlaced images, depending on the mask value.
   The mask value describes which pixels are to be combined with
   the row.  The pattern always repeats every 8 pixels, so just 8
   bits are needed.  A one indicates the pixel is to be combined; a
   zero indicates the pixel is to be skipped.  This is in addition
   to any alpha or transparency value associated with the pixel.  If
   you want all pixels to be combined, pass 0xff (255) in mask.  */

/* Use this routine for x86 platform - uses faster MMX routine if machine
   supports MMX */

void /* PRIVATE */
png_combine_row(png_structp png_ptr, png_bytep row, int mask)
{
#ifdef PNG_USE_LOCAL_ARRAYS
   const int png_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};
#endif

   png_debug(1,"in png_combine_row_asm\n");

   if (mmx_supported == 2) {
#if !defined(PNG_1_0_X)
       /* this should have happened in png_init_mmx_flags() already */
       png_warning(png_ptr, "asm_flags may not have been initialized");
#endif
       png_mmx_support();
   }

   if (mask == 0xff)
   {
      png_memcpy(row, png_ptr->row_buf + 1,
       (png_size_t)PNG_ROWBYTES(png_ptr->row_info.pixel_depth,
       png_ptr->width));
   }
   /* GRR:  add "else if (mask == 0)" case?
    *       or does png_combine_row() not even get called in that case? */
   else
   {
      switch (png_ptr->row_info.pixel_depth)
      {
         case 1:
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

         case 2:
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

         case 4:
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

         case 8:
         {
            png_bytep srcptr;
            png_bytep dstptr;
            png_uint_32 len;
            int m;
            int diff, unmask;

            __int64 mask0=0x0102040810204080;

#if !defined(PNG_1_0_X)
            if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_COMBINE_ROW)
                /* && mmx_supported */ )
#else
            if (mmx_supported)
#endif
            {
               srcptr = png_ptr->row_buf + 1;
               dstptr = row;
               m = 0x80;
               unmask = ~mask;
               len  = png_ptr->width &~7;  
               diff = png_ptr->width & 7;  

               _asm
               {
                  movd       mm7, unmask   
                  psubb      mm6,mm6       
                  punpcklbw  mm7,mm7
                  punpcklwd  mm7,mm7
                  punpckldq  mm7,mm7       

                  movq       mm0,mask0

                  pand       mm0,mm7       
                  pcmpeqb    mm0,mm6       

                  mov        ecx,len       
                  mov        esi,srcptr    
                  mov        ebx,dstptr    
                  cmp        ecx,0         
                  je         mainloop8end

mainloop8:
                  movq       mm4,[esi]
                  pand       mm4,mm0
                  movq       mm6,mm0
                  pandn      mm6,[ebx]
                  por        mm4,mm6
                  movq       [ebx],mm4

                  add        esi,8         
                  add        ebx,8
                  sub        ecx,8         

                  ja         mainloop8
mainloop8end:

                  mov        ecx,diff
                  cmp        ecx,0
                  jz         end8

                  mov        edx,mask
                  sal        edx,24        

secondloop8:
                  sal        edx,1         
                  jnc        skip8         
                  mov        al,[esi]
                  mov        [ebx],al
skip8:
                  inc        esi
                  inc        ebx

                  dec        ecx
                  jnz        secondloop8
end8:
                  emms
               }
            }
            else /* mmx not supported - use modified C routine */
            {
               register unsigned int incr1, initial_val, final_val;
               png_size_t pixel_bytes;
               png_uint_32 i;
               register int disp = png_pass_inc[png_ptr->pass];
               int offset_table[7] = {0, 4, 0, 2, 0, 1, 0};

               pixel_bytes = (png_ptr->row_info.pixel_depth >> 3);
               srcptr = png_ptr->row_buf + 1 + offset_table[png_ptr->pass]*
                  pixel_bytes;
               dstptr = row + offset_table[png_ptr->pass]*pixel_bytes;
               initial_val = offset_table[png_ptr->pass]*pixel_bytes;
               final_val = png_ptr->width*pixel_bytes;
               incr1 = (disp)*pixel_bytes;
               for (i = initial_val; i < final_val; i += incr1)
               {
                  png_memcpy(dstptr, srcptr, pixel_bytes);
                  srcptr += incr1;
                  dstptr += incr1;
               }
            } /* end of else */

            break;
         }       

         case 16:
         {
            png_bytep srcptr;
            png_bytep dstptr;
            png_uint_32 len;
            int unmask, diff;
            __int64 mask1=0x0101020204040808,
                    mask0=0x1010202040408080;

#if !defined(PNG_1_0_X)
            if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_COMBINE_ROW)
                /* && mmx_supported */ )
#else
            if (mmx_supported)
#endif
            {
               srcptr = png_ptr->row_buf + 1;
               dstptr = row;

               unmask = ~mask;
               len     = (png_ptr->width)&~7;
               diff = (png_ptr->width)&7;
               _asm
               {
                  movd       mm7, unmask       
                  psubb      mm6,mm6           
                  punpcklbw  mm7,mm7
                  punpcklwd  mm7,mm7
                  punpckldq  mm7,mm7           

                  movq       mm0,mask0
                  movq       mm1,mask1

                  pand       mm0,mm7
                  pand       mm1,mm7

                  pcmpeqb    mm0,mm6
                  pcmpeqb    mm1,mm6

                  mov        ecx,len           
                  mov        esi,srcptr        
                  mov        ebx,dstptr        
                  cmp        ecx,0             
                  jz         mainloop16end

mainloop16:
                  movq       mm4,[esi]
                  pand       mm4,mm0
                  movq       mm6,mm0
                  movq       mm7,[ebx]
                  pandn      mm6,mm7
                  por        mm4,mm6
                  movq       [ebx],mm4

                  movq       mm5,[esi+8]
                  pand       mm5,mm1
                  movq       mm7,mm1
                  movq       mm6,[ebx+8]
                  pandn      mm7,mm6
                  por        mm5,mm7
                  movq       [ebx+8],mm5

                  add        esi,16            
                  add        ebx,16
                  sub        ecx,8             

                  ja         mainloop16

mainloop16end:
                  mov        ecx,diff
                  cmp        ecx,0
                  jz         end16

                  mov        edx,mask
                  sal        edx,24            
secondloop16:
                  sal        edx,1             
                  jnc        skip16            
                  mov        ax,[esi]
                  mov        [ebx],ax
skip16:
                  add        esi,2
                  add        ebx,2

                  dec        ecx
                  jnz        secondloop16
end16:
                  emms
               }
            }
            else /* mmx not supported - use modified C routine */
            {
               register unsigned int incr1, initial_val, final_val;
               png_size_t pixel_bytes;
               png_uint_32 i;
               register int disp = png_pass_inc[png_ptr->pass];
               int offset_table[7] = {0, 4, 0, 2, 0, 1, 0};

               pixel_bytes = (png_ptr->row_info.pixel_depth >> 3);
               srcptr = png_ptr->row_buf + 1 + offset_table[png_ptr->pass]*
                  pixel_bytes;
               dstptr = row + offset_table[png_ptr->pass]*pixel_bytes;
               initial_val = offset_table[png_ptr->pass]*pixel_bytes;
               final_val = png_ptr->width*pixel_bytes;
               incr1 = (disp)*pixel_bytes;
               for (i = initial_val; i < final_val; i += incr1)
               {
                  png_memcpy(dstptr, srcptr, pixel_bytes);
                  srcptr += incr1;
                  dstptr += incr1;
               }
            } /* end of else */

            break;
         }       

         case 24:
         {
            png_bytep srcptr;
            png_bytep dstptr;
            png_uint_32 len;
            int unmask, diff;

            __int64 mask2=0x0101010202020404,  
                    mask1=0x0408080810101020,
                    mask0=0x2020404040808080;

            srcptr = png_ptr->row_buf + 1;
            dstptr = row;

            unmask = ~mask;
            len     = (png_ptr->width)&~7;
            diff = (png_ptr->width)&7;

#if !defined(PNG_1_0_X)
            if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_COMBINE_ROW)
                /* && mmx_supported */ )
#else
            if (mmx_supported)
#endif
            {
               _asm
               {
                  movd       mm7, unmask       
                  psubb      mm6,mm6           
                  punpcklbw  mm7,mm7
                  punpcklwd  mm7,mm7
                  punpckldq  mm7,mm7           

                  movq       mm0,mask0
                  movq       mm1,mask1
                  movq       mm2,mask2

                  pand       mm0,mm7
                  pand       mm1,mm7
                  pand       mm2,mm7

                  pcmpeqb    mm0,mm6
                  pcmpeqb    mm1,mm6
                  pcmpeqb    mm2,mm6

                  mov        ecx,len           
                  mov        esi,srcptr        
                  mov        ebx,dstptr        
                  cmp        ecx,0
                  jz         mainloop24end

mainloop24:
                  movq       mm4,[esi]
                  pand       mm4,mm0
                  movq       mm6,mm0
                  movq       mm7,[ebx]
                  pandn      mm6,mm7
                  por        mm4,mm6
                  movq       [ebx],mm4


                  movq       mm5,[esi+8]
                  pand       mm5,mm1
                  movq       mm7,mm1
                  movq       mm6,[ebx+8]
                  pandn      mm7,mm6
                  por        mm5,mm7
                  movq       [ebx+8],mm5

                  movq       mm6,[esi+16]
                  pand       mm6,mm2
                  movq       mm4,mm2
                  movq       mm7,[ebx+16]
                  pandn      mm4,mm7
                  por        mm6,mm4
                  movq       [ebx+16],mm6

                  add        esi,24            
                  add        ebx,24
                  sub        ecx,8             

                  ja         mainloop24

mainloop24end:
                  mov        ecx,diff
                  cmp        ecx,0
                  jz         end24

                  mov        edx,mask
                  sal        edx,24            
secondloop24:
                  sal        edx,1             
                  jnc        skip24            
                  mov        ax,[esi]
                  mov        [ebx],ax
                  xor        eax,eax
                  mov        al,[esi+2]
                  mov        [ebx+2],al
skip24:
                  add        esi,3
                  add        ebx,3

                  dec        ecx
                  jnz        secondloop24

end24:
                  emms
               }
            }
            else /* mmx not supported - use modified C routine */
            {
               register unsigned int incr1, initial_val, final_val;
               png_size_t pixel_bytes;
               png_uint_32 i;
               register int disp = png_pass_inc[png_ptr->pass];
               int offset_table[7] = {0, 4, 0, 2, 0, 1, 0};

               pixel_bytes = (png_ptr->row_info.pixel_depth >> 3);
               srcptr = png_ptr->row_buf + 1 + offset_table[png_ptr->pass]*
                  pixel_bytes;
               dstptr = row + offset_table[png_ptr->pass]*pixel_bytes;
               initial_val = offset_table[png_ptr->pass]*pixel_bytes;
               final_val = png_ptr->width*pixel_bytes;
               incr1 = (disp)*pixel_bytes;
               for (i = initial_val; i < final_val; i += incr1)
               {
                  png_memcpy(dstptr, srcptr, pixel_bytes);
                  srcptr += incr1;
                  dstptr += incr1;
               }
            } /* end of else */

            break;
         }       

         case 32:
         {
            png_bytep srcptr;
            png_bytep dstptr;
            png_uint_32 len;
            int unmask, diff;

            __int64 mask3=0x0101010102020202,  
                    mask2=0x0404040408080808,
                    mask1=0x1010101020202020,
                    mask0=0x4040404080808080;

            srcptr = png_ptr->row_buf + 1;
            dstptr = row;

            unmask = ~mask;
            len     = (png_ptr->width)&~7;
            diff = (png_ptr->width)&7;

#if !defined(PNG_1_0_X)
            if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_COMBINE_ROW)
                /* && mmx_supported */ )
#else
            if (mmx_supported)
#endif
            {
               _asm
               {
                  movd       mm7, unmask       
                  psubb      mm6,mm6           
                  punpcklbw  mm7,mm7
                  punpcklwd  mm7,mm7
                  punpckldq  mm7,mm7           

                  movq       mm0,mask0
                  movq       mm1,mask1
                  movq       mm2,mask2
                  movq       mm3,mask3

                  pand       mm0,mm7
                  pand       mm1,mm7
                  pand       mm2,mm7
                  pand       mm3,mm7

                  pcmpeqb    mm0,mm6
                  pcmpeqb    mm1,mm6
                  pcmpeqb    mm2,mm6
                  pcmpeqb    mm3,mm6

                  mov        ecx,len           
                  mov        esi,srcptr        
                  mov        ebx,dstptr        

                  cmp        ecx,0             
                  jz         mainloop32end

mainloop32:
                  movq       mm4,[esi]
                  pand       mm4,mm0
                  movq       mm6,mm0
                  movq       mm7,[ebx]
                  pandn      mm6,mm7
                  por        mm4,mm6
                  movq       [ebx],mm4

                  movq       mm5,[esi+8]
                  pand       mm5,mm1
                  movq       mm7,mm1
                  movq       mm6,[ebx+8]
                  pandn      mm7,mm6
                  por        mm5,mm7
                  movq       [ebx+8],mm5

                  movq       mm6,[esi+16]
                  pand       mm6,mm2
                  movq       mm4,mm2
                  movq       mm7,[ebx+16]
                  pandn      mm4,mm7
                  por        mm6,mm4
                  movq       [ebx+16],mm6

                  movq       mm7,[esi+24]
                  pand       mm7,mm3
                  movq       mm5,mm3
                  movq       mm4,[ebx+24]
                  pandn      mm5,mm4
                  por        mm7,mm5
                  movq       [ebx+24],mm7

                  add        esi,32            
                  add        ebx,32
                  sub        ecx,8             

                  ja         mainloop32

mainloop32end:
                  mov        ecx,diff
                  cmp        ecx,0
                  jz         end32

                  mov        edx,mask
                  sal        edx,24            
secondloop32:
                  sal        edx,1             
                  jnc        skip32            
                  mov        eax,[esi]
                  mov        [ebx],eax
skip32:
                  add        esi,4
                  add        ebx,4

                  dec        ecx
                  jnz        secondloop32

end32:
                  emms
               }
            }
            else /* mmx _not supported - Use modified C routine */
            {
               register unsigned int incr1, initial_val, final_val;
               png_size_t pixel_bytes;
               png_uint_32 i;
               register int disp = png_pass_inc[png_ptr->pass];
               int offset_table[7] = {0, 4, 0, 2, 0, 1, 0};

               pixel_bytes = (png_ptr->row_info.pixel_depth >> 3);
               srcptr = png_ptr->row_buf + 1 + offset_table[png_ptr->pass]*
                  pixel_bytes;
               dstptr = row + offset_table[png_ptr->pass]*pixel_bytes;
               initial_val = offset_table[png_ptr->pass]*pixel_bytes;
               final_val = png_ptr->width*pixel_bytes;
               incr1 = (disp)*pixel_bytes;
               for (i = initial_val; i < final_val; i += incr1)
               {
                  png_memcpy(dstptr, srcptr, pixel_bytes);
                  srcptr += incr1;
                  dstptr += incr1;
               }
            } /* end of else */

            break;
         }       

         case 48:
         {
            png_bytep srcptr;
            png_bytep dstptr;
            png_uint_32 len;
            int unmask, diff;

            __int64 mask5=0x0101010101010202,
                    mask4=0x0202020204040404,
                    mask3=0x0404080808080808,
                    mask2=0x1010101010102020,
                    mask1=0x2020202040404040,
                    mask0=0x4040808080808080;

#if !defined(PNG_1_0_X)
            if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_COMBINE_ROW)
                /* && mmx_supported */ )
#else
            if (mmx_supported)
#endif
            {
               srcptr = png_ptr->row_buf + 1;
               dstptr = row;

               unmask = ~mask;
               len     = (png_ptr->width)&~7;
               diff = (png_ptr->width)&7;
               _asm
               {
                  movd       mm7, unmask       
                  psubb      mm6,mm6           
                  punpcklbw  mm7,mm7
                  punpcklwd  mm7,mm7
                  punpckldq  mm7,mm7           

                  movq       mm0,mask0
                  movq       mm1,mask1
                  movq       mm2,mask2
                  movq       mm3,mask3
                  movq       mm4,mask4
                  movq       mm5,mask5

                  pand       mm0,mm7
                  pand       mm1,mm7
                  pand       mm2,mm7
                  pand       mm3,mm7
                  pand       mm4,mm7
                  pand       mm5,mm7

                  pcmpeqb    mm0,mm6
                  pcmpeqb    mm1,mm6
                  pcmpeqb    mm2,mm6
                  pcmpeqb    mm3,mm6
                  pcmpeqb    mm4,mm6
                  pcmpeqb    mm5,mm6

                  mov        ecx,len           
                  mov        esi,srcptr        
                  mov        ebx,dstptr        

                  cmp        ecx,0
                  jz         mainloop48end

mainloop48:
                  movq       mm7,[esi]
                  pand       mm7,mm0
                  movq       mm6,mm0
                  pandn      mm6,[ebx]
                  por        mm7,mm6
                  movq       [ebx],mm7

                  movq       mm6,[esi+8]
                  pand       mm6,mm1
                  movq       mm7,mm1
                  pandn      mm7,[ebx+8]
                  por        mm6,mm7
                  movq       [ebx+8],mm6

                  movq       mm6,[esi+16]
                  pand       mm6,mm2
                  movq       mm7,mm2
                  pandn      mm7,[ebx+16]
                  por        mm6,mm7
                  movq       [ebx+16],mm6

                  movq       mm7,[esi+24]
                  pand       mm7,mm3
                  movq       mm6,mm3
                  pandn      mm6,[ebx+24]
                  por        mm7,mm6
                  movq       [ebx+24],mm7

                  movq       mm6,[esi+32]
                  pand       mm6,mm4
                  movq       mm7,mm4
                  pandn      mm7,[ebx+32]
                  por        mm6,mm7
                  movq       [ebx+32],mm6

                  movq       mm7,[esi+40]
                  pand       mm7,mm5
                  movq       mm6,mm5
                  pandn      mm6,[ebx+40]
                  por        mm7,mm6
                  movq       [ebx+40],mm7

                  add        esi,48            
                  add        ebx,48
                  sub        ecx,8             

                  ja         mainloop48
mainloop48end:

                  mov        ecx,diff
                  cmp        ecx,0
                  jz         end48

                  mov        edx,mask
                  sal        edx,24            

secondloop48:
                  sal        edx,1             
                  jnc        skip48            
                  mov        eax,[esi]
                  mov        [ebx],eax
skip48:
                  add        esi,4
                  add        ebx,4

                  dec        ecx
                  jnz        secondloop48

end48:
                  emms
               }
            }
            else /* mmx _not supported - Use modified C routine */
            {
               register unsigned int incr1, initial_val, final_val;
               png_size_t pixel_bytes;
               png_uint_32 i;
               register int disp = png_pass_inc[png_ptr->pass];
               int offset_table[7] = {0, 4, 0, 2, 0, 1, 0};

               pixel_bytes = (png_ptr->row_info.pixel_depth >> 3);
               srcptr = png_ptr->row_buf + 1 + offset_table[png_ptr->pass]*
                  pixel_bytes;
               dstptr = row + offset_table[png_ptr->pass]*pixel_bytes;
               initial_val = offset_table[png_ptr->pass]*pixel_bytes;
               final_val = png_ptr->width*pixel_bytes;
               incr1 = (disp)*pixel_bytes;
               for (i = initial_val; i < final_val; i += incr1)
               {
                  png_memcpy(dstptr, srcptr, pixel_bytes);
                  srcptr += incr1;
                  dstptr += incr1;
               }
            } /* end of else */

            break;
         }       

         default:
         {
            png_bytep sptr;
            png_bytep dp;
            png_size_t pixel_bytes;
            int offset_table[7] = {0, 4, 0, 2, 0, 1, 0};
            unsigned int i;
            register int disp = png_pass_inc[png_ptr->pass];  
            register unsigned int incr1, initial_val, final_val;

            pixel_bytes = (png_ptr->row_info.pixel_depth >> 3);
            sptr = png_ptr->row_buf + 1 + offset_table[png_ptr->pass]*
               pixel_bytes;
            dp = row + offset_table[png_ptr->pass]*pixel_bytes;
            initial_val = offset_table[png_ptr->pass]*pixel_bytes;
            final_val = png_ptr->width*pixel_bytes;
            incr1 = (disp)*pixel_bytes;
            for (i = initial_val; i < final_val; i += incr1)
            {
               png_memcpy(dp, sptr, pixel_bytes);
               sptr += incr1;
               dp += incr1;
            }
            break;
         }
      } /* end switch (png_ptr->row_info.pixel_depth) */
   } /* end if (non-trivial mask) */

} /* end png_combine_row() */


#if defined(PNG_READ_INTERLACING_SUPPORTED)

void /* PRIVATE */
png_do_read_interlace(png_structp png_ptr)
{
   png_row_infop row_info = &(png_ptr->row_info);
   png_bytep row = png_ptr->row_buf + 1;
   int pass = png_ptr->pass;
   png_uint_32 transformations = png_ptr->transformations;
#ifdef PNG_USE_LOCAL_ARRAYS
   const int png_pass_inc[7] = {8, 8, 4, 4, 2, 2, 1};
#endif

   png_debug(1,"in png_do_read_interlace\n");

   if (mmx_supported == 2) {
#if !defined(PNG_1_0_X)
       /* this should have happened in png_init_mmx_flags() already */
       png_warning(png_ptr, "asm_flags may not have been initialized");
#endif
       png_mmx_support();
   }

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

         default:         
         {
            __int64 const4 = 0x0000000000FFFFFF;
            
            __int64 const6 = 0x00000000000000FF;
            png_bytep sptr, dp;
            png_uint_32 i;
            png_size_t pixel_bytes;
            int width = row_info->width;

            pixel_bytes = (row_info->pixel_depth >> 3);

            sptr = row + (width - 1) * pixel_bytes;
            dp = row + (final_width - 1) * pixel_bytes;
            
            
            

            
#if !defined(PNG_1_0_X)
            if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_INTERLACE)
                /* && mmx_supported */ )
#else
            if (mmx_supported)
#endif
            {
               if (pixel_bytes == 3)
               {
                  if (((pass == 0) || (pass == 1)) && width)
                  {
                     _asm
                     {
                        mov esi, sptr
                        mov edi, dp
                        mov ecx, width
                        sub edi, 21   
loop_pass0:
                        movd mm0, [esi]     ; X X X X X v2 v1 v0
                        pand mm0, const4    ; 0 0 0 0 0 v2 v1 v0
                        movq mm1, mm0       ; 0 0 0 0 0 v2 v1 v0
                        psllq mm0, 16       ; 0 0 0 v2 v1 v0 0 0
                        movq mm2, mm0       ; 0 0 0 v2 v1 v0 0 0
                        psllq mm0, 24       ; v2 v1 v0 0 0 0 0 0
                        psrlq mm1, 8        ; 0 0 0 0 0 0 v2 v1
                        por mm0, mm2        ; v2 v1 v0 v2 v1 v0 0 0
                        por mm0, mm1        ; v2 v1 v0 v2 v1 v0 v2 v1
                        movq mm3, mm0       ; v2 v1 v0 v2 v1 v0 v2 v1
                        psllq mm0, 16       ; v0 v2 v1 v0 v2 v1 0 0
                        movq mm4, mm3       ; v2 v1 v0 v2 v1 v0 v2 v1
                        punpckhdq mm3, mm0  ; v0 v2 v1 v0 v2 v1 v0 v2
                        movq [edi+16] , mm4
                        psrlq mm0, 32       ; 0 0 0 0 v0 v2 v1 v0
                        movq [edi+8] , mm3
                        punpckldq mm0, mm4  ; v1 v0 v2 v1 v0 v2 v1 v0
                        sub esi, 3
                        movq [edi], mm0
                        sub edi, 24
                        
                        dec ecx
                        jnz loop_pass0
                        EMMS
                     }
                  }
                  else if (((pass == 2) || (pass == 3)) && width)
                  {
                     _asm
                     {
                        mov esi, sptr
                        mov edi, dp
                        mov ecx, width
                        sub edi, 9   
loop_pass2:
                        movd mm0, [esi]     ; X X X X X v2 v1 v0
                        pand mm0, const4    ; 0 0 0 0 0 v2 v1 v0
                        movq mm1, mm0       ; 0 0 0 0 0 v2 v1 v0
                        psllq mm0, 16       ; 0 0 0 v2 v1 v0 0 0
                        movq mm2, mm0       ; 0 0 0 v2 v1 v0 0 0
                        psllq mm0, 24       ; v2 v1 v0 0 0 0 0 0
                        psrlq mm1, 8        ; 0 0 0 0 0 0 v2 v1
                        por mm0, mm2        ; v2 v1 v0 v2 v1 v0 0 0
                        por mm0, mm1        ; v2 v1 v0 v2 v1 v0 v2 v1
                        movq [edi+4], mm0   ; move to memory
                        psrlq mm0, 16       ; 0 0 v2 v1 v0 v2 v1 v0
                        movd [edi], mm0     ; move to memory
                        sub esi, 3
                        sub edi, 12
                        dec ecx
                        jnz loop_pass2
                        EMMS
                     }
                  }
                  else if (width) /* && ((pass == 4) || (pass == 5)) */
                  {
                     int width_mmx = ((width >> 1) << 1) - 8;
                     if (width_mmx < 0)
                         width_mmx = 0;
                     width -= width_mmx;        
                     if (width_mmx)
                     {
                        _asm
                        {
                           mov esi, sptr
                           mov edi, dp
                           mov ecx, width_mmx
                           sub esi, 3
                           sub edi, 9
loop_pass4:
                           movq mm0, [esi]     ; X X v2 v1 v0 v5 v4 v3
                           movq mm7, mm0       ; X X v2 v1 v0 v5 v4 v3
                           movq mm6, mm0       ; X X v2 v1 v0 v5 v4 v3
                           psllq mm0, 24       ; v1 v0 v5 v4 v3 0 0 0
                           pand mm7, const4    ; 0 0 0 0 0 v5 v4 v3
                           psrlq mm6, 24       ; 0 0 0 X X v2 v1 v0
                           por mm0, mm7        ; v1 v0 v5 v4 v3 v5 v4 v3
                           movq mm5, mm6       ; 0 0 0 X X v2 v1 v0
                           psllq mm6, 8        ; 0 0 X X v2 v1 v0 0
                           movq [edi], mm0     ; move quad to memory
                           psrlq mm5, 16       ; 0 0 0 0 0 X X v2
                           pand mm5, const6    ; 0 0 0 0 0 0 0 v2
                           por mm6, mm5        ; 0 0 X X v2 v1 v0 v2
                           movd [edi+8], mm6   ; move double to memory
                           sub esi, 6
                           sub edi, 12
                           sub ecx, 2
                           jnz loop_pass4
                           EMMS
                        }
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
                        _asm
                        {
                           mov esi, sptr
                           mov edi, dp
                           mov ecx, width_mmx
                           sub edi, 31
                           sub esi, 3
loop1_pass0:
                           movd mm0, [esi]     ; X X X X v0 v1 v2 v3
                           movq mm1, mm0       ; X X X X v0 v1 v2 v3
                           punpcklbw mm0, mm0  ; v0 v0 v1 v1 v2 v2 v3 v3
                           movq mm2, mm0       ; v0 v0 v1 v1 v2 v2 v3 v3
                           punpcklwd mm0, mm0  ; v2 v2 v2 v2 v3 v3 v3 v3
                           movq mm3, mm0       ; v2 v2 v2 v2 v3 v3 v3 v3
                           punpckldq mm0, mm0  ; v3 v3 v3 v3 v3 v3 v3 v3
                           punpckhdq mm3, mm3  ; v2 v2 v2 v2 v2 v2 v2 v2
                           movq [edi], mm0     ; move to memory v3
                           punpckhwd mm2, mm2  ; v0 v0 v0 v0 v1 v1 v1 v1
                           movq [edi+8], mm3   ; move to memory v2
                           movq mm4, mm2       ; v0 v0 v0 v0 v1 v1 v1 v1
                           punpckldq mm2, mm2  ; v1 v1 v1 v1 v1 v1 v1 v1
                           punpckhdq mm4, mm4  ; v0 v0 v0 v0 v0 v0 v0 v0
                           movq [edi+16], mm2  ; move to memory v1
                           movq [edi+24], mm4  ; move to memory v0
                           sub esi, 4
                           sub edi, 32
                           sub ecx, 4
                           jnz loop1_pass0
                           EMMS
                        }
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
                           *dp-- = *sptr;
                        sptr--;
                     }
                  }
                  else if (((pass == 2) || (pass == 3)) && width)
                  {
                     int width_mmx = ((width >> 2) << 2);
                     width -= width_mmx;
                     if (width_mmx)
                     {
                        _asm
                        {
                           mov esi, sptr
                           mov edi, dp
                           mov ecx, width_mmx
                           sub edi, 15
                           sub esi, 3
loop1_pass2:
                           movd mm0, [esi]     ; X X X X v0 v1 v2 v3
                           punpcklbw mm0, mm0  ; v0 v0 v1 v1 v2 v2 v3 v3
                           movq mm1, mm0       ; v0 v0 v1 v1 v2 v2 v3 v3
                           punpcklwd mm0, mm0  ; v2 v2 v2 v2 v3 v3 v3 v3
                           punpckhwd mm1, mm1  ; v0 v0 v0 v0 v1 v1 v1 v1
                           movq [edi], mm0     ; move to memory v2 and v3
                           sub esi, 4
                           movq [edi+8], mm1   ; move to memory v1     and v0
                           sub edi, 16
                           sub ecx, 4
                           jnz loop1_pass2
                           EMMS
                        }
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
                        sptr --;
                     }
                  }
                  else if (width) /* && ((pass == 4) || (pass == 5))) */
                  {
                     int width_mmx = ((width >> 3) << 3);
                     width -= width_mmx;
                     if (width_mmx)
                     {
                        _asm
                        {
                           mov esi, sptr
                           mov edi, dp
                           mov ecx, width_mmx
                           sub edi, 15
                           sub esi, 7
loop1_pass4:
                           movq mm0, [esi]     ; v0 v1 v2 v3 v4 v5 v6 v7
                           movq mm1, mm0       ; v0 v1 v2 v3 v4 v5 v6 v7
                           punpcklbw mm0, mm0  ; v4 v4 v5 v5 v6 v6 v7 v7
                           
                           punpckhbw mm1, mm1  ;v0 v0 v1 v1 v2 v2 v3 v3
                           movq [edi+8], mm1   ; move to memory v0 v1 v2 and v3
                           sub esi, 8
                           movq [edi], mm0     ; move to memory v4 v5 v6 and v7
                           
                           sub edi, 16
                           sub ecx, 8
                           jnz loop1_pass4
                           EMMS
                        }
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
                        sptr --;
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
                        _asm
                        {
                           mov esi, sptr
                           mov edi, dp
                           mov ecx, width_mmx
                           sub esi, 2
                           sub edi, 30
loop2_pass0:
                           movd mm0, [esi]        ; X X X X v1 v0 v3 v2
                           punpcklwd mm0, mm0     ; v1 v0 v1 v0 v3 v2 v3 v2
                           movq mm1, mm0          ; v1 v0 v1 v0 v3 v2 v3 v2
                           punpckldq mm0, mm0     ; v3 v2 v3 v2 v3 v2 v3 v2
                           punpckhdq mm1, mm1     ; v1 v0 v1 v0 v1 v0 v1 v0
                           movq [edi], mm0
                           movq [edi + 8], mm0
                           movq [edi + 16], mm1
                           movq [edi + 24], mm1
                           sub esi, 4
                           sub edi, 32
                           sub ecx, 2
                           jnz loop2_pass0
                           EMMS
                        }
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
                        _asm
                        {
                           mov esi, sptr
                           mov edi, dp
                           mov ecx, width_mmx
                           sub esi, 2
                           sub edi, 14
loop2_pass2:
                           movd mm0, [esi]        ; X X X X v1 v0 v3 v2
                           punpcklwd mm0, mm0     ; v1 v0 v1 v0 v3 v2 v3 v2
                           movq mm1, mm0          ; v1 v0 v1 v0 v3 v2 v3 v2
                           punpckldq mm0, mm0     ; v3 v2 v3 v2 v3 v2 v3 v2
                           punpckhdq mm1, mm1     ; v1 v0 v1 v0 v1 v0 v1 v0
                           movq [edi], mm0
                           sub esi, 4
                           movq [edi + 8], mm1
                           
                           sub edi, 16
                           sub ecx, 2
                           jnz loop2_pass2
                           EMMS
                        }
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
                        _asm
                        {
                           mov esi, sptr
                           mov edi, dp
                           mov ecx, width_mmx
                           sub esi, 2
                           sub edi, 6
loop2_pass4:
                           movd mm0, [esi]        ; X X X X v1 v0 v3 v2
                           punpcklwd mm0, mm0     ; v1 v0 v1 v0 v3 v2 v3 v2
                           sub esi, 4
                           movq [edi], mm0
                           sub edi, 8
                           sub ecx, 2
                           jnz loop2_pass4
                           EMMS
                        }
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
                     int width_mmx = ((width >> 1) << 1) ;
                     width -= width_mmx;
                     if (width_mmx)
                     {
                        _asm
                        {
                           mov esi, sptr
                           mov edi, dp
                           mov ecx, width_mmx
                           sub esi, 4
                           sub edi, 60
loop4_pass0:
                           movq mm0, [esi]        ; v3 v2 v1 v0 v7 v6 v5 v4
                           movq mm1, mm0          ; v3 v2 v1 v0 v7 v6 v5 v4
                           punpckldq mm0, mm0     ; v7 v6 v5 v4 v7 v6 v5 v4
                           punpckhdq mm1, mm1     ; v3 v2 v1 v0 v3 v2 v1 v0
                           movq [edi], mm0
                           movq [edi + 8], mm0
                           movq [edi + 16], mm0
                           movq [edi + 24], mm0
                           movq [edi+32], mm1
                           movq [edi + 40], mm1
                           movq [edi+ 48], mm1
                           sub esi, 8
                           movq [edi + 56], mm1
                           sub edi, 64
                           sub ecx, 2
                           jnz loop4_pass0
                           EMMS
                        }
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
                     int width_mmx = ((width >> 1) << 1) ;
                     width -= width_mmx;
                     if (width_mmx)
                     {
                        _asm
                        {
                           mov esi, sptr
                           mov edi, dp
                           mov ecx, width_mmx
                           sub esi, 4
                           sub edi, 28
loop4_pass2:
                           movq mm0, [esi]      ; v3 v2 v1 v0 v7 v6 v5 v4
                           movq mm1, mm0        ; v3 v2 v1 v0 v7 v6 v5 v4
                           punpckldq mm0, mm0   ; v7 v6 v5 v4 v7 v6 v5 v4
                           punpckhdq mm1, mm1   ; v3 v2 v1 v0 v3 v2 v1 v0
                           movq [edi], mm0
                           movq [edi + 8], mm0
                           movq [edi+16], mm1
                           movq [edi + 24], mm1
                           sub esi, 8
                           sub edi, 32
                           sub ecx, 2
                           jnz loop4_pass2
                           EMMS
                        }
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
                        _asm
                        {
                           mov esi, sptr
                           mov edi, dp
                           mov ecx, width_mmx
                           sub esi, 4
                           sub edi, 12
loop4_pass4:
                           movq mm0, [esi]      ; v3 v2 v1 v0 v7 v6 v5 v4
                           movq mm1, mm0        ; v3 v2 v1 v0 v7 v6 v5 v4
                           punpckldq mm0, mm0   ; v7 v6 v5 v4 v7 v6 v5 v4
                           punpckhdq mm1, mm1   ; v3 v2 v1 v0 v3 v2 v1 v0
                           movq [edi], mm0
                           sub esi, 8
                           movq [edi + 8], mm1
                           sub edi, 16
                           sub ecx, 2
                           jnz loop4_pass4
                           EMMS
                        }
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
            } /* end of mmx_supported */

            else /* MMX not supported:  use modified C code - takes advantage
                  * of inlining of memcpy for a constant */
            {
               if (pixel_bytes == 1)
               {
                  for (i = width; i; i--)
                  {
                     int j;
                     for (j = 0; j < png_pass_inc[pass]; j++)
                        *dp-- = *sptr;
                     sptr--;
                  }
               }
               else if (pixel_bytes == 3)
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
               else if (pixel_bytes == 2)
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
               else if (pixel_bytes == 4)
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
               else if (pixel_bytes == 6)
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
                     sptr -= pixel_bytes;
                  }
               }

            } /* end of MMX not supported */
            break;
         }
      } /* end switch (row_info->pixel_depth) */

      row_info->width = final_width;

      row_info->rowbytes = PNG_ROWBYTES(row_info->pixel_depth,final_width);
   }

}

#endif /* PNG_READ_INTERLACING_SUPPORTED */





union uAll {
   __int64 use;
   double  align;
} LBCarryMask = {0x0101010101010101},
  HBClearMask = {0x7f7f7f7f7f7f7f7f},
  ActiveMask, ActiveMask2, ActiveMaskEnd, ShiftBpp, ShiftRem;



void /* PRIVATE */
png_read_filter_row_mmx_avg(png_row_infop row_info, png_bytep row
                            , png_bytep prev_row)
{
   int bpp;
   png_uint_32 FullLength;
   png_uint_32 MMXLength;
   
   int diff;

   bpp = (row_info->pixel_depth + 7) >> 3; 
   FullLength  = row_info->rowbytes; 
   _asm {
         
         mov edi, row          
         xor ebx, ebx          
         mov edx, edi
         mov esi, prev_row           
         sub edx, bpp          

         xor eax, eax
         
         
davgrlp:
         mov al, [esi + ebx]   
         inc ebx
         shr al, 1             
         add al, [edi+ebx-1]   
         cmp ebx, bpp
         mov [edi+ebx-1], al    
                            
         jb davgrlp
         
         mov diff, edi         
         add diff, ebx         
         add diff, 0xf         
         and diff, 0xfffffff8  
         sub diff, edi         
         jz davggo
         
         
         
         xor ecx, ecx
davglp1:
         xor eax, eax
         mov cl, [esi + ebx]        
         mov al, [edx + ebx]  
         add ax, cx
         inc ebx
         shr ax, 1            
         add al, [edi+ebx-1]  
         cmp ebx, diff              
         mov [edi+ebx-1], al        
                            
         jb davglp1               
davggo:
         mov eax, FullLength
         mov ecx, eax
         sub eax, ebx          
         and eax, 0x00000007   
         sub ecx, eax          
         mov MMXLength, ecx
   } 
   
   switch ( bpp )
   {
      case 3:
      {
         ActiveMask.use  = 0x0000000000ffffff;
         ShiftBpp.use = 24;    
         ShiftRem.use = 40;    
         _asm {
            
            movq mm7, ActiveMask
            mov ebx, diff      
            movq mm5, LBCarryMask
            mov edi, row       
            movq mm4, HBClearMask
            mov esi, prev_row        
            
            movq mm2, [edi + ebx - 8]  
                               
davg3lp:
            movq mm0, [edi + ebx]      
            
            movq mm3, mm5
            psrlq mm2, ShiftRem      
            movq mm1, [esi + ebx]    
            movq mm6, mm7
            pand mm3, mm1      
            psrlq mm1, 1       
            pand  mm1, mm4     
            paddb mm0, mm1     
            
            movq mm1, mm3      
            pand mm1, mm2      
                               
            psrlq mm2, 1       
            pand  mm2, mm4     
            paddb mm2, mm1     
            pand mm2, mm6      
            paddb mm0, mm2     
                               
            
            psllq mm6, ShiftBpp  
            movq mm2, mm0        
            psllq mm2, ShiftBpp  
            movq mm1, mm3        
            pand mm1, mm2      
                               
            psrlq mm2, 1       
            pand  mm2, mm4     
            paddb mm2, mm1     
            pand mm2, mm6      
            paddb mm0, mm2     
                               

            
            psllq mm6, ShiftBpp  
                                 
            movq mm2, mm0        
            psllq mm2, ShiftBpp  
                              
                              
            movq mm1, mm3     
            pand mm1, mm2     
                              
            psrlq mm2, 1      
            pand  mm2, mm4    
            paddb mm2, mm1    
            pand mm2, mm6     
            add ebx, 8
            paddb mm0, mm2    
                              

            
            movq [edi + ebx - 8], mm0
            
            cmp ebx, MMXLength
            movq mm2, mm0     
            jb davg3lp
         } 
      }
      break;

      case 6:
      case 4:
      case 7:
      case 5:
      {
         ActiveMask.use  = 0xffffffffffffffff;  
                                                
         ShiftBpp.use = bpp << 3;
         ShiftRem.use = 64 - ShiftBpp.use;
         _asm {
            movq mm4, HBClearMask
            
            mov ebx, diff       
            
            movq mm7, ActiveMask
            mov edi, row         
            psrlq mm7, ShiftRem
            mov esi, prev_row    
            movq mm6, mm7
            movq mm5, LBCarryMask
            psllq mm6, ShiftBpp  
            
            movq mm2, [edi + ebx - 8]  
                                 
davg4lp:
            movq mm0, [edi + ebx]
            psrlq mm2, ShiftRem  
            movq mm1, [esi + ebx]
            
            movq mm3, mm5
            pand mm3, mm1     
            psrlq mm1, 1      
            pand  mm1, mm4    
            paddb mm0, mm1    
            
            movq mm1, mm3     
            pand mm1, mm2     
                              
            psrlq mm2, 1      
            pand  mm2, mm4    
            paddb mm2, mm1    
            pand mm2, mm7     
            paddb mm0, mm2    
                              
            
            movq mm2, mm0     
            psllq mm2, ShiftBpp 
            add ebx, 8
            movq mm1, mm3     
            pand mm1, mm2     
                              
            psrlq mm2, 1      
            pand  mm2, mm4    
            paddb mm2, mm1    
            pand mm2, mm6     
            paddb mm0, mm2    
                              
            cmp ebx, MMXLength
            
            movq [edi + ebx - 8], mm0
            
            movq mm2, mm0     
            jb davg4lp
         } 
      }
      break;
      case 2:
      {
         ActiveMask.use  = 0x000000000000ffff;
         ShiftBpp.use = 16;   
         ShiftRem.use = 48;   
         _asm {
            
            movq mm7, ActiveMask
            
            mov ebx, diff     
            movq mm5, LBCarryMask
            mov edi, row      
            movq mm4, HBClearMask
            mov esi, prev_row  
            
            movq mm2, [edi + ebx - 8]  
                              
davg2lp:
            movq mm0, [edi + ebx]
            psrlq mm2, ShiftRem  
            movq mm1, [esi + ebx]
            
            movq mm3, mm5
            pand mm3, mm1     
            psrlq mm1, 1      
            pand  mm1, mm4    
            movq mm6, mm7
            paddb mm0, mm1    
            
            movq mm1, mm3     
            pand mm1, mm2     
                              
            psrlq mm2, 1      
            pand  mm2, mm4    
            paddb mm2, mm1    
            pand mm2, mm6     
            paddb mm0, mm2 
            
            psllq mm6, ShiftBpp 
            movq mm2, mm0       
            psllq mm2, ShiftBpp 
            movq mm1, mm3       
            pand mm1, mm2       
                                
            psrlq mm2, 1        
            pand  mm2, mm4      
            paddb mm2, mm1      
            pand mm2, mm6       
            paddb mm0, mm2 

            
            psllq mm6, ShiftBpp 
            movq mm2, mm0       
            psllq mm2, ShiftBpp 
                                
                                
            movq mm1, mm3       
            pand mm1, mm2       
                                
            psrlq mm2, 1        
            pand  mm2, mm4      
            paddb mm2, mm1      
            pand mm2, mm6       
            paddb mm0, mm2 

            
            psllq mm6, ShiftBpp  
            movq mm2, mm0        
            psllq mm2, ShiftBpp  
                                 
                                 
            add ebx, 8
            movq mm1, mm3    
            pand mm1, mm2    
                             
            psrlq mm2, 1     
            pand  mm2, mm4   
            paddb mm2, mm1   
            pand mm2, mm6    
            paddb mm0, mm2 

            cmp ebx, MMXLength
            
            movq [edi + ebx - 8], mm0
            
            movq mm2, mm0    
            jb davg2lp
        } 
      }
      break;

      case 1:                 
      {
         _asm {
            
            mov ebx, diff     
            mov edi, row      
            cmp ebx, FullLength  
            jnb davg1end
            
            mov esi, prev_row    
            mov edx, edi
            xor ecx, ecx         
            sub edx, bpp         
davg1lp:
            
            xor eax, eax
            mov cl, [esi + ebx]  
            mov al, [edx + ebx]  
            add ax, cx
            inc ebx
            shr ax, 1            
            add al, [edi+ebx-1]  
            cmp ebx, FullLength  
            mov [edi+ebx-1], al  
                         
            jb davg1lp
davg1end:
         } 
      }
      return;

      case 8:             
      {
         _asm {
            
            mov ebx, diff           
            movq mm5, LBCarryMask
            mov edi, row            
            movq mm4, HBClearMask
            mov esi, prev_row       
            
            movq mm2, [edi + ebx - 8]  
                                
davg8lp:
            movq mm0, [edi + ebx]
            movq mm3, mm5
            movq mm1, [esi + ebx]
            add ebx, 8
            pand mm3, mm1       
            psrlq mm1, 1        
            pand mm3, mm2       
                                
            psrlq mm2, 1        
            pand  mm1, mm4      
            paddb mm0, mm3      
            pand  mm2, mm4      
            paddb mm0, mm1      
            paddb mm0, mm2      
            cmp ebx, MMXLength
            movq [edi + ebx - 8], mm0
            movq mm2, mm0       
            jb davg8lp
        } 
      }
      break;
      default:                  
      {
        _asm {
            movq mm5, LBCarryMask
            
            mov ebx, diff       
            mov edi, row        
            movq mm4, HBClearMask
            mov edx, edi
            mov esi, prev_row   
            sub edx, bpp        
davgAlp:
            movq mm0, [edi + ebx]
            movq mm3, mm5
            movq mm1, [esi + ebx]
            pand mm3, mm1       
            movq mm2, [edx + ebx]
            psrlq mm1, 1        
            pand mm3, mm2       
                                
            psrlq mm2, 1        
            pand  mm1, mm4      
            paddb mm0, mm3      
            pand  mm2, mm4      
            paddb mm0, mm1      
            add ebx, 8
            paddb mm0, mm2      
            cmp ebx, MMXLength
            movq [edi + ebx - 8], mm0
            jb davgAlp
        } 
      }
      break;
   }                         

   _asm {
         
         
         mov ebx, MMXLength    
         mov edi, row          
         cmp ebx, FullLength   
         jnb davgend
         
         mov esi, prev_row     
         mov edx, edi
         xor ecx, ecx          
         sub edx, bpp          
davglp2:
         
         xor eax, eax
         mov cl, [esi + ebx]   
         mov al, [edx + ebx]   
         add ax, cx
         inc ebx
         shr ax, 1              
         add al, [edi+ebx-1]    
         cmp ebx, FullLength    
         mov [edi+ebx-1], al    
                          
         jb davglp2
davgend:
         emms             
   } 
}


void /* PRIVATE */
png_read_filter_row_mmx_paeth(png_row_infop row_info, png_bytep row,
                              png_bytep prev_row)
{
   png_uint_32 FullLength;
   png_uint_32 MMXLength;
   
   int bpp;
   int diff;
   
   int patemp, pbtemp, pctemp;

   bpp = (row_info->pixel_depth + 7) >> 3; 
   FullLength  = row_info->rowbytes; 
   _asm
   {
         xor ebx, ebx        
         mov edi, row
         xor edx, edx        
         mov esi, prev_row
         xor eax, eax

         
         
         
dpthrlp:
         mov al, [edi + ebx]
         add al, [esi + ebx]
         inc ebx
         cmp ebx, bpp
         mov [edi + ebx - 1], al
         jb dpthrlp
         
         mov diff, edi         
         add diff, ebx         
         xor ecx, ecx
         add diff, 0xf         
         and diff, 0xfffffff8  
         sub diff, edi         
         jz dpthgo
         
dpthlp1:
         xor eax, eax
         
         mov al, [esi + ebx]   
         mov cl, [esi + edx]   
         sub eax, ecx          
         mov patemp, eax       
         xor eax, eax
         
         mov al, [edi + edx]   
         sub eax, ecx          
         mov ecx, eax
         
         add eax, patemp       
         
         test eax, 0x80000000
         jz dpthpca
         neg eax               
dpthpca:
         mov pctemp, eax       
         
         test ecx, 0x80000000
         jz dpthpba
         neg ecx               
dpthpba:
         mov pbtemp, ecx       
         
         mov eax, patemp
         test eax, 0x80000000
         jz dpthpaa
         neg eax               
dpthpaa:
         mov patemp, eax       
         
         cmp eax, ecx
         jna dpthabb
         
         cmp ecx, pctemp
         jna dpthbbc
         
         mov cl, [esi + edx]  
         jmp dpthpaeth
dpthbbc:
         
         mov cl, [esi + ebx]   
         jmp dpthpaeth
dpthabb:
         
         cmp eax, pctemp
         jna dpthabc
         
         mov cl, [esi + edx]  
         jmp dpthpaeth
dpthabc:
         
         mov cl, [edi + edx]  
dpthpaeth:
         inc ebx
         inc edx
         
         add [edi + ebx - 1], cl
         cmp ebx, diff
         jb dpthlp1
dpthgo:
         mov ecx, FullLength
         mov eax, ecx
         sub eax, ebx          
         and eax, 0x00000007   
         sub ecx, eax          
         mov MMXLength, ecx
   } 
   
   switch ( bpp )
   {
      case 3:
      {
         ActiveMask.use = 0x0000000000ffffff;
         ActiveMaskEnd.use = 0xffff000000000000;
         ShiftBpp.use = 24;    
         ShiftRem.use = 40;    
         _asm
         {
            mov ebx, diff
            mov edi, row
            mov esi, prev_row
            pxor mm0, mm0
            
            movq mm1, [edi+ebx-8]
dpth3lp:
            psrlq mm1, ShiftRem     
            movq mm2, [esi + ebx]   
            punpcklbw mm1, mm0      
            movq mm3, [esi+ebx-8]   
            punpcklbw mm2, mm0      
            psrlq mm3, ShiftRem     
            
            movq mm4, mm2
            punpcklbw mm3, mm0      
            
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7
            
            movq mm6, mm4
            psubw mm5, mm3

            
            
            
            pcmpgtw mm0, mm4    
            paddw mm6, mm5
            pand mm0, mm4       
            pcmpgtw mm7, mm5    
            psubw mm4, mm0
            pand mm7, mm5       
            psubw mm4, mm0
            psubw mm5, mm7
            pxor mm0, mm0
            pcmpgtw mm0, mm6    
            pand mm0, mm6       
            psubw mm5, mm7
            psubw mm6, mm0
            
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5    
            movq mm0, mm7
            
            pand mm5, mm7
            
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2
            
            pcmpgtw mm7, mm6       
            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            paddw mm7, mm3
            pxor mm0, mm0
            packuswb mm7, mm1
            movq mm3, [esi + ebx]   
            pand mm7, ActiveMask
            movq mm2, mm3           
            paddb mm7, [edi + ebx]  
            punpcklbw mm3, mm0      
            movq [edi + ebx], mm7   
            movq mm1, mm7           
            
            psrlq mm2, ShiftBpp     
            punpcklbw mm1, mm0      
            pxor mm7, mm7
            punpcklbw mm2, mm0      
            
            movq mm5, mm1
            
            movq mm4, mm2
            psubw mm5, mm3
            psubw mm4, mm3
            
            
            movq mm6, mm5
            paddw mm6, mm4

            
            
            
            pcmpgtw mm0, mm5       
            pcmpgtw mm7, mm4       
            pand mm0, mm5          
            pand mm7, mm4          
            psubw mm5, mm0
            psubw mm4, mm7
            psubw mm5, mm0
            psubw mm4, mm7
            pxor mm0, mm0
            pcmpgtw mm0, mm6       
            pand mm0, mm6          
            psubw mm6, mm0
            
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5       
            movq mm0, mm7
            
            pand mm5, mm7
            
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2
            
            pcmpgtw mm7, mm6       
            movq mm2, [esi + ebx]  
            pand mm3, mm7
            pandn mm7, mm0
            pxor mm1, mm1
            paddw mm7, mm3
            pxor mm0, mm0
            packuswb mm7, mm1
            movq mm3, mm2           
            pand mm7, ActiveMask
            punpckhbw mm2, mm0      
            psllq mm7, ShiftBpp     
             
            movq mm4, mm2
            paddb mm7, [edi + ebx]  
            psllq mm3, ShiftBpp     
            movq [edi + ebx], mm7   
            movq mm1, mm7
            punpckhbw mm3, mm0      
            psllq mm1, ShiftBpp     
                                    
            
            pxor mm7, mm7
            punpckhbw mm1, mm0      
            psubw mm4, mm3
            
            movq mm5, mm1
            
            movq mm6, mm4
            psubw mm5, mm3
            pxor mm0, mm0
            paddw mm6, mm5

            
            
            
            pcmpgtw mm0, mm4    
            pcmpgtw mm7, mm5    
            pand mm0, mm4       
            pand mm7, mm5       
            psubw mm4, mm0
            psubw mm5, mm7
            psubw mm4, mm0
            psubw mm5, mm7
            pxor mm0, mm0
            pcmpgtw mm0, mm6    
            pand mm0, mm6       
            psubw mm6, mm0
            
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5    
            movq mm0, mm7
            
            pand mm2, mm0
            
            pand mm5, mm7
            pandn mm0, mm1
            pandn mm7, mm4
            paddw mm0, mm2
            paddw mm7, mm5
            
            pcmpgtw mm7, mm6    
            pand mm3, mm7
            pandn mm7, mm0
            paddw mm7, mm3
            pxor mm1, mm1
            packuswb mm1, mm7
            
            add ebx, 8
            pand mm1, ActiveMaskEnd
            paddb mm1, [edi + ebx - 8] 

            cmp ebx, MMXLength
            pxor mm0, mm0              
            movq [edi + ebx - 8], mm1  
                                 
                           
            jb dpth3lp
         } 
      }
      break;

      case 6:
      case 7:
      case 5:
      {
         ActiveMask.use  = 0x00000000ffffffff;
         ActiveMask2.use = 0xffffffff00000000;
         ShiftBpp.use = bpp << 3;    
         ShiftRem.use = 64 - ShiftBpp.use;
         _asm
         {
            mov ebx, diff
            mov edi, row
            mov esi, prev_row
            
            movq mm1, [edi+ebx-8]
            pxor mm0, mm0
dpth6lp:
            
            psrlq mm1, ShiftRem
            
            movq mm3, [esi+ebx-8]      
            punpcklbw mm1, mm0      
            movq mm2, [esi + ebx]   
            punpcklbw mm2, mm0      
            
            psrlq mm3, ShiftRem
            
            movq mm4, mm2
            punpcklbw mm3, mm0      
            
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7
            
            movq mm6, mm4
            psubw mm5, mm3
            
            
            
            pcmpgtw mm0, mm4    
            paddw mm6, mm5
            pand mm0, mm4       
            pcmpgtw mm7, mm5    
            psubw mm4, mm0
            pand mm7, mm5       
            psubw mm4, mm0
            psubw mm5, mm7
            pxor mm0, mm0
            pcmpgtw mm0, mm6    
            pand mm0, mm6       
            psubw mm5, mm7
            psubw mm6, mm0
            
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5    
            movq mm0, mm7
            
            pand mm5, mm7
            
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2
            
            pcmpgtw mm7, mm6    
            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            paddw mm7, mm3
            pxor mm0, mm0
            packuswb mm7, mm1
            movq mm3, [esi + ebx - 8]  
            pand mm7, ActiveMask
            psrlq mm3, ShiftRem
            movq mm2, [esi + ebx]      
            paddb mm7, [edi + ebx]     
            movq mm6, mm2
            movq [edi + ebx], mm7      
            movq mm1, [edi+ebx-8]
            psllq mm6, ShiftBpp
            movq mm5, mm7
            psrlq mm1, ShiftRem
            por mm3, mm6
            psllq mm5, ShiftBpp
            punpckhbw mm3, mm0         
            por mm1, mm5
            
            punpckhbw mm2, mm0         
            punpckhbw mm1, mm0         
            
            movq mm4, mm2
            
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7
            
            movq mm6, mm4
            psubw mm5, mm3
            
            
            
            pcmpgtw mm0, mm4       
            paddw mm6, mm5
            pand mm0, mm4          
            pcmpgtw mm7, mm5       
            psubw mm4, mm0
            pand mm7, mm5          
            psubw mm4, mm0
            psubw mm5, mm7
            pxor mm0, mm0
            pcmpgtw mm0, mm6       
            pand mm0, mm6          
            psubw mm5, mm7
            psubw mm6, mm0
            
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5       
            movq mm0, mm7
            
            pand mm5, mm7
            
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2
            
            pcmpgtw mm7, mm6           
            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            pxor mm1, mm1
            paddw mm7, mm3
            pxor mm0, mm0
            
            add ebx, 8
            packuswb mm1, mm7
            paddb mm1, [edi + ebx - 8]     
            cmp ebx, MMXLength
            movq [edi + ebx - 8], mm1      
                                
            jb dpth6lp
         } 
      }
      break;

      case 4:
      {
         ActiveMask.use  = 0x00000000ffffffff;
         _asm {
            mov ebx, diff
            mov edi, row
            mov esi, prev_row
            pxor mm0, mm0
            
            movq mm1, [edi+ebx-8]    
                                     
dpth4lp:
            
            movq mm3, [esi+ebx-8]    
            punpckhbw mm1, mm0       
            movq mm2, [esi + ebx]    
            punpcklbw mm2, mm0       
            
            movq mm4, mm2
            punpckhbw mm3, mm0       
            
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7
            
            movq mm6, mm4
            psubw mm5, mm3
            
            
            
            pcmpgtw mm0, mm4       
            paddw mm6, mm5
            pand mm0, mm4          
            pcmpgtw mm7, mm5       
            psubw mm4, mm0
            pand mm7, mm5          
            psubw mm4, mm0
            psubw mm5, mm7
            pxor mm0, mm0
            pcmpgtw mm0, mm6       
            pand mm0, mm6          
            psubw mm5, mm7
            psubw mm6, mm0
            
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5       
            movq mm0, mm7
            
            pand mm5, mm7
            
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2
            
            pcmpgtw mm7, mm6       
            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            paddw mm7, mm3
            pxor mm0, mm0
            packuswb mm7, mm1
            movq mm3, [esi + ebx]      
            pand mm7, ActiveMask
            movq mm2, mm3              
            paddb mm7, [edi + ebx]     
            punpcklbw mm3, mm0         
            movq [edi + ebx], mm7      
            movq mm1, mm7              
            
            punpckhbw mm2, mm0         
            punpcklbw mm1, mm0         
            
            movq mm4, mm2
            
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7
            
            movq mm6, mm4
            psubw mm5, mm3
            
            
            
            pcmpgtw mm0, mm4       
            paddw mm6, mm5
            pand mm0, mm4          
            pcmpgtw mm7, mm5       
            psubw mm4, mm0
            pand mm7, mm5          
            psubw mm4, mm0
            psubw mm5, mm7
            pxor mm0, mm0
            pcmpgtw mm0, mm6       
            pand mm0, mm6          
            psubw mm5, mm7
            psubw mm6, mm0
            
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5       
            movq mm0, mm7
            
            pand mm5, mm7
            
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2
            
            pcmpgtw mm7, mm6       
            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            pxor mm1, mm1
            paddw mm7, mm3
            pxor mm0, mm0
            
            add ebx, 8
            packuswb mm1, mm7
            paddb mm1, [edi + ebx - 8]     
            cmp ebx, MMXLength
            movq [edi + ebx - 8], mm1      
                                
            jb dpth4lp
         } 
      }
      break;
      case 8:                          
      {
         ActiveMask.use  = 0x00000000ffffffff;
         _asm {
            mov ebx, diff
            mov edi, row
            mov esi, prev_row
            pxor mm0, mm0
            
            movq mm1, [edi+ebx-8]      
                                       
dpth8lp:
            
            movq mm3, [esi+ebx-8]      
            punpcklbw mm1, mm0         
            movq mm2, [esi + ebx]      
            punpcklbw mm2, mm0         
            
            movq mm4, mm2
            punpcklbw mm3, mm0         
            
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7
            
            movq mm6, mm4
            psubw mm5, mm3
            
            
            
            pcmpgtw mm0, mm4       
            paddw mm6, mm5
            pand mm0, mm4          
            pcmpgtw mm7, mm5       
            psubw mm4, mm0
            pand mm7, mm5          
            psubw mm4, mm0
            psubw mm5, mm7
            pxor mm0, mm0
            pcmpgtw mm0, mm6       
            pand mm0, mm6          
            psubw mm5, mm7
            psubw mm6, mm0
            
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5       
            movq mm0, mm7
            
            pand mm5, mm7
            
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2
            
            pcmpgtw mm7, mm6       
            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            paddw mm7, mm3
            pxor mm0, mm0
            packuswb mm7, mm1
            movq mm3, [esi+ebx-8]    
            pand mm7, ActiveMask
            movq mm2, [esi + ebx]    
            paddb mm7, [edi + ebx]   
            punpckhbw mm3, mm0       
            movq [edi + ebx], mm7    
            movq mm1, [edi+ebx-8]    

            
            punpckhbw mm2, mm0       
            punpckhbw mm1, mm0       
            
            movq mm4, mm2
            
            movq mm5, mm1
            psubw mm4, mm3
            pxor mm7, mm7
            
            movq mm6, mm4
            psubw mm5, mm3
            
            
            
            pcmpgtw mm0, mm4       
            paddw mm6, mm5
            pand mm0, mm4          
            pcmpgtw mm7, mm5       
            psubw mm4, mm0
            pand mm7, mm5          
            psubw mm4, mm0
            psubw mm5, mm7
            pxor mm0, mm0
            pcmpgtw mm0, mm6       
            pand mm0, mm6          
            psubw mm5, mm7
            psubw mm6, mm0
            
            movq mm7, mm4
            psubw mm6, mm0
            pcmpgtw mm7, mm5       
            movq mm0, mm7
            
            pand mm5, mm7
            
            pand mm2, mm0
            pandn mm7, mm4
            pandn mm0, mm1
            paddw mm7, mm5
            paddw mm0, mm2
            
            pcmpgtw mm7, mm6       
            pxor mm1, mm1
            pand mm3, mm7
            pandn mm7, mm0
            pxor mm1, mm1
            paddw mm7, mm3
            pxor mm0, mm0
            
            add ebx, 8
            packuswb mm1, mm7
            paddb mm1, [edi + ebx - 8]     
            cmp ebx, MMXLength
            movq [edi + ebx - 8], mm1      
                            
            jb dpth8lp
         } 
      }
      break;

      case 1:                
      case 2:                
      default:               
      {
         _asm {
            mov ebx, diff
            cmp ebx, FullLength
            jnb dpthdend
            mov edi, row
            mov esi, prev_row
            
            mov edx, ebx
            xor ecx, ecx        
            sub edx, bpp        
dpthdlp:
            xor eax, eax
            
            mov al, [esi + ebx]        
            mov cl, [esi + edx]        
            sub eax, ecx                 
            mov patemp, eax                 
            xor eax, eax
            
            mov al, [edi + edx]        
            sub eax, ecx                 
            mov ecx, eax
            
            add eax, patemp                 
            
            test eax, 0x80000000
            jz dpthdpca
            neg eax                     
dpthdpca:
            mov pctemp, eax             
            
            test ecx, 0x80000000
            jz dpthdpba
            neg ecx                     
dpthdpba:
            mov pbtemp, ecx             
            
            mov eax, patemp
            test eax, 0x80000000
            jz dpthdpaa
            neg eax                     
dpthdpaa:
            mov patemp, eax             
            
            cmp eax, ecx
            jna dpthdabb
            
            cmp ecx, pctemp
            jna dpthdbbc
            
            mov cl, [esi + edx]  
            jmp dpthdpaeth
dpthdbbc:
            
            mov cl, [esi + ebx]        
            jmp dpthdpaeth
dpthdabb:
            
            cmp eax, pctemp
            jna dpthdabc
            
            mov cl, [esi + edx]  
            jmp dpthdpaeth
dpthdabc:
            
            mov cl, [edi + edx]  
dpthdpaeth:
            inc ebx
            inc edx
            
            add [edi + ebx - 1], cl
            cmp ebx, FullLength
            jb dpthdlp
dpthdend:
         } 
      }
      return;                   
   }                         
   _asm
   {
         
         
         mov ebx, MMXLength
         cmp ebx, FullLength
         jnb dpthend
         mov edi, row
         mov esi, prev_row
         
         mov edx, ebx
         xor ecx, ecx         
         sub edx, bpp         
dpthlp2:
         xor eax, eax
         
         mov al, [esi + ebx]  
         mov cl, [esi + edx]  
         sub eax, ecx         
         mov patemp, eax      
         xor eax, eax
         
         mov al, [edi + edx]  
         sub eax, ecx         
         mov ecx, eax
         
         add eax, patemp      
         
         test eax, 0x80000000
         jz dpthpca2
         neg eax              
dpthpca2:
         mov pctemp, eax      
         
         test ecx, 0x80000000
         jz dpthpba2
         neg ecx              
dpthpba2:
         mov pbtemp, ecx      
         
         mov eax, patemp
         test eax, 0x80000000
         jz dpthpaa2
         neg eax              
dpthpaa2:
         mov patemp, eax      
         
         cmp eax, ecx
         jna dpthabb2
         
         cmp ecx, pctemp
         jna dpthbbc2
         
         mov cl, [esi + edx]  
         jmp dpthpaeth2
dpthbbc2:
         
         mov cl, [esi + ebx]        
         jmp dpthpaeth2
dpthabb2:
         
         cmp eax, pctemp
         jna dpthabc2
         
         mov cl, [esi + edx]  
         jmp dpthpaeth2
dpthabc2:
         
         mov cl, [edi + edx]  
dpthpaeth2:
         inc ebx
         inc edx
         
         add [edi + ebx - 1], cl
         cmp ebx, FullLength
         jb dpthlp2
dpthend:
         emms             
   } 
}


void /* PRIVATE */
png_read_filter_row_mmx_sub(png_row_infop row_info, png_bytep row)
{
   
   int bpp;
   png_uint_32 FullLength;
   png_uint_32 MMXLength;
   int diff;

   bpp = (row_info->pixel_depth + 7) >> 3; 
   FullLength  = row_info->rowbytes - bpp; 
   _asm {
        mov edi, row
        mov esi, edi               
        add edi, bpp               
        xor eax, eax
        
        mov diff, edi               
        add diff, 0xf               
                                        
        xor ebx, ebx
        and diff, 0xfffffff8        
        sub diff, edi               
                                        
        jz dsubgo
        
dsublp1:
        mov al, [esi+ebx]
        add [edi+ebx], al
        inc ebx
        cmp ebx, diff
        jb dsublp1
dsubgo:
        mov ecx, FullLength
        mov edx, ecx
        sub edx, ebx                  
        and edx, 0x00000007           
        sub ecx, edx                  
        mov MMXLength, ecx
   } 

   
   switch ( bpp )
   {
        case 3:
        {
         ActiveMask.use  = 0x0000ffffff000000;
         ShiftBpp.use = 24;       
         ShiftRem.use  = 40;      
         _asm {
            mov edi, row
            movq mm7, ActiveMask  
            mov esi, edi              
            add edi, bpp          
            movq mm6, mm7
            mov ebx, diff
            psllq mm6, ShiftBpp   
                                  
            
            movq mm1, [edi+ebx-8]
dsub3lp:
            psrlq mm1, ShiftRem   
                          
            
            movq mm0, [edi+ebx]
            paddb mm0, mm1
            
            movq mm1, mm0         
            psllq mm1, ShiftBpp   
            pand mm1, mm7         
            paddb mm0, mm1
            
            movq mm1, mm0         
            psllq mm1, ShiftBpp   
            pand mm1, mm6         
            add ebx, 8
            paddb mm0, mm1
            cmp ebx, MMXLength
            movq [edi+ebx-8], mm0     
            
            movq mm1, mm0
            jb dsub3lp
         } 
      }
      break;

      case 1:
      {
         
         
         
         
         
         
         
         
         
         
         
         
         _asm {
            mov ebx, diff
            mov edi, row
            cmp ebx, FullLength
            jnb dsub1end
            mov esi, edi          
            xor eax, eax
            add edi, bpp      
dsub1lp:
            mov al, [esi+ebx]
            add [edi+ebx], al
            inc ebx
            cmp ebx, FullLength
            jb dsub1lp
dsub1end:
         } 
      }
      return;

      case 6:
      case 7:
      case 4:
      case 5:
      {
         ShiftBpp.use = bpp << 3;
         ShiftRem.use = 64 - ShiftBpp.use;
         _asm {
            mov edi, row
            mov ebx, diff
            mov esi, edi               
            add edi, bpp           
            
            movq mm1, [edi+ebx-8]
dsub4lp:
            psrlq mm1, ShiftRem 
                          
            movq mm0, [edi+ebx]
            paddb mm0, mm1
            
            movq mm1, mm0          
            psllq mm1, ShiftBpp    
                                   
                                   
            add ebx, 8
            paddb mm0, mm1
            cmp ebx, MMXLength
            movq [edi+ebx-8], mm0
            movq mm1, mm0          
            jb dsub4lp
         } 
      }
      break;

      case 2:
      {
         ActiveMask.use  = 0x00000000ffff0000;
         ShiftBpp.use = 16;       
         ShiftRem.use = 48;       
         _asm {
            movq mm7, ActiveMask  
            mov ebx, diff
            movq mm6, mm7
            mov edi, row
            psllq mm6, ShiftBpp     
                                    
            mov esi, edi            
            movq mm5, mm6
            add edi, bpp            
            psllq mm5, ShiftBpp     
                                    
            
            movq mm1, [edi+ebx-8]
dsub2lp:
            
            psrlq mm1, ShiftRem     
                                    
                                    
            movq mm0, [edi+ebx]
            paddb mm0, mm1
            
            movq mm1, mm0           
            psllq mm1, ShiftBpp     
            pand mm1, mm7           
            paddb mm0, mm1
            
            movq mm1, mm0           
            psllq mm1, ShiftBpp     
            pand mm1, mm6           
            paddb mm0, mm1
            
            movq mm1, mm0           
            psllq mm1, ShiftBpp     
            pand mm1, mm5           
            add ebx, 8
            paddb mm0, mm1
            cmp ebx, MMXLength
            movq [edi+ebx-8], mm0   
            movq mm1, mm0           
            jb dsub2lp
         } 
      }
      break;
      case 8:
      {
         _asm {
            mov edi, row
            mov ebx, diff
            mov esi, edi            
            add edi, bpp            
            mov ecx, MMXLength
            movq mm7, [edi+ebx-8]   
                                    
            and ecx, 0x0000003f     
dsub8lp:
            movq mm0, [edi+ebx]     
            paddb mm0, mm7
            movq mm1, [edi+ebx+8]   
            movq [edi+ebx], mm0    
                                   
                                   
                                   
                                   
                                   
            paddb mm1, mm0
            movq mm2, [edi+ebx+16]  
            movq [edi+ebx+8], mm1   
            paddb mm2, mm1
            movq mm3, [edi+ebx+24]  
            movq [edi+ebx+16], mm2  
            paddb mm3, mm2
            movq mm4, [edi+ebx+32]  
            movq [edi+ebx+24], mm3  
            paddb mm4, mm3
            movq mm5, [edi+ebx+40]  
            movq [edi+ebx+32], mm4  
            paddb mm5, mm4
            movq mm6, [edi+ebx+48]  
            movq [edi+ebx+40], mm5  
            paddb mm6, mm5
            movq mm7, [edi+ebx+56]  
            movq [edi+ebx+48], mm6  
            add ebx, 64
            paddb mm7, mm6
            cmp ebx, ecx
            movq [edi+ebx-8], mm7   
            jb dsub8lp
            cmp ebx, MMXLength
            jnb dsub8lt8
dsub8lpA:
            movq mm0, [edi+ebx]
            add ebx, 8
            paddb mm0, mm7
            cmp ebx, MMXLength
            movq [edi+ebx-8], mm0   
            movq mm7, mm0           
                                    
            jb dsub8lpA
dsub8lt8:
         } 
      }
      break;

      default:                
      {
         _asm {
            mov ebx, diff
            mov edi, row
            mov esi, edi           
            add edi, bpp           
dsubAlp:
            movq mm0, [edi+ebx]
            movq mm1, [esi+ebx]
            add ebx, 8
            paddb mm0, mm1
            cmp ebx, MMXLength
            movq [edi+ebx-8], mm0  
                                   
            jb dsubAlp
         } 
      }
      break;

   } 

   _asm {
        mov ebx, MMXLength
        mov edi, row
        cmp ebx, FullLength
        jnb dsubend
        mov esi, edi               
        xor eax, eax
        add edi, bpp               
dsublp2:
        mov al, [esi+ebx]
        add [edi+ebx], al
        inc ebx
        cmp ebx, FullLength
        jb dsublp2
dsubend:
        emms             
   } 
}


void /* PRIVATE */
png_read_filter_row_mmx_up(png_row_infop row_info, png_bytep row,
   png_bytep prev_row)
{
   png_uint_32 len;
   len  = row_info->rowbytes;       
   _asm {
      mov edi, row
      
      mov ecx, edi
      xor ebx, ebx
      add ecx, 0x7
      xor eax, eax
      and ecx, 0xfffffff8
      mov esi, prev_row
      sub ecx, edi
      jz dupgo
      
duplp1:
      mov al, [edi+ebx]
      add al, [esi+ebx]
      inc ebx
      cmp ebx, ecx
      mov [edi + ebx-1], al  
      jb duplp1
dupgo:
      mov ecx, len
      mov edx, ecx
      sub edx, ebx                  
      and edx, 0x0000003f           
      sub ecx, edx                  
      
      
duploop:
      movq mm1, [esi+ebx]
      movq mm0, [edi+ebx]
      movq mm3, [esi+ebx+8]
      paddb mm0, mm1
      movq mm2, [edi+ebx+8]
      movq [edi+ebx], mm0
      paddb mm2, mm3
      movq mm5, [esi+ebx+16]
      movq [edi+ebx+8], mm2
      movq mm4, [edi+ebx+16]
      movq mm7, [esi+ebx+24]
      paddb mm4, mm5
      movq mm6, [edi+ebx+24]
      movq [edi+ebx+16], mm4
      paddb mm6, mm7
      movq mm1, [esi+ebx+32]
      movq [edi+ebx+24], mm6
      movq mm0, [edi+ebx+32]
      movq mm3, [esi+ebx+40]
      paddb mm0, mm1
      movq mm2, [edi+ebx+40]
      movq [edi+ebx+32], mm0
      paddb mm2, mm3
      movq mm5, [esi+ebx+48]
      movq [edi+ebx+40], mm2
      movq mm4, [edi+ebx+48]
      movq mm7, [esi+ebx+56]
      paddb mm4, mm5
      movq mm6, [edi+ebx+56]
      movq [edi+ebx+48], mm4
      add ebx, 64
      paddb mm6, mm7
      cmp ebx, ecx
      movq [edi+ebx-8], mm6 
                                     
      jb duploop

      cmp edx, 0                     
      jz dupend


      
      
      cmp edx, 8 
      jb duplt8


      add ecx, edx
      and edx, 0x00000007           
      sub ecx, edx                  
      jz duplt8
      
duplpA:
      movq mm1, [esi+ebx]
      movq mm0, [edi+ebx]
      add ebx, 8
      paddb mm0, mm1
      cmp ebx, ecx
      movq [edi+ebx-8], mm0 
      jb duplpA
      cmp edx, 0            
      jz dupend
duplt8:
      xor eax, eax
      add ecx, edx          
      
duplp2:
      mov al, [edi + ebx]
      add al, [esi + ebx]
      inc ebx
      cmp ebx, ecx
      mov [edi + ebx-1], al 
      jb duplp2
dupend:
      
      emms          
   } 
}



void /* PRIVATE */
png_read_filter_row(png_structp png_ptr, png_row_infop row_info, png_bytep
   row, png_bytep prev_row, int filter)
{
#ifdef PNG_DEBUG
   char filnm[10];
#endif

   if (mmx_supported == 2) {
#if !defined(PNG_1_0_X)
       /* this should have happened in png_init_mmx_flags() already */
       png_warning(png_ptr, "asm_flags may not have been initialized");
#endif
       png_mmx_support();
   }

#ifdef PNG_DEBUG
   png_debug(1, "in png_read_filter_row\n");
   switch (filter)
   {
      case 0: sprintf(filnm, "none");
         break;
#if !defined(PNG_1_0_X)
      case 1: sprintf(filnm, "sub-%s",
        (png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_SUB)? "MMX" : "x86");
         break;
      case 2: sprintf(filnm, "up-%s",
        (png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_UP)? "MMX" : "x86");
         break;
      case 3: sprintf(filnm, "avg-%s",
        (png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_AVG)? "MMX" : "x86");
         break;
      case 4: sprintf(filnm, "Paeth-%s",
        (png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_PAETH)? "MMX":"x86");
         break;
#else
      case 1: sprintf(filnm, "sub");
         break;
      case 2: sprintf(filnm, "up");
         break;
      case 3: sprintf(filnm, "avg");
         break;
      case 4: sprintf(filnm, "Paeth");
         break;
#endif
      default: sprintf(filnm, "unknw");
         break;
   }
   png_debug2(0,"row=%5d, %s, ", png_ptr->row_number, filnm);
   png_debug2(0, "pd=%2d, b=%d, ", (int)row_info->pixel_depth,
      (int)((row_info->pixel_depth + 7) >> 3));
   png_debug1(0,"len=%8d, ", row_info->rowbytes);
#endif /* PNG_DEBUG */

   switch (filter)
   {
      case PNG_FILTER_VALUE_NONE:
         break;

      case PNG_FILTER_VALUE_SUB:
      {
#if !defined(PNG_1_0_X)
         if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_SUB) &&
             (row_info->pixel_depth >= png_ptr->mmx_bitdepth_threshold) &&
             (row_info->rowbytes >= png_ptr->mmx_rowbytes_threshold))
#else
         if (mmx_supported)
#endif
         {
            png_read_filter_row_mmx_sub(row_info, row);
         }
         else
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
         }
         break;
      }

      case PNG_FILTER_VALUE_UP:
      {
#if !defined(PNG_1_0_X)
         if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_UP) &&
             (row_info->pixel_depth >= png_ptr->mmx_bitdepth_threshold) &&
             (row_info->rowbytes >= png_ptr->mmx_rowbytes_threshold))
#else
         if (mmx_supported)
#endif
         {
            png_read_filter_row_mmx_up(row_info, row, prev_row);
         }
         else
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
         }
         break;
      }

      case PNG_FILTER_VALUE_AVG:
      {
#if !defined(PNG_1_0_X)
         if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_AVG) &&
             (row_info->pixel_depth >= png_ptr->mmx_bitdepth_threshold) &&
             (row_info->rowbytes >= png_ptr->mmx_rowbytes_threshold))
#else
         if (mmx_supported)
#endif
         {
            png_read_filter_row_mmx_avg(row_info, row, prev_row);
         }
         else
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
         }
         break;
      }

      case PNG_FILTER_VALUE_PAETH:
      {
#if !defined(PNG_1_0_X)
         if ((png_ptr->asm_flags & PNG_ASM_FLAG_MMX_READ_FILTER_PAETH) &&
             (row_info->pixel_depth >= png_ptr->mmx_bitdepth_threshold) &&
             (row_info->rowbytes >= png_ptr->mmx_rowbytes_threshold))
#else
         if (mmx_supported)
#endif
         {
            png_read_filter_row_mmx_paeth(row_info, row, prev_row);
         }
         else
         {
            png_uint_32 i;
            png_bytep rp = row;
            png_bytep pp = prev_row;
            png_bytep lp = row;
            png_bytep cp = prev_row;
            png_uint_32 bpp = (row_info->pixel_depth + 7) >> 3;
            png_uint_32 istop=row_info->rowbytes - bpp;

            for (i = 0; i < bpp; i++)
            {
               *rp = (png_byte)(((int)(*rp) + (int)(*pp++)) & 0xff);
               rp++;
            }

            for (i = 0; i < istop; i++)   
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

               p = (pa <= pb && pa <=pc) ? a : (pb <= pc) ? b : c;

               *rp = (png_byte)(((int)(*rp) + p) & 0xff);
               rp++;
            }
         }
         break;
      }

      default:
         png_warning(png_ptr, "Ignoring bad row filter type");
         *row=0;
         break;
   }
}

#endif /* PNG_ASSEMBLER_CODE_SUPPORTED && PNG_USE_PNGVCRD */
