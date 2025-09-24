# PDF Extractor GUI - Build Documentation

## Build System Overview

This project uses Qt's qmake build system with a master GNUmakefile for convenience. The build system supports 4 distinct configurations, each serving a specific purpose.

## Build Configurations

| Configuration | Console | Debug | Target | Use Case |
|--------------|---------|-------|---------|-----------|
| **release** | No | No | `build/release/pdfextractor_gui.exe` | **End users** - Production build |
| **release-console** | Yes | No | `build/release-console/pdfextractor_gui_console.exe` | **Claude/Development** - With console output |
| **debug** | No | Yes | `build/debug/pdfextractor_gui_debug.exe` | Windows GUI debugging |
| **debug-console** | Yes | Yes | `build/debug-console/pdfextractor_gui_debug_console.exe` | Full debugging with console |

## Quick Start

### For End Users
```bash
make              # Builds release version (no console)
make deploy       # Builds and deploys with Qt libraries
```

### For Claude/Development
```bash
make release-console    # Builds with console output
make deploy-claude      # Builds and deploys Claude version
```

### For Debugging
```bash
make debug              # Debug build without console
make debug-console      # Debug build with console
```

## Directory Structure

```
gui-extractor/
├── GNUmakefile         # Master build file
├── pdfextractor_gui.pro # Qt project file
├── build/              # All build outputs
│   ├── release/        # Production build (no console)
│   ├── release-console/ # Claude/debug build (with console)
│   ├── debug/          # Debug build (no console)
│   └── debug-console/  # Debug with console
└── [source files]
```

## Build Commands

### Primary Targets
- `make` or `make release` - Production Windows GUI
- `make release-console` - Release with console (for Claude)
- `make debug` - Debug Windows GUI
- `make debug-console` - Debug with console
- `make all` - Build all 4 configurations

### Deployment
- `make deploy` - Deploy release build with Qt dependencies
- `make deploy-claude` - Deploy Claude build with Qt dependencies

### Maintenance
- `make clean` - Remove all build artifacts
- `make help` - Show detailed help

## Important Notes

### For Claude AI Assistant
**Always use `make release-console`** when building from Claude. This configuration:
- Provides console output for debugging
- Shows qDebug() messages
- Allows you to see runtime errors
- Essential for development and testing

### For End Users
The default `make` command builds the production version:
- No console window
- Professional Windows application
- All debug output suppressed
- Includes application icon

### Database Location
All builds use the executable directory for the SQLite database (`settings.db`). The application will:
1. Check for database in exe directory
2. Create it if missing
3. Fall back to AppData if no write permission
4. Use in-memory database as last resort

### Crash Protection
All builds include:
- Windows exception handling
- Crash logs saved to `logs/` directory
- Graceful error recovery
- Resource verification on startup

## Build Requirements

- Qt 6.10.0 with LLVM-MinGW compiler
- MinGW-w64 toolchain
- Windows SDK libraries

## Troubleshooting

### Build Fails
```bash
make clean        # Clean all artifacts
make release-console  # Rebuild
```

### Missing Dependencies
```bash
make deploy-claude    # Deploys all Qt dependencies
```

### Wrong Configuration
Check which executable you're running:
- `pdfextractor_gui.exe` - No console (end users)
- `pdfextractor_gui_console.exe` - With console (Claude/debug)

## Development Workflow

1. **Make changes** to source files
2. **Build with console** for testing: `make release-console`
3. **Test** the application
4. **Build production** when ready: `make release`
5. **Deploy** for distribution: `make deploy`

## CI/CD Integration

The GNUmakefile is designed for easy CI/CD integration:

```yaml
# Example GitHub Actions
- run: make clean
- run: make all
- run: make deploy
```

## Version Information

Current version: 3.0.0
- Major rewrite with QueryRunner architecture
- Robust error handling and crash protection
- Four distinct build configurations
- Comprehensive build system