Alt-Shift-Toggle is a set of two programs for switching keyboard layout in linux in a similar manner to Windows operating system Alt+Shift combination. The operation is as follows:

1. Press (keydown) Alt or Shift;
2. Press (keydown) Alt (if Shift on first step), or Shift (if Alt on first step);
3. Release (keyup) either Alt or Shift to make a keyboard layout change (note: language switch occurs on keyup);
4. You can re-press (keydown) the key release in step 3 to make a switch again;

What's the difference with standard Alt+Shift in Linux. Step 3 occurs on keyup event. Alternative at step 3 is:

3b. Press any other non-modifier key while holding Alt+Shift;

If step `3b` occurs, language is not switched and switching language is canceled until both Alt and Shift keys are released. This allows programs using keyboard shortcuts like (Alt+Shift+F) do not interfere with a keyboard language switch.

# The problem
This started as removing the annoying issue with xkb on linux. Issue existed in the year 2007 and still lives, although a patch is included. 
The ignorance of the xkb developers in making language switch with the same behavior like windows, over the "standardizing" some syntax.
More about the issue can be read here: https://bugs.freedesktop.org/show_bug.cgi?id=865

# This solution

This solution is made out of two separate programs intended to communicate each other. A summary of what the programs do:

## ``alt-shift-notify``

This is a program that does the following:

1. Using ``udev`` to find which of the ``/dev/input?`` is a keyboard;
2. Listen for keyboard events and signals;
3. Collect the `PID` of all processes that registeres (sending `SIGUSR1`) to the process or unregister (sending `SIGUSR2`).
4. When a keyboard mush be switched send `SIGUSR1` to all registered processes. The program do not expose the keyboard state to registered processes, it only reports "switch keyboard layout now" event;

This program is installed as `systemd` service and creates a PID file in ``/var/pid``.

## ``xkb-next-layout``

This program finds the PID file of the ``alt-shift-notify`` and register itself (sending `SIGUSR1`). When the program receives `SIGUSR1` signal, it finds the next input group (keyboard layout) and switch it using ``XkbLockGroup`` from `xkb` library.

# More info

