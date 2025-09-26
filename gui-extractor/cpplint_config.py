"""
cpplint configuration for Qt projects
Google's cpplint with Qt-specific adjustments
"""

# Filters for Qt-specific patterns
filters = [
    # Disable checks that conflict with Qt style
    '-whitespace/indent',  # Qt uses 4 spaces, not Google's 2
    '-build/include_order',  # Qt headers have specific order
    '-runtime/references',  # Qt uses const& extensively
    '-readability/streams',  # Qt uses QDebug streams

    # Qt-specific allowances
    '-build/include_what_you_use',  # Qt forward declares a lot
    '-runtime/explicit',  # Qt constructors often single-arg

    # Project-specific
    '-legal/copyright',  # Add if you have copyright headers
    '-build/header_guard',  # Qt uses #pragma once or different style
]

# Set line length (Qt typically uses 100-120)
linelength = 120

# Extensions to check
extensions = ['cpp', 'h']

# Root directory
root = 'gui-extractor'