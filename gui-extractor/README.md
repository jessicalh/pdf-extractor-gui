# PDF Extractor GUI

A Qt-based GUI application for PDF text extraction with AI-powered summary and keyword generation.

## Features

- **Dual Mode**: Works as both GUI application and command-line tool
- **PDF Text Extraction**: Extract text from specific page ranges
- **Copyright Removal**: Optional removal of copyright notices
- **AI Integration**: Generate summaries and extract keywords using LM Studio
- **File Picker**: Browse and select PDF files easily
- **Tabbed Interface**: View extracted text, summary, and keywords in separate tabs
- **Real-time Settings**: Adjust temperature, max tokens, and model on the fly
- **Progress Tracking**: Visual progress bar during processing
- **Command Line Compatible**: Supports all CLI parameters from the original tool

## Quick Start

### GUI Mode
```batch
run_gui.bat
```
Or directly:
```batch
release\pdfextractor_gui.exe --gui
```

### Command Line Mode
```batch
release\pdfextractor_gui.exe input.pdf output.txt -p 1-5 --config lmstudio_config.toml --summary summary.txt --keywords keywords.txt
```

## GUI Interface

### Main Sections:

1. **File Selection**
   - Browse button to select PDF files
   - Text field shows selected file path
   - Extract & Process button to start

2. **Settings Panel**
   - Page range selection (start/end pages)
   - Preserve copyright checkbox
   - Generate summary/keywords checkboxes
   - Temperature control (0.0-2.0)
   - Max tokens setting (100-32000)
   - Model name field

3. **Results Tabs**
   - **Extracted Text**: Full text from PDF
   - **Summary**: AI-generated summary
   - **Keywords**: AI-extracted keywords

## Command Line Options

```
Usage: pdfextractor_gui.exe [options] pdf output

Options:
  -?, -h, --help             Show help
  -v, --version              Show version
  -p, --pages <range>        Page range (e.g., 1-10 or 5)
  --preserve                 Preserve copyright notices
  -c, --config <config>      TOML configuration file
  -s, --summary <file>       Output file for summary
  -k, --keywords <file>      Output file for keywords
  --verbose                  Enable verbose output
  -g, --gui                  Force GUI mode even with arguments

Arguments:
  pdf                        PDF file to extract
  output                     Output text file
```

## Configuration

The application uses `lmstudio_config.toml` for AI settings:
- LM Studio endpoint: `172.20.10.3:8090`
- Model: `gpt-oss-120b`
- Temperature: `0.8`
- Max tokens: `8000`

## Testing

Run the test suite:
```batch
test_gui.bat
```

This runs:
1. Basic command line extraction
2. Command line with AI features
3. Verbose mode with all features
4. Launches GUI for interactive testing

## File Structure

```
gui-extractor/
├── release/
│   ├── pdfextractor_gui.exe     # Main executable
│   ├── Qt6*.dll                 # Qt runtime libraries
│   ├── platforms/               # Qt platform plugins
│   ├── styles/                  # Qt style plugins
│   └── translations/            # Qt translations
├── lmstudio_config.toml         # AI configuration
├── run_gui.bat                  # GUI launcher
└── test_gui.bat                 # Test suite
```

## Examples

### GUI Mode Example
1. Launch with `run_gui.bat`
2. Click "Browse..." to select a PDF
3. Adjust page range if needed
4. Click "Extract & Process"
5. View results in tabs
6. Copy text from any tab

### Command Line Example
```batch
# Extract pages 1-10 with AI processing
release\pdfextractor_gui.exe paper.pdf extracted.txt -p 1-10 --config lmstudio_config.toml --summary summary.txt --keywords keywords.txt --verbose

# Basic extraction without AI
release\pdfextractor_gui.exe paper.pdf extracted.txt -p 1-5

# GUI mode with pre-filled parameters
release\pdfextractor_gui.exe paper.pdf output.txt --gui
```

## Requirements

- Windows 10/11
- Qt 6.10.0 runtime (included)
- LM Studio running at configured endpoint for AI features
- Active network connection for AI features

## Troubleshooting

1. **Missing DLL Error**: All required DLLs are in the release folder
2. **AI Features Not Working**: Check LM Studio is running at `172.20.10.3:8090`
3. **PDF Load Error**: Ensure PDF is not password protected or corrupted
4. **No Keywords Generated**: Some papers may not contain scientific terms

## Building from Source

```batch
cd gui-extractor
qmake pdfextractor_gui.pro
mingw32-make release
windeployqt release\pdfextractor_gui.exe
```