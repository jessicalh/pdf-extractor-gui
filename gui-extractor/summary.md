# PDF Extractor GUI - Project Summary

## Overview
A Qt-based GUI application for extracting text from PDFs and analyzing it using AI (via LM Studio API).

## Current State (2025-09-23)

### ⚠️ CRITICAL: UI Preservation Policy
**The UI is finalized and correct. DO NOT CHANGE:**
- Layout geometry or positioning
- Colors or styling
- Tab names or labels
- Button appearance or placement
- Prompt strings or preprompts
- Control visibility or arrangement

**Only modify UI elements when explicitly requested by the user.**

### Features
1. **PDF Text Extraction**: Load PDF files and extract all text content
2. **Direct Text Input**: Paste text directly for analysis
3. **AI Analysis**:
   - Generate summaries using customizable prompts
   - Extract keywords with refinement capabilities
   - Prompt improvement suggestions
4. **Settings Management**: SQLite database stores all configuration including:
   - Connection settings (URL, model, timeout)
   - Per-prompt settings (temperature, context length, timeout)
   - Custom prompts and preprompts for each analysis type

### UI Structure
- **Main Window**:
  - Top-level tabs: Input | Output
  - Input sub-tabs: PDF File | Paste Text
  - Output sub-tabs: Extracted Text | Summary Result | Keywords Result | Run Log
  - Status bar with spinner animation during processing

- **Settings Dialog**:
  - Tabs: Connection | Summary | Keywords | Prompt Refinement
  - Each prompt tab has "Prompt Setup" and "Prompt" sections
  - Restore Defaults button aligned with tab edge

### Technical Details
- **Framework**: Qt 6.10.0 with LLVM-MinGW compiler
- **Database**: SQLite for settings persistence
- **PDF Library**: QPdfDocument for text extraction
- **Network**: QNetworkAccessManager for LM Studio API calls
- **Build**: qmake + mingw32-make

### Recent Changes (2025-09-23)
- Removed page range controls from PDF input
- Removed preserve copyright checkbox (always strips copyright)
- Fixed button styling to use default Qt appearance
- Renamed tabs as specified by user
- Fixed log background color to match other tabs

### Build Instructions
```bash
cd gui-extractor
/c/Qt/6.10.0/llvm-mingw_64/bin/qmake.exe pdfextractor_gui.pro
mingw32-make release
```

### Testing
```bash
cd gui-extractor
./test_gui_improved.bat
```

## Important Notes
- All UI decisions have been carefully made and tested
- The application is feature-complete and stable
- Database auto-initializes with sensible defaults
- Copyright notices are automatically stripped for LLM processing