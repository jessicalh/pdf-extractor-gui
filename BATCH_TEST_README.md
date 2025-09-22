# PDF Extractor Batch Testing

## Quick Start

### Windows (Command Prompt)
```batch
run_papers.bat
```
This runs both scientific papers with full AI processing in verbose mode.

### WSL/Linux
```bash
./test_batch.sh
```

## Available Batch Files

### 1. `run_papers.bat` (Simple Run)
- Processes both papers (Attention and BERT)
- Extracts pages 1-5 from each
- Generates AI summaries and keywords
- Shows verbose output
- Creates: attention_*.txt and bert_*.txt files

### 2. `test_batch.bat` / `test_batch.sh` (Standard Test)
- Processes both papers with consistent naming
- Extracts pages 1-5 from each
- Full AI processing with verbose mode
- Creates: batch_paper1_*.txt and batch_paper2_*.txt files

### 3. `full_test.bat` (Comprehensive Test Suite)
Runs 6 different tests:
- Test 1: Basic extraction without AI
- Test 2: Extraction with copyright preserved
- Test 3: AI Summary only (Attention paper)
- Test 4: AI Keywords only (BERT paper)
- Test 5: Full AI processing with verbose (Attention)
- Test 6: Full AI processing with verbose (BERT)

## Output Files

Each run creates three files per paper:
- `*_text.txt` - Extracted text with copyright removed
- `*_summary.txt` - AI-generated summary
- `*_keywords.txt` - AI-extracted keywords

## Configuration

The batch files use `lmstudio_config.toml` which connects to:
- LM Studio at: 172.20.10.3:8090
- Model: gpt-oss-120b
- Temperature: 0.8
- Max tokens: 8000

## Verbose Mode

All batch files run in verbose mode, showing:
- Configuration details
- API request parameters
- Response sizes
- Partial response content
- Processing progress

## Test Papers

1. **paper1_ai.pdf** - "Attention Is All You Need" (Transformer paper)
2. **paper2_bert.pdf** - "BERT: Pre-training of Deep Bidirectional Transformers"

## Troubleshooting

If LM Studio connection fails:
1. Check LM Studio is running at 172.20.10.3:8090
2. Verify the model is loaded
3. Check network connectivity
4. Review verbose output for error messages

## Manual Testing

Run individual tests:
```batch
release\pdfextract.exe paper1_ai.pdf output.txt -p 1-5 --config lmstudio_config.toml --summary summary.txt --keywords keywords.txt --verbose
```