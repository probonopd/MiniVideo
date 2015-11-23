/*!
 * COPYRIGHT (C) 2015 Emeric Grange - All Rights Reserved
 *
 * This file is part of MiniVideo.
 *
 * MiniVideo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MiniVideo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with MiniVideo.  If not, see <http://www.gnu.org/licenses/>.
 *
 * \file      riff.c
 * \author    Emeric Grange <emeric.grange@gmail.com>
 * \date      2015
 */

// minivideo headers
#include "riff.h"
#include "riff_struct.h"
#include "../../utils.h"
#include "../../bitstream_utils.h"
#include "../../typedef.h"
#include "../../minitraces.h"

// C standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ************************************************************************** */

/*!
 * \brief Parse a RIFF list header.
 *
 * bitstr pointer is not checked for performance reason.
 */
int parse_list_header(Bitstream_t *bitstr, RiffList_t *list_header)
{
    TRACE_3(WAV, "parse_list_header()\n");
    int retcode = SUCCESS;

    if (list_header == NULL)
    {
        TRACE_ERROR(WAV, "Invalid RiffList_t structure!\n");
        retcode = FAILURE;
    }
    else
    {
        // Parse RIFF list header
        list_header->offset_start = bitstream_get_absolute_byte_offset(bitstr);
        list_header->dwList       = read_bits(bitstr, 32);
        list_header->dwSize       = endian_flip_32(read_bits(bitstr, 32));
        list_header->dwFourCC     = read_bits(bitstr, 32);
        list_header->offset_end   = list_header->offset_start + list_header->dwSize + 8;

        if (list_header->dwList != fcc_RIFF &&
            list_header->dwList != fcc_LIST)
        {
            TRACE_1(WAV, "We are looking for a RIFF list, however this is neither a LIST nor a RIFF (0x%04X)\n", list_header->dwList);
            retcode = FAILURE;
        }
    }

    return retcode;
}

/* ************************************************************************** */

/*!
 * \brief Print an RIFF list header.
 */
void print_list_header(RiffList_t *list_header)
{
    if (list_header == NULL)
    {
        TRACE_ERROR(WAV, "Invalid RiffList_t structure!\n");
    }
    else
    {
        TRACE_2(WAV, "* offset_s   : %u\n", list_header->offset_start);
        TRACE_2(WAV, "* offset_e   : %u\n", list_header->offset_end);

        if (list_header->dwList == fcc_RIFF)
        {
            TRACE_2(WAV, "* RIFF header\n");
        }
        else
        {
            TRACE_2(WAV, "* LIST header\n");
        }

        char fcc[5];
        TRACE_2(WAV, "* LIST size : %u\n", list_header->dwSize);
        TRACE_2(WAV, "* LIST fcc  : 0x%08X ('%s')\n",
                list_header->dwFourCC,
                getFccString_le(list_header->dwFourCC, fcc));
    }
}

/* ************************************************************************** */

/*!
 * \brief Parse a RIFF chunk header.
 *
 * bitstr pointer is not checked for performance reason.
 */
int parse_chunk_header(Bitstream_t *bitstr, RiffChunk_t *chunk_header)
{
    TRACE_3(WAV, "parse_chunk_header()\n");
    int retcode = SUCCESS;

    if (chunk_header == NULL)
    {
        TRACE_ERROR(WAV, "Invalid RiffChunk_t structure!\n");
        retcode = FAILURE;
    }
    else
    {
        // Parse WAVE chunk header
        chunk_header->offset_start = bitstream_get_absolute_byte_offset(bitstr);
        chunk_header->dwFourCC     = read_bits(bitstr, 32);
        chunk_header->dwSize       = endian_flip_32(read_bits(bitstr, 32));
        chunk_header->offset_end   = chunk_header->offset_start + chunk_header->dwSize + 8;
    }

    return retcode;
}

/* ************************************************************************** */

/*!
 * \brief Print a RIFF chunk header.
 */
void print_chunk_header(RiffChunk_t *chunk_header)
{
    if (chunk_header == NULL)
    {
        TRACE_ERROR(WAV, "Invalid RiffChunk_t structure!\n");
    }
    else
    {
        TRACE_2(WAV, "* offset_s  : %u\n", chunk_header->offset_start);
        TRACE_2(WAV, "* offset_e  : %u\n", chunk_header->offset_end);

        char fcc[5];
        TRACE_2(WAV, "* CHUNK size: %u\n", chunk_header->dwSize);
        TRACE_2(WAV, "* CHUNK fcc : 0x%08X ('%s')\n",
                chunk_header->dwFourCC,
                getFccString_le(chunk_header->dwFourCC, fcc));
    }
}

/* ************************************************************************** */

/*!
 * \brief Skip a RIFF chunk header and its content.
 */
int skip_chunk(Bitstream_t *bitstr, RiffList_t *list_header_parent, RiffChunk_t *chunk_header_child)
{
    int retcode = FAILURE;

    if (chunk_header_child->dwSize != 0)
    {
        int64_t jump = chunk_header_child->dwSize * 8;
        int64_t offset = bitstream_get_absolute_byte_offset(bitstr);

        // Check that we do not jump outside the parent list boundaries
        if ((offset + jump) > list_header_parent->offset_end)
        {
            jump = list_header_parent->offset_end - offset;
        }

        if (skip_bits(bitstr, jump) == FAILURE)
        {
            TRACE_ERROR(WAV, "> skip_chunk() >> Unable to skip %i bytes!\n", chunk_header_child->dwSize);
            retcode = FAILURE;
        }
        else
        {
            TRACE_1(WAV, "> skip_chunk() >> %i bytes\n", chunk_header_child->dwSize);
            retcode = SUCCESS;
        }
    }
    else
    {
        TRACE_WARNING(WAV, "> skip_chunk() >> do it yourself!\n");
        retcode = FAILURE;
    }

    print_chunk_header(chunk_header_child);

    return retcode;
}

/* ************************************************************************** */
