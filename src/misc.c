/*
 *  Hamlib Interface - toolbox
 *  Copyright (c) 2000-2002 by Stephane Fillod and Frank Singleton
 *
 *		$Id: misc.c,v 1.18.2.4 2002-08-02 09:29:42 dedmons Exp $
 * 
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <sys/types.h>
#include <unistd.h>

#include <hamlib/rig.h>

#include "misc.h"

static int rig_debug_level = RIG_DEBUG_TRACE;

/*
 * Do a hex dump of the unsigned char array.
 */

#define DUMP_HEX_WIDTH 16

void dump_hex(const unsigned char ptr[], size_t size)
{
  int i;
  char buf[DUMP_HEX_WIDTH+1];

  if (!rig_need_debug(RIG_DEBUG_TRACE))
		  return;

  buf[DUMP_HEX_WIDTH] = '\0';

  for(i=0; i<size; i++) {
    if (i % DUMP_HEX_WIDTH == 0)
      rig_debug(RIG_DEBUG_TRACE,"%.4x\t",i);

    rig_debug(RIG_DEBUG_TRACE," %.2x", ptr[i]);

	if (ptr[i] >= ' ' && ptr[i] < 0x7f)
		buf[i%DUMP_HEX_WIDTH] = ptr[i];
	else
		buf[i%DUMP_HEX_WIDTH] = '.';

    if (i % DUMP_HEX_WIDTH == DUMP_HEX_WIDTH-1)
      rig_debug(RIG_DEBUG_TRACE,"\t%s\n",buf);
  }

  /* Add some spaces in order to align right ASCII dump column */
  if ((i / DUMP_HEX_WIDTH) > 0) {
    int j;
    for (j = i % DUMP_HEX_WIDTH; j < DUMP_HEX_WIDTH; j++)
      rig_debug(RIG_DEBUG_TRACE,"   ");
  }

  if (i % DUMP_HEX_WIDTH != DUMP_HEX_WIDTH-1) {
  	buf[i % DUMP_HEX_WIDTH] = '\0';
    rig_debug(RIG_DEBUG_TRACE,"\t%s\n",buf);
  }

} 


/*
 * Convert a long long (eg. frequency in Hz) to 4-bit BCD digits, 
 * packed two digits per octet, in little-endian order.
 * bcd_len is the number of BCD digits, usually 10 or 8 in 1-Hz units, 
 *	and 6 digits in 100-Hz units for Tx offset data.
 *
 * Hope the compiler will do a good job optimizing it (esp. w/ the 64bit freq)
 */
unsigned char *
to_bcd(unsigned char bcd_data[], unsigned long long freq, int bcd_len)
{
	int i;
	unsigned char a;

	/* '450'-> 0,5;4,0 */

	for (i=0; i < bcd_len/2; i++) {
			a = freq%10;
			freq /= 10;
			a |= (freq%10)<<4;
			freq /= 10;
			bcd_data[i] = a;
	}
	if (bcd_len&1) {
			bcd_data[i] &= 0xf0;
			bcd_data[i] |= freq%10;	/* NB: high nibble is left uncleared */
	}

	return bcd_data;
}

/*
 * Convert BCD digits to a long long (eg. frequency in Hz)
 * bcd_len is the number of BCD digits.
 *
 * Hope the compiler will do a good job optimizing it (esp. w/ the 64bit freq)
 */
unsigned long long from_bcd(const unsigned char bcd_data[], int bcd_len)
{
	int i;
	freq_t f = 0;

	if (bcd_len&1)
			f = bcd_data[bcd_len/2] & 0x0f;

	for (i=(bcd_len/2)-1; i >= 0; i--) {
			f *= 10;
			f += bcd_data[i]>>4;
			f *= 10;
			f += bcd_data[i] & 0x0f;
	}
	
	return f;
}

/*
 * Same as to_bcd, but in Big Endian mode
 */
