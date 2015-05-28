# Aim

Light-weight continuous integration. Written in C. Must work on an ARM platform.

# Usage

By default, yaCAD listens on port 1789, not bound to any particular
interface.

The program comes in two main components:
* The core: the main server
* The runners: they talk to the runner to get work

# Design

## Core

The main program.

* Reads some conf to find out:
    * scheduling details
    * repositories to check
    * what to do when a runner task is completed
    * configuration independent from the published repositories
    * shared directory for artifacts publishing
* Its scheduler is responsible for checking repositories
* Manages a serialized list of tasks (sqlite)
* Receives results from Runners and prepares new tasks accordingly
* Pings Runners
* Manages the conf for the CGI (note: only Core conf; runner conf is
  not managed)

## Runner

The executor. There can be more than one; they register to the core by
name; each name is unique.

* Reads some conf to find out:
    * its name
    * the location of the Core
    * the shared directory
* Regularly asks the Core for tasks
    * runs those tasks
    * publishes results (artifacts or not, and sends results to Core)
    * sends logs to the Core during its task execution
* Answers to pings

## CGI

The gui.

* Talks with the Core to manage the conf and the task list
* Allow to download artifacts and task logs

# Details

## JSON messages structure

### Get Task

This message is initiated by a runner, asking the core for something
to do.

#### Runner -> Core: query

    {
        "type": "query_get_task",
        "runner": <runnerid>
    }

#### Core -> Runner: reply

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

### Set Result

This message is initiated by a runner, sending a task execution result
to the core.

#### Runner -> Core: query

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

#### Core -> Runner: reply

    {
        "type": "reply_set_result",
        "runner": <runnerid>
    }
