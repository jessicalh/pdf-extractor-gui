@echo off
echo Running PDF extraction with AI on both papers in verbose mode...
echo.

echo [1/2] Processing Attention paper (pages 1-5)...
release\pdfextract.exe paper1_ai.pdf attention_output.txt -p 1-5 --config lmstudio_config.toml --summary attention_summary.txt --keywords attention_keywords.txt --verbose

echo.
echo ----------------------------------------
echo.

echo [2/2] Processing BERT paper (pages 1-5)...
release\pdfextract.exe paper2_bert.pdf bert_output.txt -p 1-5 --config lmstudio_config.toml --summary bert_summary.txt --keywords bert_keywords.txt --verbose

echo.
echo Done! Check the output files:
echo - attention_output.txt, attention_summary.txt, attention_keywords.txt
echo - bert_output.txt, bert_summary.txt, bert_keywords.txt
pause