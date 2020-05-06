=== BUILD PACKAGES ===

You can build Ubuntu packages with just 2 steps for Ubuntu series  `yakkety`, `wily`, `xenial` and  `trusty`.

Before running the script `scripts/ubuntu-build.sh`, you must have:
 - A valid GPG key associated with your e-mail address.
 - An account on Launchpad.net where upload packages with the configured public GPG key.
 - Set the two environment variables `DEBFULLNAME` and `DEBEMAIL` to your full name and your e-mail used in your e-mail address in the form of "`<DEBFULLNAME>` <`<DEBEMAIL>`>"

Use the flag `-d` to build a development package; if you want to build an official version, manage the `debian/changelog` file by hand.

After that follow what the script tell you to upload packages.

=== UPDATE PATCHES (IF EXISTS) ===

To manage Debian patches you must use the `quilt` package, check the documentation about it.