unsigned char *
to_bcd_be(unsigned char bcd_data[], unsigned long long freq, int bcd_len)
{
	int i;
	unsigned char a;

	/* '450'-> 0,4;5,0 */

	for (i=(bcd_len/2)-1; i >= 0; i--) {
			a = freq%10;
			freq /= 10;
			a |= (freq%10)<<4;
			freq /= 10;
			bcd_data[i] = a;
	}
	if (bcd_len&1) {
			bcd_data[0] &= 0xf0;
			bcd_data[0] |= freq%10;	/* NB: high nibble is left uncleared */
	}

	return bcd_data;
}

/*
 * Same as from_bcd, but in Big Endian mode
 */
unsigned long long from_bcd_be(const unsigned char bcd_data[], int bcd_len)
{
	int i;
	freq_t f = 0;

	if (bcd_len&1)
			f = bcd_data[0] & 0x0f;

	for (i=bcd_len&1; i < (bcd_len+1)/2; i++) {
			f *= 10;
			f += bcd_data[i]>>4;
			f *= 10;
			f += bcd_data[i] & 0x0f;
	}
	
	return f;
}

/*
 * rig_set_debug
 * Change the current debug level
 */
void rig_set_debug(enum rig_debug_level_e debug_level)
{
		rig_debug_level = debug_level;
}

/*
 * rig_need_debug
 * Usefull for dump_hex, etc.
 */
int rig_need_debug(enum rig_debug_level_e debug_level)
{
		return (debug_level <= rig_debug_level);
}

/*
 * rig_debug
 * Debugging messages are done through stderr
 * TODO: add syslog support if needed
 */
void rig_debug(enum rig_debug_level_e debug_level, const char *fmt, ...)
{
		va_list ap;

		if (debug_level <= rig_debug_level) {
				va_start(ap, fmt);
				/*
				 * Who cares about return code?
				 */
				vfprintf (stderr, fmt, ap);
				va_end(ap);
		}
}

#define llabs(a) ((a)<0?-(a):(a))

/*
 * rig_freq_snprintf?
 * pretty print frequencies
 * str must be long enough. max can be as long as 17 chars
 */
int sprintf_freq(char *str, freq_t freq)
{
		double f;
		char *hz;

		if (llabs(freq) >= GHz(1)) {
				hz = "GHz";
				f = (double)freq/GHz(1);
		} else if (llabs(freq) >= MHz(1)) {
				hz = "MHz";
				f = (double)freq/MHz(1);
		} else if (llabs(freq) >= kHz(1)) {
				hz = "kHz";
				f = (double)freq/kHz(1);
		} else {
				hz = "Hz";
				f = (double)freq;
		}

		return sprintf (str, "%g %s", f, hz);
}

const char * strmode(rmode_t mode)
{
	switch (mode) {
    case RIG_MODE_AM: return "AM";
	case RIG_MODE_CW: return "CW";
	case RIG_MODE_USB: return "USB";
	case RIG_MODE_LSB: return "LSB";
	case RIG_MODE_RTTY: return "RTTY";
	case RIG_MODE_FM: return "FM";
	case RIG_MODE_WFM: return "WFM";
	case RIG_MODE_NONE: return "";
	default: break;
	}
	return NULL;
}

/*
 * shouldn't this use the same table as parse_vfo()?
 * It already caused me one bug.  :)		--Dale
 */
