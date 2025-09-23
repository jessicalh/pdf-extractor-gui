import sqlite3
import os

db_path = "release/settings.db" if os.path.exists("release/settings.db") else "settings.db"

if os.path.exists(db_path):
    print(f"[OK] Database found at: {db_path}")

    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Get schema
    cursor.execute("PRAGMA table_info(settings)")
    columns = cursor.fetchall()

    print("\nDatabase Schema (New):")
    print("=" * 60)
    for col in columns:
        print(f"  {col[1]:35} {col[2]:10}")

    # Check data
    cursor.execute("SELECT * FROM settings LIMIT 1")
    row = cursor.fetchone()

    if row:
        print("\n[OK] Default values initialized")
        print("\nConnection Settings:")
        print(f"  URL: {row[1]}")
        print(f"  Model: {row[2]}")
        print(f"  Overall Timeout: {row[3]} ms")

        print("\nSummary Settings:")
        print(f"  Temperature: {row[4]}")
        print(f"  Context Length: {row[5]} tokens")
        print(f"  Timeout: {row[6]} ms")

        print("\nKeywords Settings:")
        print(f"  Temperature: {row[9]}")
        print(f"  Context Length: {row[10]} tokens")
        print(f"  Timeout: {row[11]} ms")
    else:
        print("\n[WARNING] No data found in settings table")

    conn.close()
else:
    print(f"[ERROR] Database not found at {db_path}")