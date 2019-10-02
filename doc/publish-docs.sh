#!/bin/sh
# builds documentation and publishes it
# run in root of repository, assumes `git worktree add doc/html gh-pages`

doxygen doc/Doxyfile
cd doc/html
git add --all
git commit -am"Update documentation"
git push --force origin gh-pages

