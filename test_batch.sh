#!/bin/bash

echo "========================================"
echo "PDF Extraction Batch Test with AI"
echo "========================================"
echo ""

echo "Processing Paper 1: Attention Is All You Need"
echo "----------------------------------------"
./release/pdfextract.exe paper1_ai.pdf batch_paper1_text.txt -p 1-5 \
  --config lmstudio_config.toml \
  --summary batch_paper1_summary.txt \
  --keywords batch_paper1_keywords.txt \
  --verbose

echo ""
echo "========================================"
echo ""

echo "Processing Paper 2: BERT"
echo "----------------------------------------"
./release/pdfextract.exe paper2_bert.pdf batch_paper2_text.txt -p 1-5 \
  --config lmstudio_config.toml \
  --summary batch_paper2_summary.txt \
  --keywords batch_paper2_keywords.txt \
  --verbose

echo ""
echo "========================================"
echo "Batch processing complete!"
echo ""
echo "Output files created:"
echo "- batch_paper1_text.txt (Attention paper text)"
echo "- batch_paper1_summary.txt (Attention paper AI summary)"
echo "- batch_paper1_keywords.txt (Attention paper AI keywords)"
echo "- batch_paper2_text.txt (BERT paper text)"
echo "- batch_paper2_summary.txt (BERT paper AI summary)"
echo "- batch_paper2_keywords.txt (BERT paper AI keywords)"
echo "========================================"