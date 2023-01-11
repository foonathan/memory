#!/bin/sh
# builds documentation and publishes it
# run in root of repository, assumes `git worktree add doc/html gh-pages`

rm -r doc/html/*
doxygen doc/Doxyfile
echo "memory.foonathan.net" > doc/html/CNAME
cd doc/html
git add --all
git commit -am"Update documentation" --amend
git push --force origin gh-pages

