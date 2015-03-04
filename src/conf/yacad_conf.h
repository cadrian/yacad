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

#ifndef __YACAD_CONF_H__
#define __YACAD_CONF_H__

typedef struct yacad_conf_s yacad_conf_t;

typedef void (*yacad_conf_free_fn)(yacad_conf_t *this);

struct yacad_conf_s {
     yacad_conf_free_fn free;
};

yacad_conf_t *yacad_conf_new(void);

#endif /* __YACAD_CONF_H__ */