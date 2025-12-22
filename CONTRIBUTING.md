# Contributing to HamClock C

Thank you for your interest in improving HamClock C! This document explains how to report bugs, request features, and contribute to the project.

---

## Reporting Bugs

### Before Reporting
1. Check existing issues: https://github.com/dpaschal/HamClock/issues
2. Read the documentation: BUILD.md, TESTING.md, GETTING_STARTED.md
3. Try on a clean build: `rm -rf build && mkdir build && cd build && cmake .. && make`

### How to Report (Choose One)

#### Option 1: GitHub Issues (Recommended - Public)
https://github.com/dpaschal/HamClock/issues

Click "New Issue" and include:
```
Title: [BUG] Brief description

Description:
- Platform: (Linux x86_64, Raspberry Pi 4, etc.)
- Compiler: (GCC version, Clang, etc.)
- Build type: (Release, Debug)
- Error message: (full output)
- Steps to reproduce:
  1. ...
  2. ...
- Expected vs. actual behavior
```

#### Option 2: Email (No GitHub Account Required)
Subject: `HamClock Bug Report: [Brief Description]`

Include:
- Platform and version
- Compiler information
- Complete error output
- Steps to reproduce
- Any relevant build logs

#### Option 3: GitHub Discussions
https://github.com/dpaschal/HamClock/discussions

Post in the "Q&A" category for help diagnosing issues.

---

## Requesting Features

### How to Request (Choose One)

#### Option 1: GitHub Discussions (Recommended for Ideas)
https://github.com/dpaschal/HamClock/discussions

Click "New discussion" → Select "Ideas" category:
```
Title: [Feature Idea] Brief description

Description:
- What problem does this solve?
- How would users benefit?
- Example use case
- Any implementation ideas?
```

#### Option 2: GitHub Issues (for Tracked Features)
https://github.com/dpaschal/HamClock/issues

Click "New Issue" and include:
```
Title: [FEATURE] Brief description

Description:
- Motivation: Why is this needed?
- Use case: When would operators use this?
- Implementation sketch: How might it work?
- Alternative approaches: Other ways to solve this?
```

#### Option 3: Email
Subject: `HamClock Feature Request: [Feature Name]`

Include:
- Description of feature
- Why it's needed
- How it benefits ham radio operators
- Any implementation suggestions

---

## Code Contributions

### Before Starting
1. Create an issue first to discuss your changes
2. Fork the repository
3. Create a feature branch: `git checkout -b feature/your-feature`

### Requirements
- Follow the code style (K&R style, see README.md)
- Keep functions under 100 lines
- Test on your platform before submitting
- Run test suite: `cd build-tests && ctest -V`
- No breaking changes to public APIs

### Pull Request Process
1. Create branch from `master`
2. Make changes and test thoroughly
3. Run: `ctest -V` (tests must pass)
4. Commit with clear message
5. Push and create pull request
6. Describe changes and testing done

### Testing
All contributions must pass the test suite:
```bash
cd build
cmake ..
make -j4
ctest -V
```

---

## Public Access & Transparency

✅ **HamClock is completely open:**
- No authentication required to access repository
- All issues and discussions are public
- Anyone can read, clone, and download without account
- Free to fork and modify (MIT License)

GitHub account (free) required only to:
- Create issues/discussions
- Submit pull requests
- Leave comments

---

## Code of Conduct

- Be respectful and constructive
- Focus on technical merit
- Assume good intentions
- Welcome diverse perspectives
- Help each other learn

---

## Questions?

- **General questions:** Post in [GitHub Discussions Q&A](https://github.com/dpaschal/HamClock/discussions)
- **Build problems:** See BUILD.md troubleshooting section
- **Technical details:** Open an issue with [QUESTION] tag

---

## What to Expect

- **Bug reports:** Triaged within 1-2 weeks
- **Feature requests:** Discussed in community
- **Pull requests:** Reviewed for code quality and testing
- **Email submissions:** Best effort response

---

## License

By contributing, you agree that your code will be licensed under the MIT License (same as HamClock C).

---

## Thank You!

Your contributions help make HamClock C better for the entire ham radio community. 🎉

**Ready to contribute?**
- Start with an issue: https://github.com/dpaschal/HamClock/issues
- Fork the repo: https://github.com/dpaschal/HamClock
- See README.md for build instructions
