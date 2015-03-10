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

#include "yacad.h"

#define DATETIME_FORMAT "%Y-%m-%d %H:%M:%S"

const char *datetime(time_t t, char *tmbuf) {
     char *result = NULL;
     struct tm tm;
     if (localtime_r(&t, &tm) != NULL) {
          strftime(result = tmbuf, 20, DATETIME_FORMAT, &tm);
     }
     return tmbuf;
}
