import sqlite3
import os

db_path = "settings.db"

if os.path.exists(db_path):
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    cursor.execute("SELECT * FROM settings LIMIT 1")
    row = cursor.fetchone()

    if row:
        print("=" * 80)
        print("COMPLETE PROMPT CONFIGURATION")
        print("=" * 80)

        print("\n1. CONNECTION SETTINGS:")
        print("-" * 40)
        print(f"URL: {row[1]}")
        print(f"Model: {row[2]}")
        print(f"Temperature: {row[3]}")
        print(f"Context Length: {row[4]} tokens")

        print("\n2. SUMMARY GENERATION:")
        print("-" * 40)
        print("Pre-prompt:")
        print(row[5])
        print("\nMain Prompt:")
        print(row[6])

        print("\n3. KEYWORD EXTRACTION:")
        print("-" * 40)
        print("Pre-prompt:")
        print(row[7])
        print("\nMain Prompt:")
        print(row[8])

        print("\n4. KEYWORD REFINEMENT:")
        print("-" * 40)
        print("Pre-prompt:")
        print(row[9])

        print("\n5. PREPROMPT REFINEMENT:")
        print("-" * 40)
        print("Prompt:")
        print(row[10])

    conn.close()
else:
    print(f"Database file '{db_path}' not found")