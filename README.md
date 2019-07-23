# Fui - A text-user-interface file explorer
## Objectives
* Create a functional file explorer
* Obtain a better understanding of linux system calls
* Design a user friendly interface

## Technical constraints
* Use only the C programming language and the functionaliy provided by the standard library.
* NCurses may be used for the interface.
* Keep things "low level" - Vague, I know.

# Program Specifications
## Useage
Invoking `fui` without any arguments should open a file explorer in the current working directory.
Optionally, the directory which is to be explored may be provided as a command line argument to `fui`.

## Explorer interface
Upon opening a directory, fui should display all of the contents of that directory.
This includes the `.` and `..` directories.

The user may use the arrow keys (or H J K L) to highlight a file or subdirectory.
If the user presses enter on a directory, it shall be opened in the manner which has been explained.
