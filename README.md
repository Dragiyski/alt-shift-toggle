Alt-Shift-Toggle is a fix for the cluncky behavior in language (keyboard layout) switching in linux using `Alt+Shift` combo. Both Alt and Shift are modifier keys and the combo can be used to switch keyboard layouts alongside other keyboard shortcut (for example `Alt+Shift+F` in code editors usully formats the code). To achieve this we use 4 states: `alt_state` and `shift_state` reflects whether `Alt` or `Shift` key is pressed. The `group_state` becomes `True` when both `Alt` and `Shift` keys are pressed at the same time. The `fail_state` prevents language switch if another key is pressed alongside `Alt` and `Shift`. The language switch occurs on **key up**, and not on **key down** when `group_state` is `True` and `fail_state` is `False`. The `fail_state` is reset only when both `Alt` and `Shift` are released.

So, to switch the language, the user must:

1. Press `Alt` or `Shift` key;
2. Press `Shift` (if `Alt` on step 1), or press `Alt` (if `Shift` on step 1);
3. Release either `Alt` or `Shift` key without pressing any other key;
4. To switch again, press the released key in step 3 and go to step 3;

To use keyboard shortcut with `Alt` and `Shift` (for example `Alt+Shift+F`) and switch language after that:
1. Press `Alt` and `Shift`;
2. Press the `F` key to use `Alt+Shift+F`;
3. Release all pressed keys;
4. Go to step 1 in language switch;

Pressing `Alt` and `Shift` before `fail_state` resets will not switch the language. This allow `Alt+Shift` combo to switch the language without interference with keyboard shortcuts.

# Operation

The linux is famously clunky for international users used to Windows-like language switching. This is 20+ years problem in XKB, not considered a bug, since there is no way to encode keyboard shortcuts with fail state. This is an attempt to add this functionality non-intrusively, without modifying the XKB itself (although this problem should be fixed in XKB).

The solution consists of two individual programs communicating through signals `SIGUSR1` and `SIGUSR2`.

## `alt-shift-notify`

This program is notifier that reads keyboard keys from `dev/input/event*` files and sends `SIGUSR1` to any subscribed processes when the above requirements for switching language/keyboard layout is met. The program operates as `systemd` service as root.

In this version, the application is rewritten in python, allowing simpler code. You should examine this code (**as any code meant to run as root**) before install it. **The code reads keyboard keys, so you should not blindly trust programs that do so**.

### Changelog

* Completely rewritten in python.
* Detection of keyboard-related `/dev/input/event*` can happen not only during the initialization, but also when a new input device is plugged-in. This allow language switch to occur from newly plugged USB keyboard without resetting the service.
* Waiting for signal happens in the main thread using `sigwaitinfo` instead of `sigaction`, allowing signals to be observed without interrupting the `read` syscall from the keyboard observing threads.
* Each detected keyboard have its own thread now.

## `xkb_next_layout`

This program perform keyboard layout/group switch whenever it receives `SIGUSR1` from the expected process. This operates as non-root and it is added by the GUI/X startup system. The reason for separate program is:

* X window system has completely different lifetime compared to `systemd`;
* The connection to X window system passes through `.Xauthority` cookie;

Therefore, detecing when X is finally available and connecting to it from `systemd` service is significantly more challenging task compared to having separate program that is started by the X window system itself. As a result, a separate program is easier solution, but it requires the two program to communicate, without exposing the system to security risk. As a result, `alt-shift-notify` notifies only for `Alt+Shift` combo described above, and sends `SIGUSR1` only to subscribed processes and only when language switch must occur. The `xkb_next_layout` can only subscribe as non-root if the executable has `cap_kill` capability (`kill(2)` can be used to send any signal, not just `SIGKILL`).

### Changelog

* The application now uses `inotify` to observe when the `alt-shift-notify` PID file will appear. This allow the `alt-shift-notify` service to be reset without resetting `xkb_next_layout`. Whenever the PID file changes, `SIGUSR1` is send to the new PID. On exit, `SIGUSR2` is send if the current PID is not zero.
* The application now uses `sigwaitinfo` in separate thread, instead of `sigaction` callback, so signals does not interrupt `read()` from `inotify`.

# Installation

## `alt-shift-notify`

1. Install `python3` and `libudev`.
2. Run `python3 install.py <install-dir>` or `./install.py <install-dir>` under root. In case of fatal errors, check if any requirements are missing and install them. Some errors from `wheel` are expected, but do not prevent installing `pyudev`, so you can ignore them.
3. Optional: Modify `${INSTALL_DIR}/alt-shift-notify.service`, if necessary.
4. Run `systemctl enable alt-shift-notify.service`
5. Reboot or run `service alt-shift-notify start`

For more options run `${INSTALL_DIR}/venv/bin/python main.py --help`. This allow switching the PID file location (see `--pid-file`) and print more information in `stdout` (see `--verbose`);

## `xkb_next_layout`

1. Install `cmake`;
2. Create a build directory;
3. Run `cmake <source-directory>`;
4. Run `make`;
5. Run `make install` (might require `sudo`);
6. Add `xkb_next_layout` to the `Startup Application` in your GUI environment;

The installation automatically adds `cap_kill` capability to the installed `xkb_next_layout`;

You can also run `xkb_next_layout` in bash to test and observe its behavior.

### TODO

* Add `--verbose` option and print only when verbose;
