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

#ifndef __YACAD_LOG_H__
#define __YACAD_LOG_H__

/**
 * The log levels
 */
typedef enum {
     /**
      * Errors
      */
     error=0,
     /**
      * Warnings
      */
     warn,
     /**
      * Informations on how ExP runs
      */
     info,
     /**
      * Developer details, usually not useful
      */
     debug,
     /**
      * Developper useless details
      */
     trace,
} level_t;

/**
 * A logger is a `printf`-like function to call
 *
 * @param[in] level the logging level
 * @param[in] format the message format
 * @param[in] ... the message arguments
 *
 * @return the length of the message after expansion; 0 if the log is not emitted because of the level
 */
typedef int (*logger_t) (level_t level, const char *format, ...) __PRINTF__;

/**
 * The low-level logger. Usually printf(3).
 *
 * @param[in] format the message format
 * @param[in] ... the message arguments
 *
 * @return the length of the message after expansion; 0 if the log is not emitted because of the level
 */
typedef int (*logger_fn) (const char *format, ...) __attribute__((format(printf, 1, 2)));

/**
 * Get a logger for the given level.
 *
 * @return the logger for the given level.
 */
logger_t get_logger(level_t level);

/**
 * Set the low-level logging function. Must be called before \ref get_logger.
 *
 * Usually only for tests.
 *
 * @param[in] fn the logging function
 */
void set_logger_fn(logger_fn fn);

#endif /* __YACAD_LOG_H__ */
