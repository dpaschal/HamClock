# GitHub Wiki Access & Guide

**Project:** HamClock Rust Rewrite
**Date:** 2025-12-19

---

## How to Access the GitHub Wiki

### Method 1: From GitHub Website

1. **Navigate to HamClock Repository**
   - Go to: `https://github.com/[username]/HamClock`
   - (Replace [username] with actual repository owner)

2. **Click "Wiki" Tab**
   - At the top of the repository, click the **"Wiki"** tab
   - Located between "Discussions" and "Security" tabs

3. **View Wiki Pages**
   - All wiki pages will be listed in the sidebar
   - Click any page to view its content

### Method 2: Direct URL Access

```
https://github.com/[username]/HamClock/wiki
```

Replace `[username]` with the actual repository owner.

### Method 3: From Your Local Repository

```bash
# Clone the wiki separately
git clone https://github.com/[username]/HamClock.wiki.git

# View markdown files
cd HamClock.wiki
ls -la
cat Phase8-Alert-System.md
```

---

## Creating Wiki Pages for Phase 8

If the wiki is set up, you should create these pages:

### 1. **Phase 8: Alert System** (Home Page)
- Overview of all 8 features
- Quick links to other pages
- Configuration examples

### 2. **Features: DX Band Monitoring**
- How to configure watched bands
- Alert message format
- Example configuration

### 3. **Features: Satellite Alerts**
- Elevation threshold setup
- Watched satellite list
- ETA calculation

### 4. **Features: Space Weather**
- Kp spike detection
- X-ray flare classification
- Aurora forecasting

### 5. **Features: CME Detection** (NEW)
- Flux/AP tracking
- Threshold explanation
- Time-critical alerts

### 6. **Features: Audio Alerts** (NEW)
- Cross-platform support
- Audio pattern reference
- Configuration options

### 7. **Features: Alert Acknowledgment** (NEW)
- Keyboard shortcuts
- State management
- User dismissal workflow

### 8. **Deployment Guide**
- Installation steps
- Configuration procedures
- Troubleshooting

### 9. **Configuration Reference**
- All AlertConfig parameters
- Default values
- Platform-specific notes

### 10. **Testing & Verification**
- Test procedures
- Platform tests
- Performance benchmarks

---

## Setting Up the Wiki

### If Wiki Doesn't Exist Yet:

1. **Enable Wiki in Repository Settings**
   - Go to Settings → General
   - Scroll to "Features"
   - Check "Wikis" if not already enabled

2. **Create Home Page**
   - Click "Create the first page" or "New page"
   - Use `Home` as the page name
   - Add overview content

3. **Create Additional Pages**
   - Click "New page" for each topic
   - Use descriptive names (no spaces)
   - Example: `Phase-8-Features`

### GitHub Wiki Markdown Format

```markdown
# Page Title

## Section Heading

### Subsection

- Bullet point
- Another point

**Bold text**
*Italic text*

[Link Text](URL)

### Code Block
\`\`\`rust
fn example() {
    println!("Hello");
}
\`\`\`

| Column 1 | Column 2 |
|----------|----------|
| Data 1   | Data 2   |
```

---

## Importing Phase 8 Documentation to Wiki

### Step 1: Convert Markdown Files

```bash
# Copy Phase 8 documentation to wiki
cp /tmp/HamClock/PHASE8-*.md /path/to/HamClock.wiki/

# Rename for wiki format (remove dates/extensions)
mv PHASE8-README.md Home.md
mv PHASE8-QUICK-REFERENCE.md Features-Quick-Reference.md
mv PHASE8-FULL-FEATURE-IMPLEMENTATION.md Complete-Feature-Specifications.md
mv PHASE8-DEPLOYMENT-GUIDE.md Deployment-Guide.md
```

### Step 2: Create Sidebar Navigation

Create `_Sidebar.md` in wiki root:

```markdown
# HamClock Wiki

## Overview
- [Home](Home)
- [Phase 8: Alert System](Phase-8-Overview)

## Features
- [DX Band Monitoring](Features-DX-Band-Monitoring)
- [Satellite Alerts](Features-Satellite-Alerts)
- [Space Weather Alerts](Features-Space-Weather)
- [CME Detection](Features-CME-Detection)
- [Audio Alerts](Features-Audio-Alerts)
- [Alert Acknowledgment](Features-Alert-Acknowledgment)

## Guides
- [Quick Reference](Features-Quick-Reference)
- [Complete Specifications](Complete-Feature-Specifications)
- [Deployment Guide](Deployment-Guide)
- [Testing Guide](Testing-Guide)

## Configuration
- [AlertConfig Reference](Configuration-Reference)
- [Platform-Specific Setup](Platform-Setup)

## Troubleshooting
- [Common Issues](Troubleshooting)
- [Performance Tuning](Performance-Tuning)
```

### Step 3: Create Footer Navigation

Create `_Footer.md`:

```markdown
---

**Phase 8 Alert System** | Version 1.0.0 | [GitHub](https://github.com/[username]/HamClock) | [Issues](https://github.com/[username]/HamClock/issues)
```

---

## Wiki Content Structure Recommendation

### For Maximum Usability:

