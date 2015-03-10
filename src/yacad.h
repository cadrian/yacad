/*
  This file is part of yaCAD.

  yaCAD is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, version 3 of the License.

  yaCAD is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with yaCAD.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __YACAD_H__
#define __YACAD_H__

#include <time.h>
#include <sys/types.h>

#define I(impl) (&((impl)->fn))

typedef enum {
     false=0,
     true
} bool_t;

#define bool2str(flag) ((flag) ? "true" : "false")

const char *datetime(time_t t, char *tmbuf);
int mkpath(const char *dir, mode_t mode);

#endif /* __YACAD_H__ */
