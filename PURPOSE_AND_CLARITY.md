# Project Purpose and Clarity Statement

## What This Is

A simple Windows tool that extracts keywords and summaries from academic PDFs for use with Zotero reference management. It helps researchers build keyword taxonomies and enables full-text searches to trace conceptual threads through literature collections.

## Why This Exists

### The Practical Problem
When managing hundreds of papers in Zotero, researchers need:
1. Consistent keyword extraction for search and filtering
2. Brief summaries to quickly recall paper contents
3. Domain-specific terms that automated extraction might miss
4. Clean text without copyright notices cluttering searches

### Our Approach: A Four-Stage Pipeline

Each stage has a specific job:

1. **Summary** - Creates a brief overview for quick reference in Zotero
2. **Keywords** - Extracts initial terms using a general scientific template
3. **Refinement** - Attempts to improve the keyword prompt based on this specific paper
4. **Refined Keywords** - Tries again with the improved prompt (if available)

### Why the Refinement Step?

The idea is straightforward: the AI looks at what it found in the initial keyword extraction and tries to tune the prompt for this particular paper's domain. Sometimes this works well (adding specific technical terms), sometimes it doesn't produce anything useful. When it works, papers about similar topics benefit from more targeted extraction.

## Technical Clarity

### Why Qt Signals/Slots?

It keeps the UI responsive during long API calls and makes it easier to track what's happening at each step. The UI and processing logic don't need to know about each other's internals.

### Why SQLite Over TOML?

TOML files were getting unwieldy with all the per-prompt settings. SQLite lets us store and retrieve settings more reliably, and makes it easier to add features later (like saving successful refinements).

### Why QueryRunner as Orchestrator?

It keeps all the pipeline logic in one place. The QueryRunner manages the sequence of API calls, while the UI just displays results. This makes it easier to debug when something goes wrong in the pipeline.

## Current Focus Areas

### What's Working Well
- ✅ Complete pipeline execution with proper stage transitions
- ✅ Robust error handling with graceful degradation
- ✅ Comprehensive logging for debugging (UI + file)
- ✅ Clean separation of concerns
- ✅ Proper Windows localhost networking (no WSL issues)

### What Needs Refinement
- 🔧 The refinement prompt needs tuning to consistently produce improved prompts
- 🔧 Consider caching successful refinements for similar document types
- 🔧 Add metrics to measure keyword extraction quality

## Design Decisions and Rationale

### Why Continue After Empty Keywords?
Keywords might fail but refinement could still suggest improvements for future runs. This provides learning opportunities even from failures.

### Why Stop After Empty Summary?
If we can't summarize, we likely can't extract meaningful keywords either. This prevents wasting API calls on clearly unsuitable text.

### Why Stop After Failed Refinement?
Without a refined prompt, Stage 4 would just repeat Stage 2. No value in redundant processing.

### Why 10-Minute Timeouts?
Large documents + slower models (gpt-oss-120b) + complex prompts = long processing times. Better to wait than fail prematurely.

## Integration with Zotero Workflow

The extracted keywords and summaries are designed to paste directly into Zotero's metadata fields. The keywords follow standard comma-delimited format, and summaries are kept concise for the Abstract field. The clean extracted text (without copyright) can be stored as a note for full-text searching.

## Known Limitations

- The refinement step is experimental and doesn't always improve results
- Large PDFs may take several minutes to process through all stages
- Some papers produce better keywords than others depending on domain
- The 10-minute timeout might still be too short for very large documents

## For Future Maintainers

See `SIGNALS_AND_SLOTS_REFERENCE.md` for the complete signal flow with actual object and method names. If you need to trace why something isn't updating in the UI or why a stage isn't completing, that document shows exactly which signals connect to which slots throughout the pipeline.