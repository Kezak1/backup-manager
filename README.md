# Backup Manger
Interactive backup managemnt system for Linux. The system allows to create multiple backups for of selected local directories and maintain them by monitoring filesystem changes and synchronizing updates for the backup direcory.

## Setup
1. Requirements:
    - have Linux operating system with `inotify` support,
    - have POSIX-compliant enviroment,
    - have C compiler (`gcc` or `clang`).
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
- To add backup and start watching some local directory use:
    ```
    add <source path> <target path 1> <target path 2> ... <target path n>
    ```

- To stop watching some backup use:
    ```
    end <source path> <target path 1> <target path 2> ... <target path n>
    ```

- To restore source directory we can use (after this command the source dir will be like the targer dir):
    ```
    restore <source path> <target path>
    ```

- To list watched backups:
    ```
    list
    ```

- And to exit use simply:
    ```
    exit
    ```
    or send `SIGTERM` or `SIGINT`
