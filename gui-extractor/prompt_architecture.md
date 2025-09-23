# Prompt Query Architecture Design

## Overview
Four query types needed:
1. **Extract Summary** - Analyzes text and produces a structured summary
2. **Extract Keywords** - Extracts domain-specific keywords from text
3. **Refine Keywords** - Takes extracted keywords and improves them
4. **Keywords with Refinement** - Uses the refined prompt to extract keywords

## Class Hierarchy

```cpp
class PromptQuery : public QObject {
    Q_OBJECT

protected:
    // Common settings
    QString m_url;
    QString m_modelName;
    double m_temperature;
    int m_contextLength;
    int m_timeout;

    // Prompt components
    QString m_preprompt;
    QString m_prompt;

    // Network handling
    QNetworkAccessManager* m_networkManager;

public:
    // Virtual methods for customization
    virtual QString buildFullPrompt(const QString& text) = 0;
    virtual void processResponse(const QString& response) = 0;
    virtual QString getQueryType() const = 0;

    // Common methods
    void execute(const QString& inputText);
    void setSettings(double temp, int context, int timeout);
    void setPreprompt(const QString& preprompt);
    void setPrompt(const QString& prompt);

signals:
    void resultReady(const QString& result);
    void errorOccurred(const QString& error);
    void progressUpdate(const QString& status);
};
```

## Derived Classes

### 1. SummaryQuery
```cpp
class SummaryQuery : public PromptQuery {
    Q_OBJECT

public:
    QString buildFullPrompt(const QString& text) override {
        // Combine preprompt + prompt with {text} replacement
        return m_preprompt + "\n\n" +
               m_prompt.replace("{text}", text);
    }

    void processResponse(const QString& response) override {
        // Clean up and format summary
        emit resultReady(response.trimmed());
    }

    QString getQueryType() const override {
        return "Summary Extraction";
    }
};
```

### 2. KeywordsQuery
```cpp
class KeywordsQuery : public PromptQuery {
    Q_OBJECT

public:
    QString buildFullPrompt(const QString& text) override {
        return m_preprompt + "\n\n" +
               m_prompt.replace("{text}", text);
    }

    void processResponse(const QString& response) override {
        // Parse comma-delimited keywords
        QStringList keywords = response.split(",");
        for (QString& keyword : keywords) {
            keyword = keyword.trimmed();
        }
        emit resultReady(keywords.join(", "));
    }

    QString getQueryType() const override {
        return "Keyword Extraction";
    }
};
```

### 3. RefineKeywordsQuery
```cpp
class RefineKeywordsQuery : public PromptQuery {
    Q_OBJECT

private:
    QString m_originalKeywords;
    QString m_originalPrompt;

public:
    void setOriginalKeywords(const QString& keywords) {
        m_originalKeywords = keywords;
    }

    void setOriginalPrompt(const QString& prompt) {
        m_originalPrompt = prompt;
    }

    QString buildFullPrompt(const QString& text) override {
        // This uses the refinement preprompt and prompt
        QString fullPrompt = m_preprompt + "\n\n" + m_prompt;
        fullPrompt.replace("{text}", text);
        fullPrompt.replace("{keywords}", m_originalKeywords);
        fullPrompt.replace("{original_prompt}", m_originalPrompt);
        return fullPrompt;
    }

    void processResponse(const QString& response) override {
        // Returns improved prompt suggestion
        emit resultReady(response.trimmed());
    }

    QString getQueryType() const override {
        return "Keyword Refinement";
    }
};
```

### 4. KeywordsWithRefinementQuery
```cpp
class KeywordsWithRefinementQuery : public KeywordsQuery {
    Q_OBJECT

private:
    QString m_refinedPrompt;

public:
    void setRefinedPrompt(const QString& refinedPrompt) {
        m_refinedPrompt = refinedPrompt;
        // Override the base prompt with refined version
        m_prompt = m_refinedPrompt;
    }

    QString getQueryType() const override {
        return "Keywords (Refined Prompt)";
    }
};
```

## Usage Flow

### Sequential Processing Pipeline:
```cpp
// 1. Extract initial keywords
auto keywordsQuery = new KeywordsQuery();
keywordsQuery->setSettings(0.3, 8000, 60000);
keywordsQuery->setPreprompt(keywordPreprompt);
keywordsQuery->setPrompt(keywordPrompt);
connect(keywordsQuery, &PromptQuery::resultReady, [=](QString keywords) {

    // 2. Refine the keywords/prompt
    auto refineQuery = new RefineKeywordsQuery();
    refineQuery->setSettings(0.8, 8000, 60000);
    refineQuery->setPreprompt(refinementPreprompt);
    refineQuery->setPrompt(refinementPrompt);
    refineQuery->setOriginalKeywords(keywords);
    refineQuery->setOriginalPrompt(keywordPrompt);

    connect(refineQuery, &PromptQuery::resultReady, [=](QString refinedPrompt) {

        // 3. Re-extract keywords with refined prompt
        auto refinedKeywordsQuery = new KeywordsWithRefinementQuery();
        refinedKeywordsQuery->setSettings(0.3, 8000, 60000);
        refinedKeywordsQuery->setPreprompt(keywordPreprompt);
        refinedKeywordsQuery->setRefinedPrompt(refinedPrompt);

        refinedKeywordsQuery->execute(text);
    });

    refineQuery->execute(text);
});
keywordsQuery->execute(text);
```

## Benefits of This Architecture

1. **Separation of Concerns**: Each query type handles its specific logic
2. **Reusability**: Common network/error handling in base class
3. **Extensibility**: Easy to add new query types
4. **Testability**: Each class can be unit tested independently
5. **Clear Data Flow**: The pipeline of queries is explicit
6. **Type Safety**: Compile-time checking of query types

## Implementation Notes

### Database Integration
Each query class can load its settings from the SQLite database:
```cpp
void PromptQuery::loadFromDatabase(const QString& queryType) {
    QSqlQuery query;
    if (queryType == "summary") {
        query.exec("SELECT summary_temperature, summary_context_length, ...");
    } else if (queryType == "keywords") {
        query.exec("SELECT keyword_temperature, keyword_context_length, ...");
    }
    // Apply settings
}
```

### Error Handling
Base class handles common errors:
- Network timeouts
- Invalid JSON responses
- "Not Evaluated" responses
- Model errors

### Progress Tracking
Base class emits signals for UI updates:
- `progressUpdate("Sending request...")`
- `progressUpdate("Processing response...")`
- `progressUpdate("Complete")`

## Alternative Simpler Approach

If you want something lighter weight, you could use a single class with an enum:

```cpp
enum class QueryType {
    Summary,
    Keywords,
    RefineKeywords,
    KeywordsWithRefinement
};

class PromptExecutor : public QObject {
    Q_OBJECT

private:
    QueryType m_type;
    QMap<QueryType, PromptSettings> m_settings;

public:
    void execute(QueryType type, const QString& text,
                 const QVariantMap& extraParams = {});

    QString buildPrompt(QueryType type, const QString& text,
                       const QVariantMap& params);

    void processResponse(QueryType type, const QString& response);
};
```

However, the class hierarchy approach is cleaner for your use case with the complex refinement pipeline.