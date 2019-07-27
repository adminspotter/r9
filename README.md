# r9 #

[![Build Status](https://travis-ci.org/adminspotter/r9.svg?branch=master)](https://travis-ci.org/adminspotter/r9) [![Code Climate](https://codeclimate.com/github/adminspotter/r9/badges/gpa.svg)](https://codeclimate.com/github/adminspotter/r9) [![codecov](https://codecov.io/gh/adminspotter/r9/branch/master/graph/badge.svg)](https://codecov.io/gh/adminspotter/r9)

Revision IX (revision9, r9, etc.) is a client-server game engine in
C++.

We've always wanted a highly modifiable server platform which
supported almost all of its game-related functionality as pluggable
modules.  r9 is intended to be as much a simulator of 3d space as any
specific game; it could be that r9 becomes more valuable as a platform
for trying things out in a physical environment.  Rocketry?
Ballistics?  Sure, why not.  Fool with the gravitational constant?
Absolutely.

The game that *we* have always wanted is an MMO, mostly based in
space, which allows you to fly a ship to wherever you want to go, and
get out of the ship and walk around.  You can land on planets, do
spacewalks, mine asteroids, dock at space stations and walk around
inside them.  You can upgrade your ship, or craft new objects, or
build entire new vehicles.

One interesting idea that we had was to allow other servers to take
over parts of space.  For example, somebody could decide they wanted
to make a given planet their colony, and provide their own server for
it.  Players would be handed off seamlessly between the two servers.
Each server could have its own set of skills and abilities; a
swords-and-sorcery planet could have legitimate magic, but other
servers may not want to support those abilities.

We wanted to be able to script logic for autonomous objects in an
embedded scripting language.  NPCs could have sophisticated logic,
while missiles would be pretty simple.  We also have plans to be able
to script game behaviour (like how does a given action work on the
server-side) in those other embedded languages.

## Features ##

* Network agnosticism (IPv4, IPv6, datagram, stream, whatever!)
* Encrypted wire protocol
* Pluggable server modules for client actions
* Support for backend MySQL and PostgreSQL databases
* Pluggable embedded language support
  * Lua
  * Python
  * Tcl

### Under active development ###

### To come ###

We've got a bunch of issues on our
[backlog](https://github.com/adminspotter/r9/issues) that we would
love to collaborate on.

* Pluggable physics libraries (change the gravitational constant!)
* Support for more embedded languages
  * Ruby
  * Common Lisp
  * ???
* Beautiful, full-motion, customizable character and vehicle models
* Seamless handoff of clients between neighboring servers
* Server consoles to allow on-the-fly configuration changes

## Platforms ##

r9 has been built and tested on:

* CentOS Linux 6/7
* Debian GNU/Linux Stretch
* Ubuntu GNU/Linux Trusty Tahr and Xenial Xerus (via [Travis CI](https://travis-ci.org))
* MacOS X Mavericks - Mojave

As long as your platform contains the typical POSIX libraries, you
should at least be in the ballpark.  Most other Linuxes should be
fine, and SysV and BSD platforms will probably be close.  Testing on
other platforms is always welcome, and patches should try to confine
any platform-specific stuff to its own files.

## Build dependencies ##

* A C++11 compiler
* OpenGL 3.2 support
* The GNU autotools
* [glm](http://glm.g-truc.net/)
* [GLFW](http://www.glfw.org/) 3.2

## Contributing ##

We love contributions!  Feel free to submit a pull request, or start
some discussion in an issue.

We love 80 columns.  No, we *really* love 80 columns.  If you
absolutely can't avoid long lines, well, we'll deal with that when it
pops up.  Just make sure your code is clean, small, and easily
understandable.  We're not fans of marathon functions, or giant
commits either; if you can keep both at or under 50 lines, we'll get
along just fine.

Pull requests don't need to be squashed.  In fact, we'd prefer that
they **not** be squashed.  Some people get twitchy about that, but as
long as the commit chain makes sense, is documented well by the commit
messages (no "fixed stuff"), and follows a good logical flow, they'll
be perfectly acceptable.  Tag relevant commits or issues in your
commit messages.  Add your name to the AUTHORS file and the comment
block at the top of the files you change.  It's not amazingly
important to update the 'last updated' lines at the tops of files, but
it would be nice. :)

Code is licensed GPL2.
