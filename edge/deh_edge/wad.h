//------------------------------------------------------------------------
//  WAD I/O
//------------------------------------------------------------------------
// 
//  DEH_EDGE  Copyright (C) 2004  The EDGE Team
// 
//  This program is under the GNU General Public License.
//  It comes WITHOUT ANY WARRANTY of any kind.
//  See COPYING.txt for the full details.
//
//------------------------------------------------------------------------

#ifndef __WAD_HDR__
#define __WAD_HDR__

#include "i_defs.h"

namespace WAD
{
	void Startup(void);
	void Shutdown(void);

	void NewLump(const char *name);
	void AddData(const byte *data, int len);
	void Printf(const char *str, ...);
	void FinishLump(void);

	void WriteFile(const char *name);
}

#endif /* __WAD_HDR__ */
