# Backup Manger
Interactive backup management system for Linux. The system allows creating multiple backups of selected local directories and keeps them in sync by monitoring filesystem changes using C POSIX system calls and Linux `inotify`.

## Setup
1. Requirements:
    - Linux operating system with `inotify` support
    - POSIX-compliant enviroment
    - C compiler (`gcc` or `clang`)
    - `make`

2. To build:
    ```
    make
    ```

3. To run:
    ```
    ./main
    ```

## What we can do?
- Add backup and start watching some local directory use:
    ```
    add <source path> <target path 1> <target path 2> ... <target path n>
    ```

- Stop watching some backup use:
    ```
    end <source path> <target path 1> <target path 2> ... <target path n>
    ```

- Restore source directory we can use (after this command the source dir will be like the targer dir):
    ```
    restore <source path> <target path>
    ```

- List watched backups:
    ```
    list
    ```

- Exit:
    ```
    exit
    ```
    or send `SIGTERM` or `SIGINT`

## Future plans
- Change data structures for faster adding and ending backups
- Improve and optimize some functions