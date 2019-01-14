# Contributing to Remmina

Given the implied openness of the project, contributing to Remmina is extremely simple.
Everything is needed to contribute is well known and available to be used, and, most important, we are a kind, openminded, simple community.

In this page you should find everything you need to know if you want to contribute, if it misses anything let us know otr try to fix it yourself.

## Writing Code

**Remmina** uses the popular [Fork and Pull](http://stackoverflow.com/questions/11582995/what-is-the-fork-pull-model-in-github) model when it comes to contributing.

If you&#8217;d like to make changes to this project then the following basic steps will get you there!

  1. Fork [this repo](https://gitlab.com/Remmina/Remmina/forks/new)
  2. Make your changes
  3. [Submit a merge request](https://gitlab.com/Remmina/Remmina/merge_requests/new)

See our [wiki](https://gitlab.com/Remmina/Remmina/wikis/home) to know how to compile Remmina and FreeRDP.

In general look at the issues labeled [&#8220;help wanted&#8221;](https://gitlab.com/Remmina/Remmina/issues?label_name%5B%5D=help+wanted), it&#8217;s probaly the best place where to start. If you would like to earn some money there are some bounties available, see below for more details.

### Fix existing bugs

This, in general, should be the most important task.

At the moment there are not critical bugs to be fixed, but several annoyances and enhancements that we&#8217;d like to fix.

To see all the issues already labeled as bug, you can use [this query](https://gitlab.com/Remmina/Remmina/issues?label_name%5B%5D=bug)

Some exemples are:

  * [#146](https://gitlab.com/Remmina/Remmina/issues/146) : XFCE intercepts shortcuts no matter of &#8220;Grab all keyboard events&#8221; button
  * [#190](https://gitlab.com/Remmina/Remmina/issues/190) : Modifier keys (alt) are wrongly mapped across a Ubuntu -> Mac remote session
  * [#380](https://gitlab.com/Remmina/Remmina/issues/380) : Segfault on apparently invalid pixel data bug unconfirmed
  * [#246](https://gitlab.com/Remmina/Remmina/issues/246) : Clipboard Sync from Mac OS X to Linux over VNC Not Working

### Implement most wanted features and fixes (_bounty $305_)

Similarly, we have a quite important list of feature requests, that are labeled as [enhancement](https://gitlab.com/Remmina/Remmina/issues?label_name%5B%5D=enhancement&sort=created_date&state=opened), of which some that have a [bounty](https://gitlab.com/Remmina/Remmina/issues?scope=all&utf8=%E2%9C%93&state=opened&label_name[]=enhancement&label_name[]=bounty) for those that will be able to implement them.

  * ~~ [#6](https://gitlab.com/Remmina/Remmina/issues/6) : New resolution setting &#8220;fit to window&#8221; [$250] ~~ Implemented by @giox069
  * [#323](https://gitlab.com/Remmina/Remmina/issues/323) : File transfer in RDP plugin [$25]
  * [#476](https://gitlab.com/Remmina/Remmina/issues/476) : Favorite and recent connections [$5]
  * [#815](https://gitlab.com/Remmina/Remmina/issues/815) : Feature request &#8211; Option &#8211; Floating Desktop Name [$5]
  * [#376](https://gitlab.com/Remmina/Remmina/issues/376) : RDP: Multi monitor support [$15]

### Internal projects

  * Refactorying [remmina\_connection\_window.c](https://gitlab.com/Remmina/Remmina/blob/next/remmina/src/remmina_connection_window.c) as it is big, complicated and hard to extend.
  * Simplify the user interface.
  * Separate plugin processes from the main process to improve stability.
  * Write an additional plugin system to write plugins also in Python and eventually Ruby.

## Report bugs, ideas, issues

Install it, use it and report back to us. Whatever you find that it doesn&#8217;t work, it&#8217;s missing, it&#8217;s ugly don&#8217;t hesitate to let us know.

For bug and feature requests use [GitLab issues](https://gitlab.com/Remmina/Remmina/issues) For discussions you can use G+, reddit ad irc (we are not often connected, be patient)

## Translating

You can help translate Remmina, some basic instructions to get started are documented in the [How-to-translate-Remmina Wiki](https://gitlab.com/Remmina/Remmina/wikis/How-to-translate-Remmina)

## Documenting

We need much more user and developer guides. You can submit any (accessible) format you want for the user guides (screencasts, pdf, html, Open Document, gs, ps, LaTeX, gitlab wiki , etc).

For the developers the same and we need to document much more the source code.

### Screenshots

Send us your nice embodied desktops.

## Donating

If you rather prefer to contribute to Remmina with money you are more than welcome.

For more informations See the [Remmina web site donation page](https://remmina.org/donations/).

See the [THANKS.md](THANKS.md) file for an exhaustive list of supporters.

