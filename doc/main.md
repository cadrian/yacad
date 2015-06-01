Overview
========

yaCAD is __Yet Another Continuous Automation Design__.

# Copyright #

yaCAD is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation, version 3 of the License.

yaCAD is distributed in the hope that it will be useful, but __without
any warranty__; without even the implied warranty of
__merchantability__ or __fitness for a particular purpose__.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with yaCAD.  If not, see http://www.gnu.org/licenses/

Copyleft © 2015 Cyril ADRIAN aka CAD

# Design #

## Modules ##

### Core ###

The main program.

* Reads some conf to find out:
    * scheduling details
    * repositories to check
    * what to do when a Runner task is completed
    * configuration independent from the published repositories
    * shared directory for artifacts publishing
* Its scheduler is responsible for checking repositories
* Manages a serialized list of tasks (sqlite)
* Receives results from Runners and prepares new tasks accordingly
* Manages the conf for the CGI (note: only Core conf; Runner conf is
  not managed)

### Runner ###

The executor. There can be more than one; they talk to the Core by
name; each name must be unique (the administrator must make sure of
that).

* Reads some conf to find out:
    * its name
    * the location of the Core
    * the shared directory
* Regularly asks the Core for tasks
    * runs those tasks
    * publishes results (artifacts or not, and sends results to Core)
    * sends logs to the Core during its task execution

### CGI ###

The gui.

* Talks with the Core to manage the conf and the task list
* Allow to download artifacts and task logs

## JSON messages structure ##

### Get Task ###

This message is initiated by a Runner, asking the Core for something
to do.

#### Runner -> Core: query ####

    {
        "type": "query_get_task",
        "runner": <runnerid>
    }

#### Core -> Runner: reply ####

    {
        "type": "reply_get_task",
        "runner": <runnerid>,
        "task": {
            "id": <taskid>,
            "timestamp": <date>,
            "status": <status>,
            "taskindex": <taskindex>,
            "task": <task>,
            "project_name": <projectname>,
            "env": <environment>
        },
        "scm": <scm>
    }

### Set Result ###

This message is initiated by a Runner, sending a task execution result
to the Core.

#### Runner -> Core: query ####

    {
        "type": "query_set_result",
        "runner": <runnerid>,
        "task": {
            "id": <taskid>,
            "timestamp": <date>,
            "status": <status>,
            "taskindex": <taskindex>,
            "task": <task>,
            "project_name": <projectname>,
            "env": <environment>
        },
        "success": true / false
    }

#### Core -> Runner: reply ####

    {
        "type": "reply_set_result",
        "runner": <runnerid>
    }
