# Claude AI Assistant Instructions

## PDF Extractor GUI Project

### IMPORTANT: UI Preservation Rules

**DO NOT MODIFY THE USER INTERFACE** unless explicitly instructed by the user. This includes:

- **Layout/Geometry**: Do not rearrange, resize, or reposition any UI elements
- **Colors**: Do not change any background colors, text colors, or styling
- **Tab Names**: Keep all tab names exactly as they are
- **Button Styles**: Maintain current button appearance and positioning
- **Prompt Strings**: Do not modify any AI prompt text or preprompts
- **Control Visibility**: Do not add or remove controls without explicit request

The UI has been carefully designed and tested. Any changes should only be made when the user specifically requests them.

### Current UI State (as of 2025-09-23)

- **Input Tabs**: "PDF File" and "Paste Text" with Analyze buttons in lower right
- **Output Tabs**: "Extracted Text", "Summary Result", "Keywords Result", "Run Log"
- **Settings Dialog**: Connection, Summary, Keywords, and Prompt Refinement tabs
- **Database**: SQLite-based settings storage with per-prompt configuration
- **Status Bar**: Shows status messages and spinner animation during processing

### Technical Notes

- Built with Qt 6.10.0 using LLVM-MinGW compiler
- Uses QPdfDocument for PDF text extraction
- Integrates with LM Studio API for AI text processing
- Settings stored in SQLite database (release/settings.db)