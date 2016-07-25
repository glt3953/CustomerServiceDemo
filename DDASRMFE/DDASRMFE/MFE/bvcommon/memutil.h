/*****************************************************************************/
/* BroadVoice(R)32 (BV32) Fixed-Point ANSI-C Source Code                     */
/* Revision Date: November 13, 2009                                          */
/* Version 1.1                                                               */
/*****************************************************************************/

/*****************************************************************************/
/* Copyright 2000-2009 Broadcom Corporation                                  */
/*                                                                           */
/* This software is provided under the GNU Lesser General Public License,    */
/* version 2.1, as published by the Free Software Foundation ("LGPL").       */
/* This program is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY SUPPORT OR WARRANTY; without even the implied warranty of     */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the LGPL for     */
/* more details.  A copy of the LGPL is available at                         */
/* http://www.broadcom.com/licenses/LGPLv2.1.php,                            */
/* or by writing to the Free Software Foundation, Inc.,                      */
/* 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                 */
/*****************************************************************************/


/*****************************************************************************
  memutil.h : Common Fixed-Point Library: memory utilities

  $Log$
******************************************************************************/
#include	"../bv32/bv32strct.h"

Word16 *allocWord16(long nl, long nh);
struct BV32_Encoder_State *allocEncoderState(long nl, long nh);
struct BV32_Decoder_State *allocDecoderState(long nl, long nh);
struct BV32_Bit_Stream *allocBitStream(long nl, long nh);
void deallocWord16(Word16 *v, long nl, long nh);
void deallocEncoderState(struct BV32_Encoder_State *v, long nl, long nh);
void deallocDecoderState(struct BV32_Decoder_State *v, long nl, long nh);
void deallocBitStream(struct BV32_Bit_Stream *v, long nl, long nh);
Word32 *allocWord32(long nl, long nh);
void deallocWord32(Word32 *v, long nl, long nh);
