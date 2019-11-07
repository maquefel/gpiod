# LINUX GPIO DAEMON (GPIOD)

Daemon to monitor gpio via sysfs or uapi interface and send them over network or
execute user defined hooks with desired arguments.

[![Build Status](https://semaphoreci.com/api/v1/maquefel/gpiod/branches/master/badge.svg)](https://semaphoreci.com/maquefel/gpiod)
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fmaquefel%2Fgpiod.svg?type=shield)](https://app.fossa.io/projects/git%2Bgithub.com%2Fmaquefel%2Fgpiod?ref=badge_shield)

## Requirements

- libconfuse (https://github.com/martinh/libconfuse.git)

## Explanatory note

### Concept

**gpiod** supports both uapi and sysfs interface and currently (although this
is not recommended) it can be used both at the same time. Triggers can be set to
rising edge, falling edge, both or be polled via configured interval.

On being triggered an event is dispatched to all connected clients.

**gpiod** fully acts as a daemon, through it needs privileges to be launched
as gpio's are not considered user-space.

## Installation

### Compiling

For version that supports both sysfs and uapi simply invoke:

```
$ make
```

For sysfs only version provide flag:

```
$ make WITHOUT_GPIOD_UAPI=1
```

### Cross-Compiling

```
$ make CROSS_COMPILE=${TARGET_CROSS}-
# for example:
$ make CROSS_COMPILE=arm-none-linux-gnueabi-
```

### Installing

```
$ make DESTDIR= prefix= install
```
Both **DESTDIR** and **prefix** should be used when installing to an extern rootfs.

By default /etc/gpio.d and /etc/gpiod.conf are not created - you can find different
examples in etc directory of repository.

### Tests

Some sanity tests are provided in tests. bats (https://github.com/sstephenson/bats)
is additionally required to perform tests.

## Running

Following options can be passed:

```
-v, --version              prints version and exits
-V, --verbose              be more verbose
-l, --log-level            set log level[default=LOG_INFO]
-n                         don't fork off as daemon
-k, --kill                 kill old daemon instance in case if present
-H, --hup                  send daemon signal to reload configuration
-p, --pid                  path to pid file[default=/var/run/gpiod.pid]
-c, --config=FILE          configuration file[default=/etc/gpiod/gpiod.conf]
-d, --tabdir=DIR           tabs location directory[default=/etc/gpiod.d/]
-h, --help                 prints this message
```

### Configuration file

```
# (optional) listen on specific address, can be overriden by cmd args [default=127.0.0.1]
listen = 127.0.0.1
# (optional) listen on port, can be overriden by cmd args [default=1500]
port = 1500
# (optional) poll interval in msec, can be overriden by cmd args [default = 50]
poll = 50

gpio {
  # facility to use for this gpio sysfs or uapi [default = uapi]
  facility = uapi

  # (optional) calculated system gpio offset [nodefault]
  system = 256

  # (optional) gpiochip name [nodefault]
  gpiochip = gpiochip0

  # (optional) local offset in gpiochip, can be negative [nodefault]
  offset = 0

  # (optional) local mapping [default=system]
  local = 0

  # label for passing via libpack i.e. WD0, WD1, ..., WDN
  label = WD0

  # rising, falling, both, polled [default=both]
  edge = both
}
```

### Hook tabs

**gpiod** supports launching user defined executables based on configured trigger
files placed under **/etc/gpio.d** or specified by **--tabdir**,**-d** argument.

Tab files should be in form of :

```
[LABEL] [MODIFIERS]         [PATH]       [ARGS]
WD0     EDGE_RISING         /tmp/test.sh -n --version -v 23 -s test $@ $% $&
```

Where **LABEL** is pin label defined in config file i.e. **label =**, **MODIFIERS**
one of **EDGE_RISING**, **EDGE_FALLING**, **EDGE_BOTH** and (optional) **NO_LOOP**,
**ONESHOT** flags.

**NO_LOOP** prevents launching before previous launch has finished and **ONESHOT**
to launch hook only once per daemon lifetime.

see etc/gpio.d/testtab example.

## Copyright
© 2019 Nikita Shubin


## License
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fmaquefel%2Fgpiod.svg?type=large)](https://app.fossa.io/projects/git%2Bgithub.com%2Fmaquefel%2Fgpiod?ref=badge_large)