Why this is performed by two programs? Unfortunately, what ``alt-shift-notify`` does can be done only by the root user (it would be security issue if any other user can do that). Accessing raw state of the keyboard can be used for keyboard loggers used to steal passwords. Therefore it is strong recommendation to examine the code of this program before install it (don't just trust random github user).

But `root` user can access everything, why `xkb-next-layout` program is needed? The answer is timing... The services started by `systemd` starts way before a program can connect to the X server (in fact it starts before X server is started, but even using systemd dependencies, X server takes time before it can initialize connections). As a result any attempt by the root user will fail. Moreover, even if X connection is made on demand, it is likely X is started on login using the user's `.Xauthority` file. If a single program is used, the root user must also include `.Xauthority` which is both unusual configuration for a user machine and requires a more elaborate installation process. If two programs are used, the `xkb-next-layout` can be installed as a startup program on user level, while `alt-shift-notify` can be installed and started earlier on system initialization. Both of these are really easy and semi-portable between multiple X environment.

# Installation

Both programs are build and installed using `cmake`.

To install `alt-shift-notify` do:

0. Install dependencies: `libudev-dev` or equivalent package. Dev Packages for `pthreads` and `pkg-config` package are also be required.
1. Run `cmake` in a chosen build directory.
2. Run `make` and `sudo make install` to install the binary.
3. Copy `alt-shift-notify.service` from the source directory to `/etc/systemd/system` (assuming your main user is with UID 1000) (if not, open and modify it).
4. Run `systemctl enable alt-shift-notify`

To install `xkb-next-layout` do:

0. Install dependencies: `libxkb-dev` and/or `libx11-dev`.
1. Run `cmake` in a chosed build directory.
2. Run `make` and `sudo make install`. *
3. Add `<prefix>/xkb_next_layout` to startup programs of your desktop environment.

* This should add ``setcap cap_kill=ep <prefix>/xkb_next_layout`` capability to the program. Without this capability, signals cannot be send to unrelated (non-parent/non-child) processes. This program will only send `SIGUSR1` AND `SIGUSR2` only to the `alt-shift-notify` process (never broadcasted), so it should not affect any other programs. However, it is good idea to check the source before use it, since sending signals can be used in malicious way (a program with `cap_kill` can send `SIGKILL` to ANY program, so do not trust blindly, check if only `SIGUSR1` and `SIGUSR2` are send only to the intended program).

# Conclusion
This problem should not be so difficult to solve, but unfortunately, the only people that suffer from this problem are those that use non-latin keyboard. However, both arabic and kanji-based languages seems to have separate solution, so switching keyboard layout for non-latin alphabets like cyrillic seems to be a problem affecting minority of linux users. On the top of that, a user can easily adjust to non-modifier-only keyboard layout switching (like Win+Space), so this problem will affect a minority the above minority that refuses to switch from modifiers-only keyboard layout switching introduced in Windows operating system.

Even with the above problem, the default switcher (Alt+Shift) works most of the time, with the following caveats:

* Some implementations switch keyboard layout on keydown of Alt or Shift when combination occurs. In this case, only users using keyboard shortcuts like `Alt+Shift+F` would experience any negative consequences;
* Some implementations switch keyboard layout on keyup, but never send a key event to currently focused window for Alt+Shift (again a problem with shortcuts like `Alt+Shift+E`;
* Some implementations allow keyboard shortcuts like `Alt+Shift+C`, but switch the keyboard layout anyway on keyup;

The `alt-shift-toggle` service operates in 3 states and 2 flags:

1. Idle - Either `alt` or `shift` flag is `false` (or both).
2. StandBy - Both `alt` and `shift` flags are `true`. If `keyup` event on `alt` or `shift` appears a `SIGUSR1` is sent to switch keyboard layout.
3. Reset - This state occurs if `keydown` on any key occurs while in StandBy state (both `alt` and `shift` flags are true). In this case no signal is sent (no keyboard layout switch occurs). The program remains in this state until both `alt` and `shift` flags becomes `false`, in which case it returns to Idle.

The problem is `xkb` cannot have something like state 3 (which is essential for having both keyboard layout switch and keyboard shortcuts). The `xkb` keyboard shortcuts are given by special syntax file that is often abstracted to user interface for setting up shortcuts for different events (like "Show Desktop" shortcut, or "Maximize Window" shortcut). If state 3 is not supported then:

* If a keyboard layout switch occurs on keydown, it will occur before the user finishes a keyboard shortcut involving both `Alt` and `Shift` keys.
* If a keyboard layout switch occurs on keyup, it will occur after the user finishes a keyboard shortcut involving both `Alt` and `Shift` keys.
* If a keyboard layout switch occurs on keydown and the keyboard state is reset on switch the program won't receive the keyboard shortcut.

State 3 (Reset) allows us to disable keyboard layout switch if the key combination is a keyboard shortcut and Reset it only after all relevant modifier keys are released. This allows keyboard layout switch to not interfere with keyboard shortcut usage even when a series of consecutive keyboard shortcuts are pressed. However, when the user have clear intention to switch the keyboard layout, by pressing `Alt`, then pressing `Shift` and then releasing `Shift` without pressing any other key, a keyboard switch can occur. Since modifier keys are almost never used on they own, this will not interfere with any currently focused program. Some games have bound function to modifier keys, but usually on `Shift` and `Ctrl`, not `Alt` due to its unnatural position. As a result the above states almost never interferes with any existing program behavior.

Since state 3 uses flag logic, it is too advanced to describe declarative as a key combination in `xkb`. It is unlikely `libxkb` would ever be fixed to allow such behavior (that would require bash like syntax for keyboard shortcuts, which will introduce huge amount of problems), nor such a solution will be provided in `wayland`. The only way is the logic for modifier-only keyboard layout switch to be integrated in the `xkb` library so it can operate the same way as the Windows operating system desktop environment, however this will break modularity (as the logic must include keyboard layout switch in the key event processing) and it is unlikely `xkb` devs would ever consider such fix.

Therefore, third-party programs like this would remain the only solution for achieving the incredible complexity and convenience that Windows operating system introduced when converted the simple act of pressing two keys to extremely complicated endeavour of handling both keyboard layout switch and any keyboard shortcut that a program can bind.
