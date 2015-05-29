# Aim

YaCAD is a light-weight continuous integration, written in C.

It is designed to work well on an ARM platform (Pi, Cubie...)

It works on Linux. There is no cross-platform provision; it is
developped mainly on Linux, but also on Cygwin.

# Usage

The program comes in two main components:
* The Core: the main server. By default, it listens on port 1789, not
  bound to any particular interface.
* The Runners: they talk to the Core to get work

# TODO

* Better unit tests
* Finish to code the Runner
* Find a (programmable) way to restart a task if a Runner who grabbed
  it is shut down. It should not involve a shut-down procedure because
  it must be resilient to failures.
  Maybe using its runnerid? (e.g. some message when the Runner comes
  online?)