const char *strvfo(vfo_t vfo)
{
	unsigned int i, j, k, c;
	static char tmp[80];
	vfo_t tvfo;	// tmp for display purposes

	// Mask out PTT/CTRL so standard VFO's will work
	tvfo = vfo & (RIG_VFO_VALID & ~(RIG_VFO_CTRL | RIG_VFO_PTT));

/* special cases and debug	--Dale */
#define _SHOW_BITS
#ifdef _SHOW_BITS
	j=0; k=0;
	for(i = 1<<(RIG_MAJOR+1);
		(i >= 0x0001) && (j < 60) && (k < RIG_MAJOR); i = i>>1) {
		switch(vfo & i) {
		case RIG_VFO_CTRL:	j += sprintf(&tmp[j], "CTRL"); break;
		case RIG_VFO_PTT:	j += sprintf(&tmp[j], "PTT"); break;
		case RIG_VFO1:	j += sprintf(&tmp[j], "VFO1"); break;
		case RIG_VFO2:	j += sprintf(&tmp[j], "VFO2"); break;
		case RIG_VFO3:	j += sprintf(&tmp[j], "VFO3"); break;
		case RIG_VFO4:	j += sprintf(&tmp[j], "VFO4"); break;
		case RIG_VFO5:	j += sprintf(&tmp[j], "VFO5"); break;
		case RIG_CTRL_FAKE:	j += sprintf(&tmp[j], "FAKE"); break;
		case RIG_CTRL_MAIN:	j += sprintf(&tmp[j], "MAIN"); break;
		case RIG_CTRL_SUB:	j += sprintf(&tmp[j], "SUB"); break;
		case RIG_CTRL_MEM:	j += sprintf(&tmp[j], "MEM"); break;
		case RIG_CTRL_CALL:	j += sprintf(&tmp[j], "CALL"); break;
		case RIG_CTRL_REV:	j += sprintf(&tmp[j], "REV"); break;
		case RIG_CTRL_RIT:	j += sprintf(&tmp[j], "RIT"); break;
		case RIG_CTRL_XIT:	j += sprintf(&tmp[j], "XIT"); break;
		case RIG_CTRL_SPLIT:	j += sprintf(&tmp[j], "SPLT"); break;
		case RIG_CTRL_SCAN:	j += sprintf(&tmp[j], "SCAN"); break;
		case RIG_CTRL_SAT:	j += sprintf(&tmp[j], "SAT"); break;
		case RIG_CTRL_CROSS:	j += sprintf(&tmp[j], "RPTR"); break;

		default:	j += sprintf(&tmp[j], "%c", (vfo & i)? '1': '0'); break;
		}
		j += sprintf(&tmp[j], ",");
		k++;
	}
#endif

	// the --j is to remove the last erroneous comma
	j += sprintf(&tmp[--j], " = ");
//	rig_debug(RIG_DEBUG_TRACE, "%s--> ", tmp);

	c=0;	// Nothing below set yet

	switch (tvfo) {
	case	RIG_VFO_A:
		j += sprintf(&tmp[j], "VFOA"); c=1; break;
	case	RIG_VFO_B:
		j += sprintf(&tmp[j], "VFOB"); c=1; break;
	case	RIG_VFO_C:
		j += sprintf(&tmp[j], "VFOC"); c=1; break;
	case	RIG_VFO_CURR:
		j += sprintf(&tmp[j], "VFOcurr"); c=1; break;
	case	RIG_VFO_ALL:
		j += sprintf(&tmp[j], "VFOall"); c=1; break;
	case	RIG_VFO_MEM:
		j += sprintf(&tmp[j], "MEM"); c=1; break;
	case	RIG_VFO_VFO:
		j += sprintf(&tmp[j], "VFO"); c=1; break;
	case	RIG_VFO_MAIN:
		j += sprintf(&tmp[j], "Main"); c=1; break;
	case	RIG_VFO_SUB:
		j += sprintf(&tmp[j], "Sub"); c=1; break;
	case	RIG_VFO_MEM_A:
		j += sprintf(&tmp[j], "MEMA"); c=1; break;
	case	RIG_VFO_MEM_C:
		j += sprintf(&tmp[j], "MEMC"); c=1; break;
	case	RIG_VFO_CALL_A:
		j += sprintf(&tmp[j], "CALLA"); c=1; break;
	case	RIG_VFO_CALL_C:
		j += sprintf(&tmp[j], "CALLC"); c=1; break;
	case	RIG_VFO_AB:
		j += sprintf(&tmp[j], "VFOAB"); c=1; break;
	case	RIG_VFO_BA:
		j += sprintf(&tmp[j], "VFOBA"); c=1; break;
	}

	// Special modes (non-standard RIG_VFO_*)
	if(c==0) {
		if( vfo & RIG_CTRL_SAT)	
			j += sprintf(&tmp[j], "SAT");
		else
			j += sprintf(&tmp[j], "Special");
	}

	tmp[j+1] = '\0';

	return tmp;
}

