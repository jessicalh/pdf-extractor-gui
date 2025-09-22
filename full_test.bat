@echo off
title PDF Extractor Full Test Suite
echo ================================================================================
echo                     PDF EXTRACTOR WITH AI - FULL TEST SUITE
echo ================================================================================
echo.
echo Configuration:
echo - LM Studio endpoint: 172.20.10.3:8090
echo - Model: gpt-oss-120b
echo - Temperature: 0.8
echo - Max tokens: 8000
echo.
echo ================================================================================
echo.

echo [TEST 1] Basic extraction without AI (first 2 pages)
echo --------------------------------------------------------------------------------
release\pdfextract.exe paper1_ai.pdf test1_basic.txt -p 1-2
echo.
timeout /t 2 >nul

echo [TEST 2] Full extraction with copyright preserved
echo --------------------------------------------------------------------------------
release\pdfextract.exe paper2_bert.pdf test2_preserved.txt -p 1-3 --preserve
echo.
timeout /t 2 >nul

echo [TEST 3] AI Summary generation only (Attention paper)
echo --------------------------------------------------------------------------------
release\pdfextract.exe paper1_ai.pdf test3_text.txt -p 1-3 --config lmstudio_config.toml --summary test3_summary.txt
echo.
timeout /t 2 >nul

echo [TEST 4] AI Keywords generation only (BERT paper)
echo --------------------------------------------------------------------------------
release\pdfextract.exe paper2_bert.pdf test4_text.txt -p 1-3 --config lmstudio_config.toml --keywords test4_keywords.txt
echo.
timeout /t 2 >nul

echo [TEST 5] Full AI processing with verbose mode (Attention paper - 5 pages)
echo --------------------------------------------------------------------------------
release\pdfextract.exe paper1_ai.pdf test5_text.txt -p 1-5 --config lmstudio_config.toml --summary test5_summary.txt --keywords test5_keywords.txt --verbose
echo.
timeout /t 2 >nul

echo [TEST 6] Full AI processing with verbose mode (BERT paper - 5 pages)
echo --------------------------------------------------------------------------------
release\pdfextract.exe paper2_bert.pdf test6_text.txt -p 1-5 --config lmstudio_config.toml --summary test6_summary.txt --keywords test6_keywords.txt --verbose
echo.

echo ================================================================================
echo                              TEST SUITE COMPLETE
echo ================================================================================
echo.
echo Test outputs created:
echo.
echo Basic tests:
echo   - test1_basic.txt (Attention paper, pages 1-2, no AI)
echo   - test2_preserved.txt (BERT paper, pages 1-3, copyright preserved)
echo.
echo AI tests:
echo   - test3_text.txt + test3_summary.txt (Attention summary only)
echo   - test4_text.txt + test4_keywords.txt (BERT keywords only)
echo.
echo Full verbose tests:
echo   - test5_text.txt + test5_summary.txt + test5_keywords.txt (Attention full)
echo   - test6_text.txt + test6_summary.txt + test6_keywords.txt (BERT full)
echo.
echo ================================================================================
echo.
pause