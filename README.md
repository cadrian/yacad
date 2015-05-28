# Aim

YaCAD is a light-weight continuous integration, written in C.

It is designed to work well on an ARM platform (Pi, Cubie...)

It works on Linux. There is no cross-platform provision; it is
developped mainly on Linux, but also on Cygwin.

# Usage

By default, yaCAD listens on port 1789, not bound to any particular
interface.

The program comes in two main components:
* The Core: the main server
* The Runners: they talk to the Core to get work

# Design

## Core

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

## Runner

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

## CGI

The gui.

* Talks with the Core to manage the conf and the task list
* Allow to download artifacts and task logs

# Details

## JSON messages structure

### Get Task

This message is initiated by a Runner, asking the Core for something
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

This message is initiated by a Runner, sending a task execution result
to the Core.

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

# TODO

* Better unit tests
* Finish to code the Runner
* Find a (programmable) way to restart a task if a Runner who grabbed
  it is shut down. It should not involve a shut-down procedure because
  it must be resilient to failures.