const char *strfunc(setting_t func)
{
	switch (func) {
	case RIG_FUNC_FAGC: return "FAGC";
	case RIG_FUNC_NB: return "NB";
	case RIG_FUNC_COMP: return "COMP";
	case RIG_FUNC_VOX: return "VOX";
	case RIG_FUNC_TONE: return "TONE";
	case RIG_FUNC_TSQL: return "TSQL";
	case RIG_FUNC_SBKIN: return "SBKIN";
	case RIG_FUNC_FBKIN: return "FBKIN";
	case RIG_FUNC_ANF: return "ANF";
	case RIG_FUNC_NR: return "NR";
	case RIG_FUNC_AIP: return "AIP";
	case RIG_FUNC_APF: return "APF";
	case RIG_FUNC_MON: return "MON";
	case RIG_FUNC_MN: return "MN";
	case RIG_FUNC_RNF: return "RNF";
	case RIG_FUNC_ARO: return "ARO";
	case RIG_FUNC_LOCK: return "LOCK";
	case RIG_FUNC_MUTE: return "MUTE";
	case RIG_FUNC_VSC: return "VSC";
	case RIG_FUNC_REV: return "REV";
	case RIG_FUNC_SQL: return "SQL";
	case RIG_FUNC_BC: return "BC";
	case RIG_FUNC_MBC: return "MBC";
	case RIG_FUNC_LMP: return "LMP";
	case RIG_FUNC_AFC: return "AFC";
	case RIG_FUNC_SATMODE: return "SATMODE";
	case RIG_FUNC_SCOPE: return "SCOPE";
	case RIG_FUNC_RESUME: return "RESUME";

	case RIG_FUNC_NONE: return "";
	default: break;
	}
	return NULL;
}

const char *strlevel(setting_t level)
{
	switch (level) {
	case RIG_LEVEL_PREAMP: return "PREAMP";
	case RIG_LEVEL_ATT: return "ATT";
	case RIG_LEVEL_VOX: return "VOX";
	case RIG_LEVEL_AF: return "AF";
	case RIG_LEVEL_RF: return "RF";
	case RIG_LEVEL_SQL: return "SQL";
	case RIG_LEVEL_IF: return "IF";
	case RIG_LEVEL_APF: return "APF";
	case RIG_LEVEL_NR: return "NR";
	case RIG_LEVEL_PBT_IN: return "PBT_IN";
	case RIG_LEVEL_PBT_OUT: return "PBT_OUT";
	case RIG_LEVEL_CWPITCH: return "CWPITCH";
	case RIG_LEVEL_RFPOWER: return "RFPOWER";
	case RIG_LEVEL_MICGAIN: return "MICGAIN";
	case RIG_LEVEL_KEYSPD: return "KEYSPD";
	case RIG_LEVEL_NOTCHF: return "NOTCHF";
	case RIG_LEVEL_COMP: return "COMP";
	case RIG_LEVEL_AGC: return "AGC";
	case RIG_LEVEL_BKINDL: return "BKINDL";
	case RIG_LEVEL_BALANCE: return "BALANCE";
	case RIG_LEVEL_METER: return "METER";
	case RIG_LEVEL_VOXGAIN: return "VOXGAIN";
	case RIG_LEVEL_ANTIVOX: return "ANTIVOX";

	case RIG_LEVEL_SWR: return "SWR";
	case RIG_LEVEL_ALC: return "ALC";
	case RIG_LEVEL_SQLSTAT: return "SQLSTAT";
	case RIG_LEVEL_STRENGTH: return "STRENGTH";

	case RIG_LEVEL_NONE: return "";
	default: break;
	}
	return NULL;
}

const char *strparm(setting_t parm)
{
	switch (parm) {
	case RIG_PARM_ANN: return "ANN";
	case RIG_PARM_APO: return "APO";
	case RIG_PARM_BACKLIGHT: return "BACKLIGHT";
	case RIG_PARM_BEEP: return "BEEP";
	case RIG_PARM_TIME: return "TIME";
	case RIG_PARM_BAT: return "BAT";

	case RIG_PARM_NONE: return "";
	default: break;
	}
	return NULL;
}

