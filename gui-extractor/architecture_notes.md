# Architecture Notes

## Network Request Handling

### Decision: Keep PromptQuery as Network Abstraction for LLM
**Date**: 2025-09-23

We considered adding a generic `NetworkRequest` abstraction layer but decided against it because:

1. **PromptQuery already serves as our LLM network abstraction**
   - Handles all network communication with LM Studio
   - Manages timeouts and error handling
   - Processes LLM-specific responses
   - Provides appropriate signals for results

2. **YAGNI Principle (You Aren't Gonna Need It)**
   - Our current need is only for LLM queries
   - PromptQuery handles this perfectly
   - Adding generic abstraction adds unnecessary complexity

3. **Future Extensibility**
   - When we add Zotero integration, create a dedicated `ZoteroClient` class
   - Each API client can handle its own specific needs
   - No need for a generic abstraction unless we have 3+ different API types

## Current Architecture

```
QueryRunner (Pipeline Manager)
    ├── SummaryQuery (LLM Network Handler)
    ├── KeywordsQuery (LLM Network Handler)
    ├── RefineKeywordsQuery (LLM Network Handler)
    └── KeywordsWithRefinementQuery (LLM Network Handler)
```

Each `PromptQuery` subclass:
- Manages its own network requests to LM Studio
- Handles response parsing specific to its query type
- Emits appropriate signals for UI updates

## Future Additions

When adding Zotero support:
1. Create a dedicated `ZoteroClient` class
2. Handle Zotero-specific authentication (API keys)
3. Implement Zotero API methods (collections, items, attachments)
4. Use Qt's network classes directly, similar to PromptQuery

This keeps each integration focused and maintainable without unnecessary abstraction layers.