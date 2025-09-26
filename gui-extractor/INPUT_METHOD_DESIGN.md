# Input Method Design

## Definition

An **Input Method** is a modular component responsible for obtaining text content from various sources and preparing it for LLM analysis. Each input method:

### Core Responsibilities
1. **Obtains content** from one of:
   - User input (paste/type)
   - Local file system (PDF files)
   - Network sources (web services, APIs)

2. **Produces** copyright-stripped, extracted text ready for LLM analysis

3. **Resource Management**:
   - Acquires necessary resources (file handles, network connections, memory)
   - **MUST** free all resources once extraction is complete
   - No resources should remain open after text extraction

### Lifecycle & Control Flow

1. **Interactive Phase** (before "Analyze" button):
   - Input method owns the UI interaction within its tab
   - Can show error dialogs, validation messages
   - Allows user to retry/fix issues
   - No processing pipeline involvement

2. **Extraction Phase** (after "Analyze" button):
   - Control transfers to main process handler (QueryRunner)
   - Input method performs extraction
   - Returns extracted text or error
   - All resources are freed
   - No further UI interaction from input method

### Error Handling

- **During Interactive Phase**:
  - Show message boxes
  - Allow user to correct and retry
  - Take no processing action

- **During Extraction Phase**:
  - Return errors to main process handler
  - Main process handles recovery/retry logic

## Implementation Requirements

Each input method must:

1. Provide a UI component (tab widget)
2. Implement `extractText()` method returning copyright-stripped text
3. Implement `cleanup()` method to free all resources
4. Handle validation before extraction
5. Be self-contained and modular

## Current Input Methods

1. **PDF File Input**: Local PDF file selection and extraction
2. **Paste Text Input**: Direct text input from clipboard
3. **Zotero Input** (planned): Web service integration for academic papers

## Architecture Integration

```
┌─────────────────────────────────────┐
│         Main Window (GUI)           │
├─────────────────────────────────────┤
│  ┌────────────────────────────┐     │
│  │     Input Method Tabs      │     │
│  │  ┌──────┬──────┬────────┐ │     │
│  │  │ PDF  │Paste │ Zotero │ │     │
│  │  └──────┴──────┴────────┘ │     │
│  │      [Analyze Button]      │     │
│  └────────────────────────────┘     │
│               ↓                      │
│  ┌────────────────────────────┐     │
│  │      QueryRunner           │     │
│  │  (Processing Pipeline)     │     │
│  └────────────────────────────┘     │
└─────────────────────────────────────┘
```

## Design Principles

1. **Separation of Concerns**: Input acquisition vs. text processing
2. **Resource Safety**: Guaranteed cleanup of all resources
3. **User Control**: Interactive until explicit processing request
4. **Modularity**: Easy to add new input methods
5. **Consistency**: Uniform interface for all input methods