const char *strptrshift(rptr_shift_t shift)
{
	switch (shift) {
	case RIG_RPT_SHIFT_MINUS: return "-";
	case RIG_RPT_SHIFT_PLUS: return "+";
	case RIG_RPT_SHIFT_1750: return "=";

	case RIG_RPT_SHIFT_NONE:
	default:
		return "None";
		break;
	}
	return NULL;
}

const char *strvfop(vfo_op_t op)
{
	switch (op) {
	case RIG_OP_CPY: return "CPY";
	case RIG_OP_XCHG: return "XCHG";
	case RIG_OP_FROM_VFO: return "FROM_VFO";
	case RIG_OP_TO_VFO: return "TO_VFO";
	case RIG_OP_MCL: return "MCL";
	case RIG_OP_UP: return "UP";
	case RIG_OP_DOWN: return "DOWN";
	case RIG_OP_BAND_UP: return "BAND_UP";
	case RIG_OP_BAND_DOWN: return "BAND_DOWN";
	case RIG_OP_LEFT: return "LEFT";
	case RIG_OP_RIGHT: return "RIGHT";

	case RIG_OP_NONE: return "";
	default: break;
	}
	return NULL;
}

const char *strscan(scan_t rscan)
{
	switch (rscan) {
	case RIG_SCAN_STOP: return "STOP";
	case RIG_SCAN_MEM: return "MEM";
	case RIG_SCAN_SLCT: return "SLCT";
	case RIG_SCAN_PRIO: return "PRIO";
	case RIG_SCAN_PROG: return "PROG";
	case RIG_SCAN_DELTA: return "DELTA";
	case RIG_SCAN_VFO: return "VFO";
	default: break;
	}
	return NULL;
}

/* *INDENT-OFF* */
const char *strstatus(enum rig_status_e status)
{
	switch (status) {
	case RIG_STATUS_ALPHA:
			return "Alpha";
	case RIG_STATUS_UNTESTED:
			return "Untested";
	case RIG_STATUS_BETA:
			return "Beta";
	case RIG_STATUS_STABLE:
			return "Stable";
	case RIG_STATUS_BUGGY:
			return "Buggy";
	case RIG_STATUS_NEW:
			return "New";
	default:
			return "";
	break;
	}
}
/* *INDENT-ON* */

int sprintf_mode(char *str, rmode_t mode)
{
		int i, len=0;

		*str = '\0';
		if (mode == RIG_MODE_NONE)
				return 0;

		for (i = 0; i < 30; i++) {
				const char *ms = strmode(mode & (1UL<<i));
				if (!ms || !ms[0])
						continue;	/* unknown, FIXME! */
				strcat(str, ms);
				strcat(str, " ");
				len += strlen(ms) + 1;
		}
		return len;
}

int sprintf_func(char *str, setting_t func)
{
		int i, len=0;

		*str = '\0';
		if (func == RIG_FUNC_NONE)
				return 0;

		for (i = 0; i < 60; i++) {
				const char *ms = strfunc(func & rig_idx2setting(i));
				if (!ms || !ms[0])
						continue;	/* unknown, FIXME! */
				strcat(str, ms);
				strcat(str, " ");
				len += strlen(ms) + 1;
		}
		return len;
}


int sprintf_level(char *str, setting_t level)
{
		int i, len=0;

		*str = '\0';
		if (level == RIG_LEVEL_NONE)
				return 0;

		for (i = 0; i < 60; i++) {
				const char *ms = strlevel(level & rig_idx2setting(i));
				if (!ms || !ms[0])
						continue;	/* unknown, FIXME! */
				strcat(str, ms);
				strcat(str, " ");
				len += strlen(ms) + 1;
		}
		return len;
}


