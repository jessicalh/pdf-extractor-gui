# PDF Text Extractor

A Qt-based command-line tool for extracting text from PDF files with automatic copyright notice removal.

## Building

### Using qmake (Recommended for Windows)

```bash
qmake pdfextract.pro
make
```

Or on Windows with MinGW:
```bash
qmake pdfextract.pro
mingw32-make
```

Or with MSVC:
```bash
qmake pdfextract.pro
nmake
```

### Using CMake

```bash
mkdir build
cd build
cmake ..
make
```

For a static build (single executable):
```bash
cmake -DBUILD_STATIC=ON ..
```

## Usage

Basic usage:
```bash
pdfextract input.pdf output.txt
```

Extract specific pages:
```bash
pdfextract input.pdf output.txt -p 1-10
```

Extract single page:
```bash
pdfextract input.pdf output.txt -p 5
```

Preserve copyright notices:
```bash
pdfextract input.pdf output.txt --preserve
```

## Features

- Extracts all text from PDF files
- Automatically removes copyright notices:
  - (c), (C), ©
  - "copyright" (case insensitive)
  - "all rights reserved" (case insensitive)
- Page range selection
- UTF-8 text output
- Progress indication for large files
- Error handling for protected/corrupted PDFs

## Requirements

- Qt 5.14+ with PDF module
- C++17 compiler
- Windows: MinGW or MSVC

## Command Line Options

- `pdf`: Input PDF file path (required)
- `output`: Output text file path (required)
- `-p, --pages <range>`: Page range to extract (e.g., "1-10" or "5")
- `--preserve`: Keep copyright notices in extracted text
- `-h, --help`: Display help
- `-v, --version`: Display version