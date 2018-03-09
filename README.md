== The problem ==
This started as removing the annoying issue with xkb on linux. Issue existed in the year 2007 and still lives, although a patch is included. 
The ignorance of the xkb developers in making language switch with the same behavior like windows, over the "standardizing" some syntax.
More about the issue can be read here: https://bugs.freedesktop.org/show_bug.cgi?id=865

=== Summary of the issue ===
On Windows OSes, switching a keyboard layout uses modifier only keys <Alt>+<Shift> by default. The behavior is as follows:
1. Wait for <Alt> and <Shift> both to be pressed at the same time;
2. Depending on the next event, one of the following happens:
  * If <Alt> or <Shift> is released, keyboard layout changes;
  * If another key is pressed/released, no change occurs (state is "failed");
3. Behavior finishes with:
  * On failed state, wait for both <Alt> and <Shift> are released, the reset state and go to (1);
  * If state is not failed, go back to step (1)

This allows layout to be switched by holding <Alt> and pressing and releasing <Shift> multiple times.
Moreover, this allows keyboard shortcuts <Alt>+<Shift>+Key to be ignored as language change.

The linux way here is nuts. It makes a language change on step 1, before it is known if the key is part of a shortcut. Fixes are terrible.

== The solution ==
Looking at the code of xkb, one can only lose hope. Luckily, linux is still very modifyable system. So, a solution exists without any code modification.
The cost is security concerns, as it will be described in a little bit. I suggest not to install those applications, before examine the code yourself and see with your eyes that no sensitive data is leaked.

The project consists of two applications:

=== alt-shift-notify ===
This application should be started as a service from root user, possibly after ``udev`` and ``local-fs`` or at least ``/sys`` and ``/dev`` filesystem had started.
The application locate all keyboard devices and their corresponding ``/dev/input/eventX`` and start key scanner for each of those.
The scanner read the events occurring in those events and if the event matches (left) <Alt> or (left) <Shift> press or release, it sets corrensponding states.
If the state detected matches the first point of step (2), it sends a signal to all subscribers.

WARNING: Keyboard events are extremely sensitive information. You should not install as root any program that scan keys, unless you know exactly what it does. Check the source.

The subscribers are programs that must be notified when <Alt>+<Shift> have been pressed. The subscribers send SIGUSR1 signal to this application in order to register.
The subscribers could send SIGUSR2 to unsubscribe. Alternatively, if a subscriber process is terminated it is removed from subscribers on the next event.
Since PIDs are reused, the program also checks ``/proc/<subscriber_pid>/stat`` to see the start time of the process. This value should not change during process execution. If it is changed,
then the original subscriber is probably terminated and the PID has been reused by another process. To prevent leak, we remove such process from subscribers.

Any subscribed process will receive SIGUSR1 when it is time to update the layout.

This program creates a file with its own PID at /var/run/alt-shift-notify.pid

== xkb-next-layout ==
This is a simple program to switch the keyboard layout to the next registered group in xkb.
The program look for /var/run/alt-shift-notify.pid to discover the ``alt-shift-notify``. Then sends SIGUSR1 in order to subscribe. On exit it sends SIGUSR2.

This program should be started by the same user that starts X11 server. Note, that this might not be the root user.
In most graphical linux desktops, the main non-root user with ID 1000 is the one that stats X11. X11 is not started by systemd, nor there is any robust way to identify when X11 has started.
That's why this program exists. It can be put on the X11 starting mechanisms, which is usually abstracted into GUI menu somewhere in the settings of the desktop.
The program can accept parameter --fork in order to become a daemon. When specified, a forked process will remain on the background and the process will exit immediately.
This allows the program to run without preventing normal startup process of X11.

To run this program, put the final executable in some directory on the ``PATH``, for example ``/usr/local/bin`` is a good choice. Once there, use the command:
```
sudo setcap cap_kill+ep /usr/local/bin/xkb-next-layout
```

Non-privileged program cannot send signals to privileged processes by default. This is good, because no program started as your user can subscribe to ``alt-shift-notify``.
But we want ``xkb-next-layout`` to be able to subscribe, so the above command allows that particular executable file to send signals to any process in the system.
This is critical security risk, so please make sure that the code of ``xkb-next-layout`` does what is supposed to do. Do not install this program blindly.

This program should only send SIGUSR1 and SIGUSR2 to the process found in ``/var/run/alt-shift-notify.pid``. It should not send any other signals to any other processes.

== FAQ ==
Q: Why two programs?
A: Because reading /dev/input/eventX requires root user, while accessing xkb requires X11. There is no easy way and reliable lay (sleep is not reliable) to make a root program accessing X11 server.
In recovery mode X11 does not run at all. Therefore, the two programs, allow two different users and allows initialization in two different time points.

Q: Why use signals?
A: Because they behave exactly as expected for this case. Unix sockets are bad - the data remains in them if not subscribers, which is useful for most of the cases, but not here.
If no one listens for Alt+Shift the message should be silently ignored. Therefore, a message queue is also not an options. Signals are exactly this - send a notification to someone.
If (s)he did not listen, whatever, ignore it, do not keep it. The message also contains no data. It is just a signal to change the layout.

Q: Why C++?
Because it is a good exercise. Because it is better than C. Because C/C++ is required for the most operations. I suppose a python could process it in the same way. But certainly, no easy way (for me) to do all this in bash.

Q: Isn't it there a solution to this already?
A: Yes, probably. Not for Ubuntu Mate at this time. There are probably several thousands linux distribution. One option is to install them one by one and configure them to check if everything works the way I used to work. Or use ubuntu mate, since I got used to the way I have configured it, but without a second language (or without Alt+Shift+... shortcuts). Maybe, there was easier solution, but if I did not found it, it is not so obvious and not so easy to implement, compared to this one.

Q: I read the security concerns, why so much warnings?
A: I have written that and I know what it does, so from my point of view it is safe to use, although it is accessing too sensitive information. From your point of view, this is some random program of random user on the internet. If you are willing to start as root such a random program, then probably linux is not for you. On the other hand, if you examine the code carefully, you can see that the program does what you expect it to do, and never do something unexpected.
* Reading /dev/input/eventX requires root, because the program sees every key you press in every program, including the keys you press during writing passwords. The ``alt-shift-notify`` ignores such an events and only look for <Alt>+<Shift>. But I (the author of this readme) could lie, the code never lies, so look at the code to see it indeed does exactly that before installing.
* Giving ``cap_kill`` to a executable file, allows that file to send **all** signals to **all** processes. This means a program could send SIGKILL to process with pid_t = 1. SIGKILL is not blocked and kill the process immediately and pid_t = 1 should not be killed while your system is working. Again, look at the code and see what signals ``xkb-next-layout`` is making. People lies, the code does not.
* Always assume all code, not written/examined by you as untrusted. There is a lot of people with bad intentions over the internet.