int sprintf_parm(char *str, setting_t parm)
{
		int i, len=0;

		*str = '\0';
		if (parm == RIG_PARM_NONE)
				return 0;

		for (i = 0; i < 60; i++) {
				const char *ms = strparm(parm & rig_idx2setting(i));
				if (!ms || !ms[0])
						continue;	/* unknown, FIXME! */
				strcat(str, ms);
				strcat(str, " ");
				len += strlen(ms) + 1;
		}
		return len;
}


int sprintf_vfop(char *str, vfo_op_t op)
{
		int i, len=0;

		*str = '\0';
		if (op == RIG_OP_NONE)
				return 0;

		for (i = 0; i < 30; i++) {
				const char *ms = strvfop(op & (1UL<<i));
				if (!ms || !ms[0])
						continue;	/* unknown, FIXME! */
				strcat(str, ms);
				strcat(str, " ");
				len += strlen(ms) + 1;
		}
		return len;
}


int sprintf_scan(char *str, scan_t rscan)
{
		int i, len=0;

		*str = '\0';
		if (rscan == RIG_SCAN_NONE)
				return 0;

		for (i = 0; i < 30; i++) {
				const char *ms = strscan(rscan & (1UL<<i));
				if (!ms || !ms[0])
						continue;	/* unknown, FIXME! */
				strcat(str, ms);
				strcat(str, " ");
				len += strlen(ms) + 1;
		}
		return len;
}



static struct { 
		rmode_t mode;
		const char *str;
} mode_str[] = {
	{ RIG_MODE_AM, "AM" },
	{ RIG_MODE_FM, "FM" },
	{ RIG_MODE_CW, "CW" },
	{ RIG_MODE_USB, "USB" },
	{ RIG_MODE_LSB, "LSB" },
	{ RIG_MODE_RTTY, "RTTY" },
	{ RIG_MODE_WFM, "WFM" },
	{ RIG_MODE_NONE, NULL },
	{0,0}
};


rmode_t parse_mode(const char *s)
{
	int i;

	for (i=0 ; mode_str[i].str != NULL; i++)
			if (!strcmp(s, mode_str[i].str))
					return mode_str[i].mode;
	return RIG_MODE_NONE;
}

static struct { 
		vfo_t vfo ;
		const char *str;
} vfo_str[] = {
	{ RIG_VFO_A, "VFOA" },
	{ RIG_VFO_B, "VFOB" },
	{ RIG_VFO_C, "VFOC" },
	{ RIG_VFO_AB, "VFOAB" },
	{ RIG_VFO_BA, "VFOBA" },
	{ RIG_VFO_MEM_A, "MEMA" },
	{ RIG_VFO_MEM_C, "MEMC" },
	{ RIG_CTRL_SAT, "SAT" },
	{ RIG_VFO_CALL_A, "CALLA" },
	{ RIG_VFO_CALL_C, "CALLC" },
	{ RIG_VFO_MAIN, "Main" },
	{ RIG_VFO_SUB, "Sub" },
// one or more of the following may be ambiguous	--Dale
	{ RIG_VFO_CURR, "currVFO" },
	{ RIG_VFO_VFO, "VFO" },
	{ RIG_VFO_MEM, "MEM" },
//	{ RIG_VFO_ALL, "allVFO" },

	{ RIG_VFO_NONE, NULL },
	{0,0}
};

vfo_t parse_vfo(const char *s)
{
	int i;

	for (i=0 ; vfo_str[i].str != NULL; i++)
			if (!strcmp(s, vfo_str[i].str))
					return vfo_str[i].vfo;
	return RIG_VFO_NONE;
}


