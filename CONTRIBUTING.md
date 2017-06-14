# Contributing to Remmina

Given the implied openness of the project, contributing to Remmina is extremely simple.
Everything is needed to contribute is well known and available to be used, and, most important, we are a kind, openminded, simple community.

In this page you should find everything you need to know if you want to contribute, if it misses anything let us know otr try to fix it yourself.

## Writing Code

`Remmina` uses the popular [Fork and Pull](http://stackoverflow.com/questions/11582995/what-is-the-fork-pull-model-in-github) model when it comes to contributing.

If you'd like to make changes to this project then the following basic steps will get you there!

1. Fork [this repo](https://github.com/FreeRDP/Remmina#fork-destination-box)
2. Make your changes
3. [Open a pull request](https://github.com/FreeRDP/Remmina/compare)

See our [wiki](https://github.com/FreeRDP/Remmina/wiki) to know how to compile Remmina and FreeRDP.

In general look at the issues labeled ["help wanted"](https://github.com/FreeRDP/Remmina/issues?utf8=✓&q=is%3Aopen%20sort%3Acreated-asc%20label%3A"help%20wanted"), it's probaly the best place where to start.
If you would like to earn some money there are some bounties available, see below for more details.

### Fix existing bugs

This, in general, should be the most important task.

At the moment there are not critical bugs to be fixed, but several annoyances and enhancements that we'd like to fix.

To see all the issues already labeled as bug, you can use [this query](https://github.com/FreeRDP/Remmina/issues?milestone=none&state=open&sort=created&labels=bug&direction=asc)

Some exemples are:

- [#146](https://github.com/FreeRDP/Remmina/issues/146) : XFCE intercepts shortcuts no matter of "Grab all keyboard events" button
- [#190](https://github.com/FreeRDP/Remmina/issues/190) : Modifier keys (alt) are wrongly mapped across a Ubuntu -> Mac remote session
- [#380](https://github.com/FreeRDP/Remmina/issues/380) : Segfault on apparently invalid pixel data  bug   unconfirmed
- [#246](https://github.com/FreeRDP/Remmina/issues/246) : Clipboard Sync from Mac OS X to Linux over VNC Not Working

### Implement most wanted features and fixes (_bounty $325_)

Similarly, we have a quite important list of feature requests, that are labeled as [enhancement](https://github.com/FreeRDP/Remmina/issues?milestone=none&state=open&sort=created&labels=enhancement&direction=asc),
of which some that have a [bounty](https://github.com/FreeRDP/Remmina/issues?milestone=none&state=open&sort=created&labels=enhancement%2Cbounty&direction=asc) for those that will be able to implement them.

- [#6](https://github.com/FreeRDP/Remmina/issues/6) : New resolution setting "fit to window" [$250]
- [#323](https://github.com/FreeRDP/Remmina/issues/323) : File transfer in RDP plugin [$25]
- [#376](https://github.com/FreeRDP/Remmina/issues/3376) : RDP: Multi monitor support [$15]
- [#476](https://github.com/FreeRDP/Remmina/issues/476) : Favorite and recent connections [$5]
- [#815](https://github.com/FreeRDP/Remmina/issues/815) : Feature request - Option - Floating Desktop Name [$5]
- [#1195](https://github.com/FreeRDP/Remmina/issues/1195) : support for CredSSP [$10]

### Internal projects

- Refactorying [remmina_connection_window.c](https://github.com/FreeRDP/Remmina/blob/next/remmina/src/remmina_connection_window.c) as it is big, complicated and hard to extend.
- Simplify the user interface.
- Separate plugin processes from the main process to improve stability.
- Write an additional plugin system to write plugins also in Python and eventually Ruby.

## Report bugs, ideas, issues

Install it, use it and report back to us.
Whatever you find that it doesn't work, it's missing, it's ugly don't hesitate to let us know.

For bug and feature requests use [GitHub issues](https://github.com/FreeRDP/Remmina/issues)
For discussions you can use G+, reddit ad irc (we are not often connected, be patient)

## Documenting

We need much more user and developer guides.
You can submit any (accessible) format you want for the user guides (screencasts, pdf, html, Open Document, gs, ps, LaTeX, github wiki , etc).

For the developers the same and we need to document much more the source code.

### Screenshots

Send us your nice embodied desktops.

## Donating

If you rather prefer to contribute to Remmina with money you are more than welcome.

For more informations See the [Remmina web site donation page](http://remmina.org/wp/donations).

See the [THANKS.md](https://raw.githubusercontent.com/FreeRDP/Remmina/next/THANKS.md) file for an exhaustive list of supporters.

#### Paypal

[![paypal](https://www.paypalobjects.com/en_US/CH/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=ZBD87JG52PTZC)

#### Bitcoin

[![bitcoin](http://www.remmina.org/wp/wp-content/uploads/2016/06/bitcoin_1298H2vaxcbDQRuR-e1465504491655.png)](bitcoin:1298H2vaxcbDQRuRYkDjfFbvGEgxE1CNjk?label=Remmina%20Donation)

If clicking on the line above does not work, use this payment info:

- Remmina bitcoin address:  1298H2vaxcbDQRuRYkDjfFbvGEgxE1CNjk
- Message: Remmina Donation
