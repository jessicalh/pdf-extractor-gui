import sqlite3
import os

db_path = "settings.db"

if os.path.exists(db_path):
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Get all records
    cursor.execute("SELECT * FROM settings")
    rows = cursor.fetchall()

    # Get column names
    cursor.execute("PRAGMA table_info(settings)")
    columns = cursor.fetchall()

    print("Database Schema:")
    print("-" * 80)
    for col in columns:
        print(f"Column: {col[1]:35} Type: {col[2]}")

    print("\nDatabase Content:")
    print("-" * 80)

    if rows:
        row = rows[0]
        col_names = [col[1] for col in columns]

        for i, col_name in enumerate(col_names):
            value = row[i] if i < len(row) else None
            if value and len(str(value)) > 100:
                value = str(value)[:97] + "..."
            print(f"{col_name:35}: {value}")
    else:
        print("No records found in settings table")

    conn.close()
else:
    print(f"Database file '{db_path}' not found")