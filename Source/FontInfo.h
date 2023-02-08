/*
 *        Copyright (c) 2020-2023 HydraSystems.
 *
 *  This program is free software; you can redistribute it and/or   
 *  modify it under the terms of the GNU General Public License as  
 *  published by the Free Software Foundation; either version 2 of  
 *  the License, or (at your option) any later version.             
 *                                                                  
 *  This program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of  
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   
 *  GNU General Public License for more details.                    
 *                                                                  
 * =====================================================================
 *
 * This file contains code to implement font display.
 *
 * Edit              Date/Ver     Edit Description
 * ==============  ============  ==============================================
 * Ersn Duchan     2009/07/24    Original
 * Baoshi Zhu      2015/01/03    Modified
 * Stuart James    2019/12/15    Modified for Heltec ESP32 WiFi Dev Card
 *
*/

#pragma once

typedef struct FontCharDesc_t
{
  UCHAR Width;                                // Character width in pixel
  UINT Offset;                                // Offset of this character in bitmap
} FontCharDesc_t;


typedef struct FontInfo_t
{
  UCHAR       Height;                         // Character height in pixel, all characters have same height
  UCHAR       Start;                          // First character
  UCHAR       End;                            // Last character
  const FontCharDesc_t *pDescriptors;         // descriptor for each character
  const UCHAR *pBitmap;                       // Character bitmap
} FontInfo_t;