```
HamClock.wiki/
├── Home.md                          # Main entry point
├── _Sidebar.md                      # Navigation
├── _Footer.md                       # Footer links
│
├── Phase-8/
│   ├── Overview.md                  # Phase 8 summary
│   ├── Features.md                  # All 8 features
│   ├── Configuration.md             # Full AlertConfig docs
│   ├── Deployment.md                # Deploy procedures
│   ├── Testing.md                   # Test guide
│   └── Troubleshooting.md          # Common issues
│
├── Features/
│   ├── DX-Band-Monitoring.md
│   ├── Satellite-Alerts.md
│   ├── Space-Weather.md
│   ├── CME-Detection.md
│   ├── Audio-Alerts.md
│   └── Alert-Acknowledgment.md
│
└── Administration/
    ├── Installation.md
    ├── Development.md
    └── Contributing.md
```

---

## Linking to Documentation

### In Markdown:

```markdown
# Internal Links
[See Features](Features)
[Deployment Guide](Deployment-Guide)

# External Links
[GitHub Repository](https://github.com/[username]/HamClock)
```

### Between Pages:

```markdown
See also:
- [Audio Alerts](Features-Audio-Alerts)
- [CME Detection](Features-CME-Detection)
- [Configuration Reference](Configuration-Reference)
```

---

## Embedding Code Examples

```markdown
# Configuration Example

Here's how to enable CME detection:

\`\`\`toml
[alert_config]
cme_alerts_enabled = true
audio_alerts_enabled = true
alert_duration_seconds = 30
\`\`\`

## Related Files

- `src/data/alerts.rs` - Alert detection logic
- `src/audio/alerts.rs` - Audio system
- `config.rs` - Configuration structure
```

---

## Collaboration & Updates

### Adding Contributors:

1. Go to Repository Settings
2. Manage Access → Collaborators
3. Add wiki collaborators
4. They can edit wiki pages directly

### Wiki Updates:

- Changes are automatically saved
- No pull request needed for wiki edits
- Edit history is maintained
- Revert changes if needed

### Best Practices:

- Keep pages focused and concise
- Use consistent formatting
- Include code examples
- Add internal links between related pages
- Keep table of contents updated
- Review for typos before publishing

---

## Sample Wiki Homepage (Home.md)

```markdown
# HamClock Rust Rewrite

Welcome to the HamClock project wiki. This project implements a high-performance
radio clock with ham radio features in Rust.

## Latest Phase: Phase 8 - Alert System

Phase 8 adds comprehensive alert notifications for:
- DX spot monitoring
- Satellite pass tracking
- Space weather alerts
- CME detection
- Audio notifications
- User-dismissible alerts

[Learn more about Phase 8](Phase-8-Overview)

## Quick Start

1. [Configure alerts](Configuration-Reference)
2. [Deploy HamClock](Deployment-Guide)
3. [Test features](Testing-Guide)

## Features

- [DX Band Monitoring](Features-DX-Band-Monitoring)
- [Satellite Alerts](Features-Satellite-Alerts)
- [Space Weather Alerts](Features-Space-Weather)
- [CME Detection](Features-CME-Detection)
- [Audio Alerts](Features-Audio-Alerts)
- [Alert Acknowledgment](Features-Alert-Acknowledgment)

## Documentation

- [Complete Feature Specifications](Complete-Feature-Specifications)
- [Quick Reference Guide](Features-Quick-Reference)
- [Troubleshooting](Troubleshooting)

## Development

- [Installation](Installation)
- [Contributing](Contributing)
- [Issues](https://github.com/[username]/HamClock/issues)

---

**Last Updated:** 2025-12-19
**Version:** Phase 8 v1.0.0
```

---

## Tips for Wiki Success

### 1. Keep It Organized
- Use consistent heading hierarchy
- Group related content
- Link between pages

### 2. Include Examples
- Configuration snippets
- Command examples
- Expected output

### 3. Use Tables
- Feature comparison
- Configuration reference
- Platform compatibility

### 4. Add Visuals (if supported)
- Create diagrams in Mermaid
- Add screenshots
- Use formatting effectively

### 5. Maintain Current
- Update when features change
- Link to issues/PRs
- Note deprecations

---

## GitHub Wiki Features

### Supported:
- ✅ Markdown formatting
- ✅ LaTeX equations
- ✅ Mermaid diagrams
- ✅ Code syntax highlighting
- ✅ Tables and lists
- ✅ Embedded links
- ✅ Edit history
- ✅ Collaboration

### Not Supported:
- ❌ GitHub Actions (wiki is separate)
- ❌ Direct code repository linking (must use URLs)
- ❌ JavaScript or HTML embedding
- ❌ Media uploads (use markdown image links)

---

## Quick Reference: Wiki URLs

```
Main Wiki:
https://github.com/[username]/HamClock/wiki

Specific Pages:
https://github.com/[username]/HamClock/wiki/Home
https://github.com/[username]/HamClock/wiki/Phase-8-Overview
https://github.com/[username]/HamClock/wiki/Features-CME-Detection
https://github.com/[username]/HamClock/wiki/Deployment-Guide

Edit a Page:
Click the pencil icon (✏️) in the top right of any page
```

---

## Next Steps

1. **Enable Wiki** (if not already enabled)
   - Repository Settings → Features → Wikis

2. **Create Home Page**
   - Use the sample provided above

3. **Import Documentation**
   - Copy Phase 8 markdown files to wiki

4. **Create Navigation**
   - Add _Sidebar.md for easy navigation

5. **Link from Repository**
   - Add wiki link to README.md

6. **Keep Updated**
   - Update wiki as development continues

---

**Wiki Setup Complete!** 🎉

Your GitHub wiki is now ready to host comprehensive HamClock documentation.
