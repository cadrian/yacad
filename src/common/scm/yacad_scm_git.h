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

#ifndef __YACAD_SCM_GIT_H__
#define __YACAD_SCM_GIT_H__

#include "yacad_scm.h"

yacad_scm_t *yacad_scm_git_new(logger_t log, const char *root_path, json_value_t *desc);

#endif /* __YACAD_SCM_GIT_H__ */