static struct { 
		setting_t func; 
		const char *str;
} func_str[] = {
	{ RIG_FUNC_FAGC, "FAGC" },
	{ RIG_FUNC_NB, "NB" },
	{ RIG_FUNC_COMP, "COMP" },
	{ RIG_FUNC_VOX, "VOX" },
	{ RIG_FUNC_TONE, "TONE" },
	{ RIG_FUNC_TSQL, "TSQL" },
	{ RIG_FUNC_SBKIN, "SBKIN" },
	{ RIG_FUNC_FBKIN, "FBKIN" },
	{ RIG_FUNC_ANF, "ANF" },
	{ RIG_FUNC_NR, "NR" },
	{ RIG_FUNC_AIP, "AIP" },
	{ RIG_FUNC_MON, "MON" },
	{ RIG_FUNC_MN, "MN" },
	{ RIG_FUNC_RNF, "RNF" },
	{ RIG_FUNC_ARO, "ARO" },
	{ RIG_FUNC_LOCK, "LOCK" },
	{ RIG_FUNC_MUTE, "MUTE" },
	{ RIG_FUNC_VSC, "VSC" },
	{ RIG_FUNC_REV, "REV" },
	{ RIG_FUNC_SQL, "SQL" },
	{ RIG_FUNC_BC, "BC" },
	{ RIG_FUNC_MBC, "MBC" },
	{ RIG_FUNC_LMP, "LMP" },
	{ RIG_FUNC_AFC, "AFC" },
	{ RIG_FUNC_SATMODE, "SATMODE" },
	{ RIG_FUNC_SCOPE, "SCOPE" },
	{ RIG_FUNC_RESUME, "RESUME" },
	{ RIG_FUNC_NONE, NULL },
	{0,0}
};

setting_t parse_func(const char *s)
{
	int i;

	for (i=0 ; func_str[i].str != NULL; i++)
			if (!strcmp(s, func_str[i].str))
					return func_str[i].func;
	return RIG_FUNC_NONE;
}

static struct { 
		setting_t level;
		const char *str;
} level_str[] = {
	{ RIG_LEVEL_PREAMP, "PREAMP" },
	{ RIG_LEVEL_ATT, "ATT" },
	{ RIG_LEVEL_VOX, "VOX" },
	{ RIG_LEVEL_AF, "AF" },
	{ RIG_LEVEL_RF, "RF" },
	{ RIG_LEVEL_SQL, "SQL" },
	{ RIG_LEVEL_IF, "IF" },
	{ RIG_LEVEL_APF, "APF" },
	{ RIG_LEVEL_NR, "NR" },
	{ RIG_LEVEL_PBT_IN, "PBT_IN" },
	{ RIG_LEVEL_PBT_OUT, "PBT_OUT" },
	{ RIG_LEVEL_CWPITCH, "CWPITCH" },
	{ RIG_LEVEL_RFPOWER, "RFPOWER" },
	{ RIG_LEVEL_MICGAIN, "MICGAIN" },
	{ RIG_LEVEL_KEYSPD, "KEYSPD" },
	{ RIG_LEVEL_NOTCHF, "NOTCHF" },
	{ RIG_LEVEL_COMP, "COMP" },
	{ RIG_LEVEL_AGC, "AGC" },
	{ RIG_LEVEL_BKINDL, "BKINDL" },
	{ RIG_LEVEL_BALANCE, "BAL" },
	{ RIG_LEVEL_METER, "METER" },
	{ RIG_LEVEL_VOXGAIN, "VOXGAIN" },
	{ RIG_LEVEL_ANTIVOX, "ANTIVOX" },

	{ RIG_LEVEL_SWR, "SWR" },
	{ RIG_LEVEL_ALC, "ALC" },
	{ RIG_LEVEL_SQLSTAT, "SQLSTAT" },
	{ RIG_LEVEL_STRENGTH, "STRENGTH" },
	{ RIG_LEVEL_NONE, NULL },
	{0,0}
};

setting_t parse_level(const char *s)
{
	int i;

	for (i=0 ; level_str[i].str != NULL; i++)
			if (!strcmp(s, level_str[i].str))
					return level_str[i].level;
	return RIG_LEVEL_NONE;
}

static struct { 
		setting_t parm;
		const char *str;
} parm_str[] = {
	{ RIG_PARM_ANN, "ANN" },
	{ RIG_PARM_APO, "APO" },
	{ RIG_PARM_BACKLIGHT, "BACKLIGHT" },
	{ RIG_PARM_BEEP, "BEEP" },
	{ RIG_PARM_TIME, "TIME" },
	{ RIG_PARM_BAT, "BAT" },
	{ RIG_PARM_NONE, NULL },
	{0,0}
};

