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

#include <string.h>

#include "yacad_scm.h"
#include "yacad_scm_git.h"

yacad_scm_t *yacad_scm_new(yacad_conf_t *conf, const char *type, const char *root_path, const char *upstream_url) {
     if (!strcmp("git", type)) {
          return yacad_scm_git_new(conf, root_path, upstream_url);
     }

     conf->log(warn, "scm type not supported: %s\n", type);
     return NULL;
}
