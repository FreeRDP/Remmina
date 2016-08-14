# Requirements

## [github_changelog_generator](https://github.com/skywinder/Github-Changelog-Generator)

You can install using gem

```sh
gem install github_changelog_generator
```

If you want to contribute back to the project you can clone the repository and build it.

## [GitHub token](https://github.com/skywinder/github-changelog-generator/blob/master/README.md#github-token)

You need the token because github doesn't accept more than 50 request per day with it's API.
It's for free...

# Generate the CHANGELOG.md

github_changelog_generator -t 689c2dd09acaebf4ce92fecaeed2ec8140b6bfbf

# Commit the new CHANGELOG.md

The CHANGELOG.md should be committed only after a new release.