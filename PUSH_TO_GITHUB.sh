#!/bin/bash

# HamClock C - GitHub Push Script
# Usage: ./PUSH_TO_GITHUB.sh YOUR_GITHUB_USERNAME

set -e

if [ -z "$1" ]; then
    echo "Usage: ./PUSH_TO_GITHUB.sh YOUR_GITHUB_USERNAME"
    echo ""
    echo "Example: ./PUSH_TO_GITHUB.sh paschal"
    echo ""
    echo "This script will:"
    echo "  1. Add GitHub remote"
    echo "  2. Push all commits"
    echo "  3. Create and push v1.0.0 tag"
    echo ""
    exit 1
fi

USERNAME=$1
REPO="hamclock-c"
REMOTE="https://github.com/${USERNAME}/${REPO}.git"

echo "╔════════════════════════════════════════════════════════════╗"
echo "║     HamClock C v1.0.0 - GitHub Push Script                ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "GitHub Username: $USERNAME"
echo "Repository: $REPO"
echo "Remote URL: $REMOTE"
echo ""

# Verify repository is clean
echo "Checking git status..."
if [ -z "$(git status -s | grep -v '^??')" ]; then
    echo "✅ Working directory clean"
else
    echo "❌ Error: Uncommitted changes found"
    echo "Please commit or stash changes before pushing"
    exit 1
fi

# Check if remote already exists
if git remote get-url origin &>/dev/null 2>&1; then
    echo "⚠️  Remote 'origin' already exists:"
    git remote get-url origin
    echo ""
    read -p "Remove and reconfigure? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        git remote remove origin
        echo "✅ Remote removed"
    else
        echo "Aborting"
        exit 1
    fi
fi

# Add remote
echo ""
echo "Adding GitHub remote..."
git remote add origin "$REMOTE"
echo "✅ Remote added: $REMOTE"

# Verify main branch is 'master'
BRANCH=$(git rev-parse --abbrev-ref HEAD)
echo ""
echo "Current branch: $BRANCH"
if [ "$BRANCH" != "master" ]; then
    echo "Renaming to 'master'..."
    git branch -M master
    echo "✅ Branch renamed to 'master'"
fi

# Push commits
echo ""
echo "Pushing commits to GitHub..."
git push -u origin master
echo "✅ Commits pushed successfully"

# Create and push tag
echo ""
echo "Creating v1.0.0 tag..."
if git rev-parse v1.0.0 &>/dev/null 2>&1; then
    echo "⚠️  Tag v1.0.0 already exists"
    read -p "Overwrite? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        git tag -d v1.0.0
        git push origin :refs/tags/v1.0.0
        git tag -a v1.0.0 -m "HamClock C v1.0.0 - Production Ready with MIT License"
        echo "✅ Tag recreated"
    fi
else
    git tag -a v1.0.0 -m "HamClock C v1.0.0 - Production Ready with MIT License"
    echo "✅ Tag created: v1.0.0"
fi

# Push tag
echo ""
echo "Pushing tag to GitHub..."
git push origin v1.0.0
echo "✅ Tag pushed successfully"

# Summary
echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                    ✅ PUSH COMPLETE!                       ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Repository: https://github.com/${USERNAME}/${REPO}"
echo "Release: https://github.com/${USERNAME}/${REPO}/releases/tag/v1.0.0"
echo ""
echo "Next steps:"
echo "  1. Visit GitHub release page"
echo "  2. Click 'Edit' on the v1.0.0 release"
echo "  3. Copy content from RELEASE.md into description"
echo "  4. Click 'Publish release'"
echo ""
echo "🎉 HamClock C v1.0.0 is now public!"
echo ""
