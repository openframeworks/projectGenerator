#!/usr/bin/env bash

# master builds upload to the "latest" release. The first build job of a push recreates it at that
# commit, so its date and tag are this build's; jobs that start later (a runner can queue for half
# an hour) see the tag already there and leave it, so nothing already uploaded is deleted.
# Re-running an older master build doesn't touch a newer latest.

REPO="${GITHUB_REPOSITORY:?}"
SHA="${GITHUB_SHA:?}"

head=$(gh api "repos/$REPO/commits/master" --jq '.sha' 2>/dev/null || true)
if [ "$head" != "$SHA" ]; then
	echo "${SHA:0:7} isn't the master head (${head:0:7}) - leaving latest"
	exit 0
fi

current=$(gh api "repos/$REPO/git/ref/tags/latest" --jq '.object.sha' 2>/dev/null || true)
if [ "$current" = "$SHA" ]; then
	echo "latest is already at ${SHA:0:7}"
	exit 0
fi

echo "recreating latest at ${SHA:0:7} (was ${current:0:7})"
gh release delete latest --repo "$REPO" --cleanup-tag --yes || echo "no latest release yet"
# another build job may create it at the same moment
gh release create latest --repo "$REPO" --target "$SHA" --title latest --latest=false \
	--notes "Latest master build from ${SHA:0:7} (copied to nightly every night). Files are added as each platform finishes building." \
	|| gh release view latest --repo "$REPO" > /dev/null
