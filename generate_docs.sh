#!/bin/bash

# DataPak Documentation Generation Script
#
# This script generates HTML documentation from Doxygen comments
# using the included Doxyfile configuration.

set -e

echo "Generating DataPak documentation..."

# Check if doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "Error: Doxygen is not installed."
    echo "Please install doxygen:"
    echo "  Ubuntu/Debian: sudo apt-get install doxygen"
    echo "  macOS: brew install doxygen"
    echo "  Windows: Download from https://www.doxygen.nl/download.html"
    exit 1
fi

# Check if graphviz is available for diagrams (optional)
if ! command -v dot &> /dev/null; then
    echo "Warning: Graphviz (dot) is not installed."
    echo "Install it for better diagrams:"
    echo "  Ubuntu/Debian: sudo apt-get install graphviz"
    echo "  macOS: brew install graphviz"
    echo ""
fi

# Create docs directory if it doesn't exist
mkdir -p docs

# Run doxygen
echo "Running doxygen..."
doxygen Doxyfile

# Check if generation was successful
if [ -d "docs/html" ] && [ -f "docs/html/index.html" ]; then
    echo ""
    echo "Documentation generated successfully!"
    echo "Open docs/html/index.html in your web browser to view the documentation."
    echo ""
    echo "You can also serve it locally:"
    echo "  cd docs/html && python3 -m http.server 8080"
    echo "  Then open http://localhost:8080 in your browser"
else
    echo "Error: Documentation generation failed!"
    exit 1
fi