setting_t parse_parm(const char *s)
{
	int i;

	for (i=0 ; parm_str[i].str != NULL; i++)
			if (!strcmp(s, parm_str[i].str))
					return parm_str[i].parm;
	return RIG_PARM_NONE;
}

static struct { 
		vfo_op_t vfo_op;
		const char *str;
} vfo_op_str[] = {
	{ RIG_OP_CPY, "CPY" },
	{ RIG_OP_XCHG, "XCHG" },
	{ RIG_OP_FROM_VFO, "FROM_VFO" },
	{ RIG_OP_TO_VFO, "TO_VFO" },
	{ RIG_OP_MCL, "MCL" },
	{ RIG_OP_UP, "UP" },
	{ RIG_OP_DOWN, "DOWN" },
	{ RIG_OP_BAND_UP, "BAND_UP" },
	{ RIG_OP_BAND_DOWN, "BAND_DOWN" },
	{ RIG_OP_LEFT, "LEFT" },
	{ RIG_OP_RIGHT, "RIGHT" },
	{ RIG_OP_NONE, NULL },
	{0,0}
};

vfo_op_t parse_vfo_op(const char *s)
{
	int i;

	for (i=0 ; vfo_op_str[i].str != NULL; i++)
			if (!strcmp(s, vfo_op_str[i].str))
					return vfo_op_str[i].vfo_op;
	return RIG_OP_NONE;
}

static struct { 
		scan_t SCan;
		const char *str;
} scan_str[] = {
	{ RIG_SCAN_STOP, "STOP" },
	{ RIG_SCAN_MEM, "MEM" },
	{ RIG_SCAN_SLCT, "SLCT" },
	{ RIG_SCAN_PRIO, "PRIO" },
	{ RIG_SCAN_PROG, "PROG" },
	{ RIG_SCAN_DELTA, "DELTA" },
	{ RIG_SCAN_VFO, "VFO" },
	{ RIG_SCAN_NONE, NULL },
	{0,0}
};

scan_t parse_scan(const char *s)
{
	int i;

	printf(__FUNCTION__": parsing %s...\n",s);

	for (i=0 ; scan_str[i].str != NULL; i++) {
		if (strcmp(s, scan_str[i].str) == 0) {
			return scan_str[i].SCan;
		}
	}

	return RIG_SCAN_NONE;
}

rptr_shift_t parse_rptr_shift(const char *s)
{
	switch(s[0]) {
	case '+':	return RIG_RPT_SHIFT_PLUS;
	case '-':	return RIG_RPT_SHIFT_MINUS;
	case '=':	return RIG_RPT_SHIFT_1750;
	default:	return RIG_RPT_SHIFT_NONE;
		break;
	}
/* old
	if (strcmp(s, "+") == 0)
		return RIG_RPT_SHIFT_PLUS;
	else if (strcmp(s, "-") == 0)
		return RIG_RPT_SHIFT_MINUS;
	else if (strcmp(s, "=") == 0)
		return RIG_RPT_SHIFT_1750;
	else
		return RIG_RPT_SHIFT_NONE;
*/
}

/* simple all-in-one function to open rig */
int rig_setup(RIG *rig, rig_model_t model, char *port)
{
	int retval;

	rig_debug(RIG_DEBUG_WARN, __FUNCTION__
		": rig=%x, model=%i, port=%s\n", rig, model, port);

	rig = rig_init(model);
	if(rig == NULL) {
		rig_debug(RIG_DEBUG_ERR, __FUNCTION__
			": rig_init() returned NULL!\n");
		return -RIG_EINVAL;
	}

	strncpy(rig->state.rigport.pathname, port, FILPATHLEN);
	retval = rig_open(rig);
	if(retval != RIG_OK) {
		rig_debug(RIG_DEBUG_ERR, __FUNCTION__
			": rig_open() returned %s!\n", rigerror(retval));
	}

	rig_debug(RIG_DEBUG_WARN, __FUNCTION__
		": rig=%x, model=%i, port=%s\n", rig, model, port);

	return retval;
}

// end
