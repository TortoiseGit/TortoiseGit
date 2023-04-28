#!/bin/bash
set -eu -o pipefail

founderror=false
function error() {
  echo -en "\e[1;31m";
  echo "$*"
  echo -en "\e[0m";
  founderror=true
}

git fetch $CI_REPOSITORY_URL --shallow-exclude=refs/heads/$CI_MERGE_REQUEST_TARGET_BRANCH_NAME refs/pipelines/$CI_PIPELINE_ID
git fetch $CI_REPOSITORY_URL --depth=1 $CI_MERGE_REQUEST_TARGET_BRANCH_NAME

git rev-list --reverse FETCH_HEAD..$CI_COMMIT_SHA | ( while read commit_hash
do
  echo "Checking commit $(git show --no-patch --pretty='%h ("%s")' $commit_hash)"
  git show --no-patch --format=%B $commit_hash | grep -q "Signed-off-by:" || error "Commit $commit_hash does not contain a Signed-off-by line! Please see <https://gitlab.com/tortoisegit/tortoisegit/blob/master/doc/HowToContribute.txt>"
done
if [[ $founderror != false ]]; then
  exit 1;
fi
) || founderror=true

set +u

if [[ $founderror != false ]]; then
  exit 1;
fi
