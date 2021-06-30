## Removed plugins

We have removed the folders xdmcp, nx and st due to the fact that Gtk have deprecated the functions
using the XEmbed protocol.

These folders have been split in a new repository as shown in the following example

```
# Split of the folder plugins/st in a new branch
git subtree split -P plugins/st -b plugins/st
# We add the remote of the new repository
git remote add plugins/st git@gitlab.com:Remmina/remmina-plugins.git
# We push to the new repository
git push plugins/st plugins/st
# We change to the location of the local copy of git@gitlab.com:Remmina/remmina-plugins.git
cd ../remmina-plugins/
# The branch plugins/st does not have the folder plugins, the st source is in the root
# The following subtree command recreate the folder structure (../Remmina is the main repo)
git subtree add -P plugins/st ../Remmina plugins/st
git push
```

The next step is to `git rm -r plugins/st` and if we need again `st`, it's enough to add it as a submodule:

```
git submodule add git@gitlab.com:Remmina/remmina-plugins.git plugins
```

The submodule part has to be tested as it'll cause issues with the existing files
