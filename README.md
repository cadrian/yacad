# Core

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
* Manages the conf for the CGI (note: only Core conf; runner conf is not managed)

# Runner

The executor. There can be more than one; they register to the core by name; each name is unique.

* Reads some conf to find out:
    * its name
    * the location of the Core
    * the shared directory
* Regularly asks the Core for tasks
    * runs those tasks
    * publishes results (artifacts or not, and sends results to Core)
    * sends logs to the Core during its task execution
* Answers to pings

# CGI

The gui.

* Talks with the Core to manage the conf and the task list
* Allow to download artifacts and task logs
