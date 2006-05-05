#ifndef _HAD_TYPES_H
#define _HAD_TYPES_H

/*
  $NiH: types.h,v 1.9 2006/04/24 11:38:38 dillo Exp $

  types.h -- type definitions
  Copyright (C) 1999-2006 Dieter Baron and Thomas Klausner

  This file is part of ckmame, a program to check rom sets for MAME.
  The authors can be contacted at <nih@giga.or.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



enum status {
    STATUS_OK, STATUS_BADDUMP, STATUS_NODUMP
};

typedef enum status status_t;

enum quality {
    QU_MISSING,		/* ROM is missing */
    QU_NOHASH,		/* disk and file have no common checksums */
    QU_HASHERR,		/* rom/disk and file have different checksums */
    QU_LONG,		/* long ROM with valid subsection */
    QU_NAMEERR,		/* wrong name */
    QU_COPIED,		/* copied from elsewhere */
    QU_INZIP,		/* is in zip, should be in ancestor */
    QU_OK,		/* name/size/crc match */
    QU_OLD		/* exists in old */
};

typedef enum quality quality_t;

enum file_status {
    FS_MISSING,		/* file does not exist (only used for disks) */
    FS_UNKNOWN,		/* unknown */
    FS_BROKEN,		/* file in zip broken (invalid data / crc error) */
    FS_PARTUSED,	/* part needed here, whole file unknown */
    FS_SUPERFLUOUS,	/* known, not needed here, and exists elsewhere */
    FS_NEEDED,		/* known and needed elsewhere */
    FS_USED,		/* needed here */
    FS_DUPLICATE	/* exists in old */
};

typedef enum file_status file_status_t;

enum game_status {
    GS_MISSING,		/* not a single own ROM found */
    GS_CORRECT,		/* all ROMs correct */
    GS_FIXABLE,		/* only fixable errors */
    GS_PARTIAL,		/* some ROMs missing */
    GS_OLD,		/* all ROMs in old */
};

typedef enum game_status game_status_t;

enum where {
    ROM_NOWHERE = -1,
    ROM_INZIP, ROM_INCO, ROM_INGCO,
    ROM_ROMSET,
    ROM_NEEDED,
    ROM_SUPERFLUOUS,
    ROM_EXTRA,
    ROM_OLD
};

typedef enum where where_t;

#define IS_ELSEWHERE(w)	((w) >= ROM_ROMSET)

enum filetype {
    TYPE_ROM, TYPE_SAMPLE, TYPE_DISK,
    TYPE_MAX,
    /* for archive_new only */
    TYPE_FULL_PATH
};

typedef enum filetype filetype_t;

#define FIX_DO			0x01 /* really make fixes */
#define FIX_PRINT		0x02 /* print fixes made */
#define FIX_MOVE_LONG		0x04 /* move partially used files to garbage */
#define FIX_MOVE_UNKNOWN	0x08 /* move unknown files to garbage */
#define FIX_DELETE_EXTRA	0x10 /* delete used from extra dirs */
#define FIX_CLEANUP_EXTRA	0x20 /* delete superfluous from extra dirs */ 
#define FIX_SUPERFLUOUS		0x40 /* move/delete superfluous */
#if 0 /* not supported (yet?) */
#define FIX_COMPLETE_GAMES	0x80 /* complete in old or complete in roms */
#endif

/* XXX: delete */
enum state {
        ROM_0,
	    ROM_UNKNOWN, ROM_SHORT, ROM_LONG, ROM_CRCERR, ROM_NOCRC,
	    ROM_NAMERR, ROM_LONGOK, ROM_BESTBADDUMP, ROM_OK, ROM_TAKEN
	};

typedef enum state state_t;



const char *filetype_db_key(filetype_t);

#endif /* types.h */
