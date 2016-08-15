=== BUILD PACKAGES ===

You can build ubuntu packages with just 2 steps for Ubuntu series  `yakkety`, `wily`, `xenial` and  `trusty`.

Before run the script `scripts/ubuntu-build.sh` you must have:
 - a valid gpg key associated with your email address
 - an account on launchpad.net where upload packages with the configured public gpg key
 - set the two environment variables `DEBFULLNAME` and `DEBEMAIL` to your full name and your email used in your email address in the form "`<DEBFULLNAME>` <`<DEBEMAIL>`>"

Use the flag `-d` to build a development package; if you want to build an official version manage the `debian/changelog` file by hand.

After that follow what the script tell you to upload packages.

=== UPDATE PATCHES (IF EXISTS) ===

To manage debian patches you must use the `quilt` package, check documentation about it.

